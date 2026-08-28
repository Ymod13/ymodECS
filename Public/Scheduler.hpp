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
    // Avvia un numero fisso di thread (di default pari ai core della CPU)
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

    // Accetta qualsiasi tipo di funzione da eseguire in background
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // Impacchetta il task in uno shared_ptr per poterlo muovere in sicurezza
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("Impossibile aggiungere task: il ThreadPool è spento.");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one(); // Sveglia uno dei thread dormienti
        return res;
    }

    // Spegne il pool in modo pulito attendendo la fine dei lavori rimasti
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
        // 1. Completa tutti i future pendenti
        wait();
        // 2. Svuota i sistemi PRIMA che il ThreadPool venga distrutto
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

    // Esegue un tick: raggruppa i sistemi in wave parallele e le esegue
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
                    // pool.enqueue sostituisce std::async
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

    // Stampa il grafo delle dipendenze tra sistemi
    void print_dependency_graph() const {
        std::cout << "\n══ Grafo dipendenze sistemi ══════════════════════\n";
        for (std::size_t i = 0; i < systems_.size(); ++i) {
            std::cout << "  [" << systems_[i].name << "]\n";
            std::cout << "    writes: " << systems_[i].writes << "\n";
            std::cout << "    reads:  " << systems_[i].reads  << "\n";
            for (std::size_t j = i + 1; j < systems_.size(); ++j) {
                if (systems_[i].conflicts_with(systems_[j])) {
                    std::cout << "     conflitto con [" << systems_[j].name << "]\n";
                } else {
                    std::cout << "    parallelo  con [" << systems_[j].name << "]\n";
                }
            }
        }
        std::cout << "══════════════════════════════════════════════════\n\n";
    }

private:
    // ── Costruisce le wave: gruppi di sistemi senza conflitti interni ──
    // Algoritmo greedy: ogni sistema va nella prima wave compatibile.
    std::vector<std::vector<std::size_t>> build_waves() const {
        std::vector<std::vector<std::size_t>> waves;
        std::vector<int> assigned(systems_.size(), -1);

        for (std::size_t i = 0; i < systems_.size(); ++i) {
            int target_wave = -1;

            if (!systems_[i].requires_single_thread()) {
                // Cerca la prima wave esistente senza conflitti
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
                        break;  // un conflitto blocca anche le wave precedenti
                    }

                }
            }

            if (target_wave == -1) {
                waves.push_back({i});          // nuova wave
            } else {
                waves[target_wave].push_back(i); // aggiunto alla wave esistente
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
