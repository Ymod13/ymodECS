//
// Created by ymod1 on 09/10/2026.
//

#include "JsRml.h"
#include <fstream>
#include <iterator>
#include <unordered_set>
#include <SDL3/SDL.h>

namespace {

    Rml::Context* g_rml = nullptr;
    JSClassID g_element_class = 0;

    // ---------- Utilities ----------

    void DumpException(JSContext* c) {
        JSValue ex = JS_GetException(c);
        const char* s = JS_ToCString(c, ex);
        SDL_Log("JS error: %s", s ? s : "?");
        JS_FreeCString(c, s);
        JSValue stack = JS_GetPropertyStr(c, ex, "stack");
        if (!JS_IsUndefined(stack)) {
            const char* st = JS_ToCString(c, stack);
            if (st) { SDL_Log("%s", st); JS_FreeCString(c, st); }
        }
        JS_FreeValue(c, stack);
        JS_FreeValue(c, ex);
    }
    //-------------------------------------------------------------------------------------------------------

    struct Str {
        JSContext* c; const char* s;
        Str(JSContext* c, JSValueConst v) : c(c), s(JS_ToCString(c, v)) {}
        ~Str() {
            if (s) {
                JS_FreeCString(c, s);
            }
        }
    };
    //-------------------------------------------------------------------------------------------------------


    // ---------- Element wrapper ----------
    // if RmlUi destroys the element, the pointer becomes null instead of being dangling in JS
    struct ElementHandle
    {
        Rml::ObserverPtr<Rml::Element> el;
    };
    //-------------------------------------------------------------------------------------------------------

    void ElementFinalizer(JSRuntime*, JSValue v) {
        delete static_cast<ElementHandle*>(JS_GetOpaque(v, g_element_class));
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue WrapElement(JSContext* c, Rml::Element* el) {
        if (!el) return JS_NULL;
        JSValue obj = JS_NewObjectClass(c, g_element_class);
        if (JS_IsException(obj)) return obj;
        JS_SetOpaque(obj, new ElementHandle{ el->GetObserverPtr() });
        return obj;
    }
    //-------------------------------------------------------------------------------------------------------

    Rml::Element* GetEl(JSContext* c, JSValueConst thiz) {
        auto* h = static_cast<ElementHandle*>(JS_GetOpaque2(c, thiz, g_element_class));
        if (!h) return nullptr;                       // eccezione già lanciata
        if (!h->el) { JS_ThrowReferenceError(c, "elemento RmlUi non più valido"); return nullptr; }
        return h->el.get();
    }
    //-------------------------------------------------------------------------------------------------------


    // ---------- events listener ----------
    JSValue WrapEvent(JSContext* c, Rml::Event& ev) {
        JSValue o = JS_NewObject(c);
        JS_SetPropertyStr(c, o, "type", JS_NewString(c, ev.GetType().c_str()));
        JS_SetPropertyStr(c, o, "target", WrapElement(c, ev.GetTargetElement()));
        JSValue params = JS_NewObject(c);
        for (auto& [key, var] : ev.GetParameters()) {
            switch (var.GetType()) {
                case Rml::Variant::INT: case Rml::Variant::FLOAT: case Rml::Variant::DOUBLE:
                case Rml::Variant::UINT: case Rml::Variant::INT64:
                    JS_SetPropertyStr(c, params, key.c_str(), JS_NewFloat64(c, var.Get<double>()));
                    break;
                case Rml::Variant::BOOL:
                    JS_SetPropertyStr(c, params, key.c_str(), JS_NewBool(c, var.Get<bool>()));
                    break;
                case Rml::Variant::STRING:
                    JS_SetPropertyStr(c, params, key.c_str(), JS_NewString(c, var.Get<Rml::String>().c_str()));
                    break;
                default: break;   // vettori, puntatori, ecc.: ignorati
            }
        }
        JS_SetPropertyStr(c, o, "params", params);
        return o;
    }
    //-------------------------------------------------------------------------------------------------------

    class JsEventListener;
    std::unordered_set<JsEventListener*> g_listeners;
    //-------------------------------------------------------------------------------------------------------

    class JsEventListener : public Rml::EventListener
    {
        public:
        JsEventListener(JSContext* c, JSValueConst f) : ctx(c), fn(JS_DupValue(c, f)) { g_listeners.insert(this); }
        ~JsEventListener() override { g_listeners.erase(this); ReleaseJs(); }

        void ReleaseJs() {                 // idempotent
            if (!ctx) return;
            JS_FreeValue(ctx, fn);
            fn  = JS_UNDEFINED;
            ctx = nullptr;
        }
        //-------------------------------------------------------------------------------------------------------

        void ProcessEvent(Rml::Event& ev) override {
            if (!ctx) return;              // dopo lo shutdown non fare nulla
            JSValue arg  = WrapEvent(ctx, ev);
            JSValue self = WrapElement(ctx, ev.GetCurrentElement());
            JSValue r = JS_Call(ctx, fn, self, 1, &arg);
            if (JS_IsException(r)) DumpException(ctx);
            JS_FreeValue(ctx, r); JS_FreeValue(ctx, self); JS_FreeValue(ctx, arg);
        }
        //-------------------------------------------------------------------------------------------------------

        void OnDetach(Rml::Element*) override {
            delete this;
        }
        //-------------------------------------------------------------------------------------------------------

        private:
            JSContext* ctx;
            JSValue fn;
    };
    //-------------------------------------------------------------------------------------------------------

    // onclick="..." or similar in Rml
    class JsEventListenerInstancer : public Rml::EventListenerInstancer
    {
    public:
        JSContext* ctx = nullptr;
        Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element*) override {
            if (!ctx) return nullptr;
            const std::string code = "(function(event){" + value + "\n})";
            JSValue fn = JS_Eval(ctx, code.c_str(), code.size(), "<rml-attr>", JS_EVAL_TYPE_GLOBAL);
            if (JS_IsException(fn)) { DumpException(ctx); return nullptr; }
            auto* l = new JsEventListener(ctx, fn);
            JS_FreeValue(ctx, fn);
            return l;
        }
    };
    JsEventListenerInstancer g_instancer;
    //-------------------------------------------------------------------------------------------------------


