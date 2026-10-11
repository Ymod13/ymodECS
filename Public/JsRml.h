//
// Created by ymod1 on 09/10/2026.
//
#pragma once

#ifndef YMODECS_JSRML_H
#define YMODECS_JSRML_H
#include <fstream>
#include <SDL3/SDL_log.h>
// JsRml.h

extern "C" {
#include "quickjs.h"
}

#include <RmlUi/Core.h>
#include <string>

namespace JsRml {
    void Register(JSContext* ctx, Rml::Context* rml);   // dopo Rml::Initialise
    bool RunFile(JSContext* ctx, const std::string& path);
    void Shutdown();
}
//-------------------------------------------------------------------------------------------------------

class JsRuntime
{
public:
    JsRuntime() {
        rt  = JS_NewRuntime();
        JS_SetDumpFlags(rt, JS_DUMP_LEAKS);
        ctx = JS_NewContext(rt);
        JS_SetModuleLoaderFunc(rt, nullptr, ModuleLoader, nullptr);  // import tra file
    }
    ~JsRuntime() {
        JsRml::Shutdown();
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }

    JsRuntime(const JsRuntime&) = delete;
    JsRuntime& operator=(const JsRuntime&) = delete;

    bool RunFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        std::string src((std::istreambuf_iterator<char>(f)), {});

        JSValue r = JS_Eval(ctx, src.c_str(), src.size(), path.c_str(),
                            JS_EVAL_TYPE_MODULE);
        bool ok = !JS_IsException(r);
        if (!ok) DumpError();
        JS_FreeValue(ctx, r);
        return ok;
    }
    //-------------------------------------------------------------------------------------------------------

    void DumpError() {
        JSValue ex = JS_GetException(ctx);
        const char* s = JS_ToCString(ctx, ex);
        SDL_Log("JS error: %s", s ? s : "?");
        JS_FreeCString(ctx, s);
        JS_FreeValue(ctx, ex);
    }
    //-------------------------------------------------------------------------------------------------------

    JSContext* ctx = nullptr;
    JSRuntime* rt  = nullptr;

private:
    static JSModuleDef* ModuleLoader(JSContext* c, const char* name, void*) {
        std::ifstream f(name, std::ios::binary);
        if (!f) { JS_ThrowReferenceError(c, "modulo '%s' non trovato", name); return nullptr; }
        std::string src((std::istreambuf_iterator<char>(f)), {});
        JSValue v = JS_Eval(c, src.c_str(), src.size(), name,
                            JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(v)) return nullptr;
        auto* m = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(v));
        JS_FreeValue(c, v);
        return m;
    }
}; // class JsRuntime

#endif //YMODECS_JSRML_H
