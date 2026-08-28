//
// Created by ymod1 on 12/05/2026.
//

#ifndef YMODECS_SCHEDULER_HPP
#define YMODECS_SCHEDULER_HPP
#include <chrono>
#include <future>
#include <queue>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ecs {

class ThreadPool {
public:
    // Starts a fixed number of threads (defaults to the number of CPU cores)
    ThreadPool(size_t threads = std::thread::hardware_concurrency()) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // Sleeps until a new task arises or the pool is shut down
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });

                        if (this->stop && this->tasks.empty()) {
                            return;
                        }

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    // Execute background task
                    task();
                }
            });
        }
    }

    // Accepts any type of function to run in the background
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // Wraps the task in a shared_ptr so it can be moved safely
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("Impossible to add task: ThreadPool is OFF.");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one(); // Wakes up one of the sleeping threads
        return res;
    }

    // Shuts down the pool cleanly, waiting for the remaining jobs to finish
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// ─── SystemNode ──────────────────────────────────────────────
// Registered system in the scheduler
struct SystemNode {
    std::string name;
    Signature   reads;   // read components (Read + ReadIfExists)
    Signature   writes;  // written components (Write + WriteIfExists)
    Signature   filters; // componets used as a filter during updates
    std::function<void(World&, float)> run;

    bool requires_single_thread() const {

        ComponentID id = ComponentRegistry::GetId<NoMultithreading>();

        if (filters[id])
            return true;


        return false;
    }

    // a conflict happens if a system node writes what another system is read/writing
    bool conflicts_with(const SystemNode& other) const {
        // A writes on data B is reading/writing
        if ((writes & (other.reads | other.writes)).any())
            return true;

        // B writes on data A is reading/writing
        if ((other.writes & (reads | writes)).any())
            return true;

        return false;
    }
};

// ─── Scheduler ───────────────────────────────────────────────
class Scheduler {

public:

    ~Scheduler() {
        // 1. Complete all pending futures
        wait();
        // 2. Clear the systems BEFORE the ThreadPool is destroyed
        systems_.clear();
    }

    // Registers a system with its dependencies declared via AccessMode<>
    // Usage:
    //   scheduler.add<MyAccessMode>("SystemName", [](World& w){ ... });
    template<typename TAccessMode>
    void add(std::string name, std::function<void(World&, float)> fn) {
        SystemNode node;
        node.name   = std::move(name);
        node.writes = TAccessMode::write_signature();
        node.reads  = TAccessMode::full_signature() & ~node.writes;
        node.filters = TAccessMode::filter_signature();
        node.run    = std::move(fn);
        systems_.push_back(std::move(node));
    }

    // Runs a tick: groups the systems into parallel waves and executes them
    void tick(World& world, float dt) {

        auto waves = build_waves();


        if (env::is_text_debug) {
            std::cout << "\n------ Scheduler tick (" << waves.size() << " waves) " << std::string(30, '-') << "\n";
        }

        for (std::size_t wi = 0; wi < waves.size(); ++wi) {

            futures.clear();
            auto& wave = waves[wi];

            if (env::is_text_debug) {
                std::cout << "->  Wave " << wi + 1 << ": ";

                for (auto idx : wave) std::cout << "[" << systems_[idx].name << "] ";

                std::cout << "\n";
            }

            if (wave.size()==1) {
                // No multithreading here
                systems_[wave[0]].run(world, dt);
            }
            else {
                for (auto idx : wave) {
                    // pool.enqueue replaces std::async
                    futures.push_back(Threadpool.enqueue([this, &world, idx, dt]() {
                        systems_[idx].run(world, dt);
                    }));
                }

                for (auto& f : futures) {
                    f.get();
                }
            }
        }

        if (env::is_text_debug) {
            std::cout << "-" << std::string(46, '-') << "\n";
        }
    }

    void wait() {
        for (auto& f : futures) {
            if (f.valid()) f.get();
        }
        futures.clear();
    }

    // Prints the dependency graph between systems
    void print_dependency_graph() const {
        std::cout << "\n══ Systems Dependecy Graph ══════════════════════\n";
        for (std::size_t i = 0; i < systems_.size(); ++i) {
            std::cout << "  [" << systems_[i].name << "]\n";
            std::cout << "    writes: " << systems_[i].writes << "\n";
            std::cout << "    reads:  " << systems_[i].reads  << "\n";
            for (std::size_t j = i + 1; j < systems_.size(); ++j) {
                if (systems_[i].conflicts_with(systems_[j])) {
                    std::cout << "     conflicts with [" << systems_[j].name << "]\n";
                } else {
                    std::cout << "    parallel to [" << systems_[j].name << "]\n";
                }
            }
        }
        std::cout << "══════════════════════════════════════════════════\n\n";
    }

private:
    // ── Builds the waves: groups of systems with no internal conflicts ──
    // Greedy algorithm: each system goes into the first compatible wave.
    std::vector<std::vector<std::size_t>> build_waves() const {
        std::vector<std::vector<std::size_t>> waves;
        std::vector<int> assigned(systems_.size(), -1);

        for (std::size_t i = 0; i < systems_.size(); ++i) {
            int target_wave = -1;

            if (!systems_[i].requires_single_thread()) {
                // Looks for the first existing wave with no conflicts
                for (int wi = static_cast<int>(waves.size()) - 1; wi >= 0; --wi) {
                    bool ok = true;
                    for (auto idx : waves[wi]) {
                        if (systems_[idx].requires_single_thread() || systems_[i].conflicts_with(systems_[idx])) {
                            ok = false;
                            break;
                        }
                    }
                    if (ok) {
                        target_wave = wi;
                    } else {
                        break;  // a conflict also blocks earlier waves
                    }

                }
            }

            if (target_wave == -1) {
                waves.push_back({i});          // new wave
            } else {
                waves[target_wave].push_back(i); // added to the existing wave
            }
            assigned[i] = target_wave == -1
                ? static_cast<int>(waves.size()) - 1
                : target_wave;
        }
        return waves;
    }

    std::vector<std::future<void>> futures;
    std::vector<SystemNode> systems_;
    ThreadPool Threadpool;
};

} // namespace ecs

#endif //YMODECS_SCHEDULER_HPP