    // ---------- Element methods ----------
    JSValue el_getAttribute(JSContext* c, JSValueConst t, int, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str n(c, a[0]); if (!n.s) return JS_EXCEPTION;
        Rml::Variant* v = e->GetAttribute(n.s);
        return v ? JS_NewString(c, v->Get<Rml::String>().c_str()) : JS_NULL;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_setAttribute(JSContext* c, JSValueConst t, int, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str n(c, a[0]), v(c, a[1]); if (!n.s || !v.s) return JS_EXCEPTION;
        e->SetAttribute(n.s, Rml::String(v.s));
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_setProperty(JSContext* c, JSValueConst t, int, JSValueConst* a) {   // stile RCSS
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str n(c, a[0]), v(c, a[1]); if (!n.s || !v.s) return JS_EXCEPTION;
        return JS_NewBool(c, e->SetProperty(n.s, v.s));
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_setInnerRML(JSContext* c, JSValueConst t, int, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str s(c, a[0]); if (!s.s) return JS_EXCEPTION;
        e->SetInnerRML(s.s);
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_getInnerRML(JSContext* c, JSValueConst t, int, JSValueConst*) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        return JS_NewString(c, e->GetInnerRML().c_str());
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_addClass(JSContext* c, JSValueConst t, int, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str s(c, a[0]); if (!s.s) return JS_EXCEPTION;
        e->SetClass(s.s, true);
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_removeClass(JSContext* c, JSValueConst t, int, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        Str s(c, a[0]); if (!s.s) return JS_EXCEPTION;
        e->SetClass(s.s, false);
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue el_addEventListener(JSContext* c, JSValueConst t, int argc, JSValueConst* a) {
        Rml::Element* e = GetEl(c, t); if (!e) return JS_EXCEPTION;
        if (argc < 2 || !JS_IsFunction(c, a[1])) return JS_ThrowTypeError(c, "addEventListener(type, fn)");
        Str type(c, a[0]); if (!type.s) return JS_EXCEPTION;
        e->AddEventListener(type.s, new JsEventListener(c, a[1]));   // si cancella da solo in OnDetach
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

    const JSCFunctionListEntry kElementFuncs[] = {
        JS_CFUNC_DEF("getAttribute",     1, el_getAttribute),
        JS_CFUNC_DEF("setAttribute",     2, el_setAttribute),
        JS_CFUNC_DEF("setProperty",      2, el_setProperty),
        JS_CFUNC_DEF("setInnerRML",      1, el_setInnerRML),
        JS_CFUNC_DEF("getInnerRML",      0, el_getInnerRML),
        JS_CFUNC_DEF("addClass",         1, el_addClass),
        JS_CFUNC_DEF("removeClass",      1, el_removeClass),
        JS_CFUNC_DEF("addEventListener", 2, el_addEventListener),
    };
    //-------------------------------------------------------------------------------------------------------

    // ---------- document / console ----------
    JSValue doc_getElementById(JSContext* c, JSValueConst, int argc, JSValueConst* a) {
        if (!g_rml)    return JS_ThrowReferenceError(c, "JsRml: Rml::Context non registrato (Register non chiamato o Shutdown già eseguito)");
        if (argc < 1)  return JS_ThrowTypeError(c, "getElementById(id) richiede un argomento");
        Str id(c, a[0]); if (!id.s) return JS_EXCEPTION;
        for (int i = 0; i < g_rml->GetNumDocuments(); ++i)
            if (Rml::Element* e = g_rml->GetDocument(i)->GetElementById(id.s))
                return WrapElement(c, e);
        return JS_NULL;
    }
    //-------------------------------------------------------------------------------------------------------

    JSValue console_log(JSContext* c, JSValueConst, int argc, JSValueConst* a) {
        std::string line;
        for (int i = 0; i < argc; ++i) {
            Str s(c, a[i]);
            if (i) line += ' ';
            line += s.s ? s.s : "?";
        }
        SDL_Log("[js] %s", line.c_str());
        return JS_UNDEFINED;
    }
    //-------------------------------------------------------------------------------------------------------

} // namespace

namespace JsRml
{
    void Register(JSContext* c, Rml::Context* rml) {
        g_rml = rml;

        JS_NewClassID(JS_GetRuntime(c), &g_element_class);   // quickjs-ng: JS_NewClassID(JS_GetRuntime(c), &g_element_class)
        JSClassDef def{};
        def.class_name = "Element";
        def.finalizer  = ElementFinalizer;
        JS_NewClass(JS_GetRuntime(c), g_element_class, &def);

        JSValue proto = JS_NewObject(c);
        JS_SetPropertyFunctionList(c, proto, kElementFuncs, std::size(kElementFuncs));
        JS_SetClassProto(c, g_element_class, proto);

        JSValue global = JS_GetGlobalObject(c);

        JSValue document = JS_NewObject(c);
        JS_SetPropertyStr(c, document, "getElementById", JS_NewCFunction(c, doc_getElementById, "getElementById", 1));
        JS_SetPropertyStr(c, global, "document", document);

        JSValue console = JS_NewObject(c);
        JS_SetPropertyStr(c, console, "log", JS_NewCFunction(c, console_log, "log", 1));
        JS_SetPropertyStr(c, global, "console", console);

        JS_FreeValue(c, global);

        g_instancer.ctx = c;
        Rml::Factory::RegisterEventListenerInstancer(&g_instancer);
    }
    //-------------------------------------------------------------------------------------------------------

    bool RunFile(JSContext* c, const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            SDL_Log("JS: file non trovato: %s", path.c_str()); return false;
        }
        std::string src((std::istreambuf_iterator<char>(f)), {});
        JSValue r = JS_Eval(c, src.c_str(), src.size(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
        const bool ok = !JS_IsException(r);
        if (!ok)
        {
            DumpException(c);
        }

        JS_FreeValue(c, r);
        return ok;
    }
    //-------------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        g_instancer.ctx = nullptr;
        for (auto* l : g_listeners) l->ReleaseJs();   // non modifica il set
        g_rml = nullptr;
    }
    //-------------------------------------------------------------------------------------------------------

} // namespace JsRml