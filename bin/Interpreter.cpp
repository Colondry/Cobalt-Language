// ============================================================================
// Cobalt tree-walking interpreter (`cobalt -run-experimental`).
//
// HOW THIS FILE IS ORGANIZED (read top to bottom, it's meant to be):
//   1. Value helpers          -- turning Values into numbers/strings/bools
//   2. Scopes (variables)     -- a stack of name -> Value maps
//   3. Struct/class instances -- how `T test = [10]` becomes an Object Value
//   4. Native library         -- C++ implementations of rand(), Stats.*,
//                                 str.*, Window.*, Chart.* (see below)
//   5. Expression dispatch    -- one small function per AST node kind
//   6. Statement dispatch     -- same idea, for statements
//   7. Interpreter() entry point
//
// WHY DISPATCH TABLES INSTEAD OF A GIANT if/dynamic_pointer_cast CHAIN:
//   The old version tested every possible node type in sequence for every
//   single expression/statement ("is it a NumberLit? no. a StringLit? no.
//   a NameExpr? no. ..."), which is slow (grows with the language) and a
//   pain to extend (a new node type means finding the right spot in a
//   50-branch if-chain). Here each node type is handled by ONE small
//   function, registered ONCE, and looked up by hash in O(1). Adding a new
//   node type is: write a small handler function, add one registration
//   line near the bottom of setupDispatch(). Nothing else changes.
//
// NATIVE LIBRARIES:
//   The interpreter cannot run the .cpp files under lib/ (they're meant to
//   be compiled by a real C++ compiler, not interpreted). So instead, the
//   handful of library calls a Cobalt program actually needs at runtime
//   (rand(), Stats.Mean(), Window.Init(), Chart.Line(), ...) are
//   reimplemented directly in this file as small C++ functions and
//   registered in `nativeFunctions` / `nativeSingletons`. To add support
//   for another library call, add ONE entry near setupNativeLibrary() --
//   nothing else in the interpreter needs to change.
//
//   Window/Chart optionally use real SDL3 if <SDL3/SDL.h> is visible to
//   this translation unit (auto-detected below with __has_include); if
//   it isn't, a headless fallback runs the same program logic and prints
//   what would have been drawn, so `-run-experimental` always builds and
//   runs even without SDL3 installed.
// ============================================================================

#include "Interpreter.hpp"
#include "value.hpp"
#include "runtime.hpp"
#include "state.hpp"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <limits>
#include <random>
#include <cmath>
#include <algorithm>
#include <functional>
#include <typeindex>
#include <memory>

#if defined(__has_include)
#  if __has_include(<SDL3/SDL.h>)
#    define COBALT_HAVE_SDL3 1
#    include <SDL3/SDL.h>
#  endif
#endif

// ============================================================================
// 1. Value helpers
// ============================================================================

static std::string toString(const Value& v);

static std::string unquote(const std::string& raw, char quoteChar) {
    std::string s = raw;
    if (s.size() >= 2 && s.front() == quoteChar && s.back() == quoteChar) {
        s = s.substr(1, s.size() - 2);
    }
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            switch (n) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case '0': out += '\0'; break;
            case '\\': out += '\\'; break;
            case '\'': out += '\''; break;
            case '"': out += '"'; break;
            default: out += n; break;
            }
            i++;
        }
        else {
            out += s[i];
        }
    }
    return out;
}

static bool truthy(const Value& v) {
    switch (v.type) {
    case ValueType::Bool:   return v.boolValue;
    case ValueType::Int:    return v.intValue != 0;
    case ValueType::Float:  return v.floatValue != 0;
    case ValueType::Double: return v.doubleValue != 0;
    case ValueType::String: return !v.stringValue.empty();
    case ValueType::List:   return v.listValue && !v.listValue->empty();
    default:                return false;
    }
}

static double asNumber(const Value& v) {
    switch (v.type) {
    case ValueType::Int:    return v.intValue;
    case ValueType::Float:  return v.floatValue;
    case ValueType::Double: return v.doubleValue;
    case ValueType::Byte:   return v.byteValue;
    case ValueType::Bool:   return v.boolValue ? 1 : 0;
    default:                return 0;
    }
}

static bool isFloatingType(ValueType t) {
    return t == ValueType::Float || t == ValueType::Double;
}

static Value makeInt(int i) { Value v; v.type = ValueType::Int; v.intValue = i; return v; }
static Value makeDouble(double d) { Value v; v.type = ValueType::Double; v.doubleValue = d; return v; }
static Value makeString(std::string s) { Value v; v.type = ValueType::String; v.stringValue = std::move(s); return v; }
static Value makeBool(bool b) { Value v; v.type = ValueType::Bool; v.boolValue = b; return v; }
static Value makeVoid() { return Value{}; }

static std::string toString(const Value& v) {
    switch (v.type) {
    case ValueType::Int:    return std::to_string(v.intValue);
    case ValueType::Float:  return std::to_string(v.floatValue);
    case ValueType::Double: return std::to_string(v.doubleValue);
    case ValueType::String: return v.stringValue;
    case ValueType::Bool:   return v.boolValue ? "true" : "false";
    case ValueType::Char:   return std::string(1, v.charValue);
    case ValueType::Byte:   return std::to_string((int)v.byteValue);
    case ValueType::List: {
        std::string out = "[";
        if (v.listValue) {
            for (size_t i = 0; i < v.listValue->size(); i++) {
                if (i) out += ", ";
                out += toString((*v.listValue)[i]);
            }
        }
        return out + "]";
    }
    case ValueType::Object: return "<" + v.typeName + ">";
    default: return std::string{};
    }
}

// ============================================================================
// 2. Scopes (variables)
// ============================================================================

static std::vector<std::unordered_map<std::string, Value>> scopes{ {} }; // scopes[0] == globals

static Value& lvalue(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return scopes.back()[name]; // not found anywhere: create in innermost scope
}
// Struct / class instances

struct TypeBlueprint {
    std::vector<std::string> fieldOrder;             // declaration order
    std::unordered_map<std::string, Value> defaults;  // field -> default value
};

static std::unordered_map<std::string, TypeBlueprint> structBlueprints; // struct name -> blueprint
static std::unordered_map<std::string, TypeBlueprint> classBlueprints;  // class name  -> blueprint
static std::unordered_map<std::string, std::unordered_map<std::string, const CFDecl*>> classMethods; // class -> method name -> body
static std::unordered_map<std::string, const FunctionDecl*> userFunctions; // top-level fn name -> body

static Value evaluateExpr(const ExprPtr& e);
static ExecResult execBlock(const std::vector<StmtPtr>& body);

static Value instantiateBlueprint(const std::string& typeName, const TypeBlueprint& bp) {
    Value obj;
    obj.type = ValueType::Object;
    obj.typeName = typeName;
    obj.objectValue = std::make_shared<std::unordered_map<std::string, Value>>(bp.defaults);
    return obj;
}

static Value instantiateFromListLit(const std::string& typeName, const TypeBlueprint& bp, const std::shared_ptr<ListLit>& lit) {
    Value obj = instantiateBlueprint(typeName, bp);
    for (size_t i = 0; i < bp.fieldOrder.size() && i < lit->items.size(); i++) {
        (*obj.objectValue)[bp.fieldOrder[i]] = evaluateExpr(lit->items[i]);
    }
    return obj;
}

// ============================================================================
// 4. Native library
// ============================================================================
//
// A native singleton (Window, Chart, Stats, ...) exposes methods and, for
// objects with public fields (Chart.Color, Chart.Title), field get/set
// callbacks. To add a new library call: add one line in setupNativeLibrary().

using NativeMethod = std::function<Value(std::vector<Value>&)>;
using NativeGetter = std::function<Value()>;
using NativeSetter = std::function<void(const Value&)>;

struct NativeSingleton {
    std::unordered_map<std::string, NativeMethod> methods;
    std::unordered_map<std::string, NativeGetter> getters;
    std::unordered_map<std::string, NativeSetter> setters;
};

static std::unordered_map<std::string, NativeSingleton> nativeSingletons; // "Window" -> ...
static std::unordered_map<std::string, NativeMethod> nativeFunctions;      // "rand" -> ...

static std::vector<Value> evalArgs(const std::vector<ExprPtr>& argExprs) {
    std::vector<Value> args;
    args.reserve(argExprs.size());
    for (auto& a : argExprs) args.push_back(evaluateExpr(a));
    return args;
}

// ---- rand(min, max) --------------------------------------------------
static Value native_rand(std::vector<Value>& args) {
    double lo = args.size() > 0 ? asNumber(args[0]) : 0;
    double hi = args.size() > 1 ? asNumber(args[1]) : 0;
    static std::mt19937 gen{ std::random_device{}() };
    std::uniform_int_distribution<long> dist((long)lo, (long)hi);
    return makeDouble((double)dist(gen)); // matches base.cpp's `long double rand(...)`
}

// ---- Stats.* -----------------------------------------------------------
static std::vector<double> toDoubleVec(const Value& listVal) {
    std::vector<double> out;
    if (listVal.type == ValueType::List && listVal.listValue) {
        out.reserve(listVal.listValue->size());
        for (auto& v : *listVal.listValue) out.push_back(asNumber(v));
    }
    return out;
}
static void registerStatsLibrary() {
    NativeSingleton stats;
    stats.methods["Mean"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        double sum = 0; for (double x : d) sum += x;
        return makeDouble(d.empty() ? 0 : sum / d.size());
        };
    stats.methods["Sum"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        double sum = 0; for (double x : d) sum += x;
        return makeDouble(sum);
        };
    stats.methods["Max"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        return makeDouble(d.empty() ? 0 : *std::max_element(d.begin(), d.end()));
        };
    stats.methods["Min"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        return makeDouble(d.empty() ? 0 : *std::min_element(d.begin(), d.end()));
        };
    stats.methods["Median"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        if (d.empty()) return makeDouble(0);
        std::sort(d.begin(), d.end());
        size_t n = d.size();
        return makeDouble(n % 2 == 0 ? (d[n / 2 - 1] + d[n / 2]) / 2 : d[n / 2]);
        };
    stats.methods["Variance"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        if (d.empty()) return makeDouble(0);
        double mean = 0; for (double x : d) mean += x; mean /= d.size();
        double sum = 0; for (double x : d) { double diff = x - mean; sum += diff * diff; }
        return makeDouble(sum / d.size());
        };
    stats.methods["StdDev"] = [](std::vector<Value>& a) -> Value {
        auto d = toDoubleVec(a[0]);
        if (d.empty()) return makeDouble(0);
        double mean = 0; for (double x : d) mean += x; mean /= d.size();
        double sum = 0; for (double x : d) { double diff = x - mean; sum += diff * diff; }
        return makeDouble(std::sqrt(sum / d.size()));
        };
    stats.methods["Shuffle"] = [](std::vector<Value>& a) -> Value {
        if (a[0].type == ValueType::List && a[0].listValue) {
            static std::mt19937 gen{ std::random_device{}() };
            std::shuffle(a[0].listValue->begin(), a[0].listValue->end(), gen);
        }
        return makeVoid();
        };
    nativeSingletons["Stats"] = std::move(stats);
}

// ---- str.* ---------------------------------------------------------------
static void registerStrLibrary() {
    NativeSingleton str;
    str.methods["len"] = [](std::vector<Value>& a) -> Value {
        return makeInt((int)toString(a[0]).size());
        };
    str.methods["upper"] = [](std::vector<Value>& a) -> Value {
        std::string s = toString(a[0]);
        for (char& c : s) c = (char)std::toupper((unsigned char)c);
        return makeString(s);
        };
    str.methods["lower"] = [](std::vector<Value>& a) -> Value {
        std::string s = toString(a[0]);
        for (char& c : s) c = (char)std::tolower((unsigned char)c);
        return makeString(s);
        };
    str.methods["startsWith"] = [](std::vector<Value>& a) -> Value {
        std::string word = toString(a[0]), start = toString(a[1]);
        return makeBool(word.size() >= start.size() && word.compare(0, start.size(), start) == 0);
        };
    str.methods["endWith"] = [](std::vector<Value>& a) -> Value {
        std::string word = toString(a[0]), end = toString(a[1]);
        return makeBool(end.size() <= word.size() && word.rfind(end) == word.size() - end.size());
        };
    nativeSingletons["str"] = std::move(str);
}

// SDL3?
struct RGB { int r = 255, g = 255, b = 255; };
static RGB hexToRgb(const std::string& hex) {
    RGB c;
    const char* s = (!hex.empty() && hex[0] == '#') ? hex.c_str() + 1 : hex.c_str();
    std::sscanf(s, "%02x%02x%02x", &c.r, &c.g, &c.b);
    return c;
}

#ifdef COBALT_HAVE_SDL3
namespace nativeWindow {
    static SDL_Window* window = nullptr;
    static SDL_Renderer* renderer = nullptr;
    static bool running = false;
}
static void registerWindowAndChartLibrary_SDL() {
    NativeSingleton win;
    win.methods["Init"] = [](std::vector<Value>& a) -> Value {
        using namespace nativeWindow;
        if (!SDL_Init(SDL_INIT_VIDEO)) { std::fprintf(stderr, "sdl: SDL_Init failed: %s\n", SDL_GetError()); return makeBool(false); }
        std::string title = a.size() > 0 ? toString(a[0]) : "Cobalt";
        int w = a.size() > 1 ? (int)asNumber(a[1]) : 800;
        int h = a.size() > 2 ? (int)asNumber(a[2]) : 600;
        window = SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_RESIZABLE);
        if (!window) { std::fprintf(stderr, "sdl: SDL_CreateWindow failed: %s\n", SDL_GetError()); SDL_Quit(); return makeBool(false); }
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) { std::fprintf(stderr, "sdl: SDL_CreateRenderer failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); window = nullptr; SDL_Quit(); return makeBool(false); }
        SDL_SetRenderVSync(renderer, 1);
        running = true;
        return makeBool(true);
        };
    win.methods["UpdateEvents"] = [](std::vector<Value>&) -> Value {
        using namespace nativeWindow;
        if (!running || !window) return makeBool(false);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) running = false;
        }
        return makeBool(running);
        };
    win.methods["BackColor"] = [](std::vector<Value>& a) -> Value {
        using namespace nativeWindow;
        if (!renderer) return makeVoid();
        RGB c = hexToRgb(a.size() > 0 ? toString(a[0]) : "#000000");
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderClear(renderer);
        return makeVoid();
        };
    win.methods["Show"] = [](std::vector<Value>&) -> Value {
        if (nativeWindow::renderer) SDL_RenderPresent(nativeWindow::renderer);
        return makeVoid();
        };
    win.methods["Close"] = [](std::vector<Value>&) -> Value {
        using namespace nativeWindow;
        if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
        if (window) { SDL_DestroyWindow(window); window = nullptr; }
        SDL_Quit();
        running = false;
        return makeVoid();
        };
    nativeSingletons["Window"] = std::move(win);

    auto chartColor = std::make_shared<std::string>("#FFFFFF");
    auto chartTitle = std::make_shared<std::string>("");

    NativeSingleton chart;
    chart.getters["Color"] = [chartColor] { return makeString(*chartColor); };
    chart.setters["Color"] = [chartColor](const Value& v) { *chartColor = toString(v); };
    chart.getters["Title"] = [chartTitle] { return makeString(*chartTitle); };
    chart.setters["Title"] = [chartTitle](const Value& v) { *chartTitle = toString(v); };
    chart.methods["Line"] = [chartColor, chartTitle](std::vector<Value>& a) -> Value {
        using namespace nativeWindow;
        if (!renderer || a.size() < 5) return makeVoid();
        int x = (int)asNumber(a[0]), y = (int)asNumber(a[1]);
        int width = (int)asNumber(a[2]), height = (int)asNumber(a[3]);
        auto data = toDoubleVec(a[4]);
        if (data.size() < 2) return makeVoid();
        double maxV = *std::max_element(data.begin(), data.end());
        if (maxV == 0) maxV = 1;
        float step = (float)width / (data.size() - 1);
        RGB c = hexToRgb(*chartColor);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        for (size_t i = 0; i + 1 < data.size(); i++) {
            float x1 = x + i * step, y1 = y + height - (float)(data[i] / maxV) * height;
            float x2 = x + (i + 1) * step, y2 = y + height - (float)(data[i + 1] / maxV) * height;
            SDL_RenderLine(renderer, x1, y1, x2, y2);
        }
        if (!chartTitle->empty()) SDL_RenderDebugText(renderer, (float)x, (float)(y - 30), chartTitle->c_str());
        return makeVoid();
        };
    nativeSingletons["Chart"] = std::move(chart);
}
#else
// ---- headless fallback: no SDL3 visible to this build --------------------
// Runs the exact same program logic, just prints instead of drawing, and
// caps the "event loop" at a handful of frames so `while Window.UpdateEvents()`
// terminates instead of running forever with no window to close.
static void registerWindowAndChartLibrary_Headless() {
    auto frame = std::make_shared<int>(0);
    const int headlessFrameLimit = 3;

    NativeSingleton win;
    win.methods["Init"] = [frame](std::vector<Value>& a) -> Value {
        std::string title = a.size() > 0 ? toString(a[0]) : "Cobalt";
        int w = a.size() > 1 ? (int)asNumber(a[1]) : 800, h = a.size() > 2 ? (int)asNumber(a[2]) : 600;
        std::cout << "[headless] Window.Init(\"" << title << "\", " << w << ", " << h << ") "
            << "-- no SDL3 found at compile time, running without a real window.\n";
        *frame = 0;
        return makeBool(true);
        };
    win.methods["UpdateEvents"] = [frame](std::vector<Value>&) -> Value {
        return makeBool((*frame)++ < headlessFrameLimit);
        };
    win.methods["BackColor"] = [](std::vector<Value>&) -> Value { return makeVoid(); };
    win.methods["Show"] = [frame](std::vector<Value>&) -> Value {
        std::cout << "[headless] frame " << *frame << " presented\n";
        return makeVoid();
        };
    win.methods["Close"] = [](std::vector<Value>&) -> Value {
        std::cout << "[headless] Window.Close()\n";
        return makeVoid();
        };
    nativeSingletons["Window"] = std::move(win);

    auto chartColor = std::make_shared<std::string>("#FFFFFF");
    auto chartTitle = std::make_shared<std::string>("");

    NativeSingleton chart;
    chart.getters["Color"] = [chartColor] { return makeString(*chartColor); };
    chart.setters["Color"] = [chartColor](const Value& v) { *chartColor = toString(v); };
    chart.getters["Title"] = [chartTitle] { return makeString(*chartTitle); };
    chart.setters["Title"] = [chartTitle](const Value& v) { *chartTitle = toString(v); };
    chart.methods["Line"] = [chartTitle](std::vector<Value>& a) -> Value {
        if (a.size() < 5) return makeVoid();
        auto data = toDoubleVec(a[4]);
        std::cout << "[headless] Chart.Line title=\"" << *chartTitle << "\" points=" << data.size();
        if (!data.empty()) {
            double mn = *std::min_element(data.begin(), data.end());
            double mx = *std::max_element(data.begin(), data.end());
            std::cout << " range=[" << mn << ", " << mx << "]";
        }
        std::cout << "\n";
        return makeVoid();
        };
    nativeSingletons["Chart"] = std::move(chart);
}
#endif

static void setupNativeLibrary() {
    nativeSingletons.clear();
    nativeFunctions.clear();
    nativeFunctions["rand"] = native_rand;
    registerStatsLibrary();
    registerStrLibrary();
#ifdef COBALT_HAVE_SDL3
    registerWindowAndChartLibrary_SDL();
#else
    registerWindowAndChartLibrary_Headless();
#endif
}

// Expression dispatch
using ExprHandler = std::function<Value(Expr*)>;
static std::unordered_map<std::type_index, ExprHandler> exprHandlers;

// Registers a handler for expression node type NodeT. Usage:
//   onExpr<NumberLit>([](NumberLit* n) -> Value { ... });
template <typename NodeT, typename Fn>
static void onExpr(Fn fn) {
    exprHandlers[std::type_index(typeid(NodeT))] = [fn](Expr* e) { return fn(static_cast<NodeT*>(e)); };
}

static Value evaluateExpr(const ExprPtr& e) {
    if (!e) return makeVoid();
    auto it = exprHandlers.find(std::type_index(typeid(*e)));
    if (it == exprHandlers.end()) {
        std::cerr << "interpreter error: no handler for this expression type.\n";
        std::exit(EXIT_FAILURE);
    }
    return it->second(e.get());
}

struct MemberTarget {
    bool isNative = false;
    std::string nativeName;   // e.g. "Chart"
    std::string field;
    std::shared_ptr<std::unordered_map<std::string, Value>> objectFields; // for Object values
};
static bool resolveMember(const ExprPtr& objectExpr, const std::string& field, MemberTarget& out) {
    if (auto name = std::dynamic_pointer_cast<NameExpr>(objectExpr)) {
        auto nativeIt = nativeSingletons.find(name->name);
        if (nativeIt != nativeSingletons.end() && nativeIt->second.getters.count(field)) {
            out.isNative = true;
            out.nativeName = name->name;
            out.field = field;
            return true;
        }
    }
    Value obj = evaluateExpr(objectExpr);
    if (obj.type == ValueType::Object && obj.objectValue) {
        out.isNative = false;
        out.field = field;
        out.objectFields = obj.objectValue;
        return true;
    }
    return false;
}

static Value callUserFunction(const FunctionDecl* fn, const std::vector<ExprPtr>& argExprs) {
    std::unordered_map<std::string, Value> locals;
    for (size_t i = 0; i < fn->params.size() && i < argExprs.size(); i++) {
        locals[fn->params[i].name] = evaluateExpr(argExprs[i]);
    }
    scopes.push_back(std::move(locals));
    ExecResult r = execBlock(fn->body);
    scopes.pop_back();
    return r.value;
}

static Value callUserMethod(const std::shared_ptr<std::unordered_map<std::string, Value>>& fields,
    const CFDecl* method, const std::vector<ExprPtr>& argExprs) {
    std::unordered_map<std::string, Value> locals;
    for (size_t i = 0; i < method->params.size() && i < argExprs.size(); i++) {
        locals[method->params[i].name] = evaluateExpr(argExprs[i]);
    }
    for (auto& kv : *fields) {
        if (!locals.count(kv.first)) locals[kv.first] = kv.second;
    }
    scopes.push_back(std::move(locals));
    ExecResult r = execBlock(method->body);
    for (auto& kv : *fields) {
        auto found = scopes.back().find(kv.first);
        if (found != scopes.back().end()) kv.second = found->second;
    }
    scopes.pop_back();
    return r.value;
}

static void setupExprDispatch() {
    onExpr<NumberLit>([](NumberLit* n) -> Value {
        if (n->value.find('.') != std::string::npos) return makeDouble(std::stod(n->value));
        return makeInt(std::stoi(n->value));
        });
    onExpr<StringLit>([](StringLit* s) -> Value { return makeString(unquote(s->value, '"')); });
    onExpr<CharLit>([](CharLit* c) -> Value {
        Value v; v.type = ValueType::Char;
        std::string unq = unquote(c->value, '\'');
        v.charValue = unq.empty() ? '\0' : unq[0];
        return v;
        });
    onExpr<NameExpr>([](NameExpr* id) -> Value { return lvalue(id->name); });

    onExpr<UnaryExpr>([](UnaryExpr* u) -> Value {
        Value operand = evaluateExpr(u->operand);
        if (u->op == "-") {
            return isFloatingType(operand.type) ? makeDouble(-asNumber(operand)) : makeInt(-(int)asNumber(operand));
        }
        if (u->op == "!") return makeBool(!truthy(operand));
        return makeVoid();
        });

    onExpr<BinaryExpr>([](BinaryExpr* b) -> Value {
        Value lhs = evaluateExpr(b->lhs);
        Value rhs = evaluateExpr(b->rhs);
        const std::string& op = b->op;

        if (op == "+") {
            if (lhs.type == ValueType::String || rhs.type == ValueType::String) return makeString(toString(lhs) + toString(rhs));
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) return makeDouble(asNumber(lhs) + asNumber(rhs));
            return makeInt(lhs.intValue + rhs.intValue);
        }
        if (op == "-") {
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) return makeDouble(asNumber(lhs) - asNumber(rhs));
            return makeInt(lhs.intValue - rhs.intValue);
        }
        if (op == "*") {
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) return makeDouble(asNumber(lhs) * asNumber(rhs));
            return makeInt(lhs.intValue * rhs.intValue);
        }
        if (op == "/") {
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) return makeDouble(asNumber(lhs) / asNumber(rhs));
            return makeInt(lhs.intValue / rhs.intValue);
        }
        if (op == "%") return makeInt(lhs.intValue % rhs.intValue);
        if (op == "==") return makeBool(asNumber(lhs) == asNumber(rhs) && toString(lhs) == toString(rhs));
        if (op == "!=") return makeBool(!(asNumber(lhs) == asNumber(rhs) && toString(lhs) == toString(rhs)));
        if (op == "<")  return makeBool(asNumber(lhs) < asNumber(rhs));
        if (op == "<=") return makeBool(asNumber(lhs) <= asNumber(rhs));
        if (op == ">")  return makeBool(asNumber(lhs) > asNumber(rhs));
        if (op == ">=") return makeBool(asNumber(lhs) >= asNumber(rhs));
        if (op == "&&") return makeBool(truthy(lhs) && truthy(rhs));
        if (op == "||") return makeBool(truthy(lhs) || truthy(rhs));
        return makeVoid();
        });

    onExpr<IndexExpr>([](IndexExpr* idx) -> Value {
        Value base = evaluateExpr(idx->base);
        int i = (int)asNumber(evaluateExpr(idx->index));
        if (base.type == ValueType::List && base.listValue && i >= 0 && (size_t)i < base.listValue->size()) {
            return (*base.listValue)[i];
        }
        return makeVoid();
        });

    onExpr<PostIncExpr>([](PostIncExpr* pi) -> Value { Value& t = lvalue(pi->name); t.intValue++; return t; });
    onExpr<PostMinExpr>([](PostMinExpr* pm) -> Value { Value& t = lvalue(pm->name); t.intValue--; return t; });

    onExpr<ListLit>([](ListLit* lit) -> Value {
        Value v;
        v.type = ValueType::List;
        v.listValue = std::make_shared<std::vector<Value>>();
        v.listValue->reserve(lit->items.size());
        for (auto& item : lit->items) v.listValue->push_back(evaluateExpr(item));
        return v;
        });

    onExpr<FracLit>([](FracLit* fl) -> Value {
        // Fractions aren't backed by a dedicated Value kind here; represent
        // as "num/denom" text, matching how they'd print.
        std::string out;
        for (size_t i = 0; i < fl->items.size(); i++) {
            if (i) out += "/";
            out += toString(evaluateExpr(fl->items[i]));
        }
        return makeString(out);
        });

    onExpr<CallExpr>([](CallExpr* call) -> Value {
        auto ufIt = userFunctions.find(call->callee);
        if (ufIt != userFunctions.end()) {
            return callUserFunction(ufIt->second, call->args);
        }
        auto nfIt = nativeFunctions.find(call->callee);
        if (nfIt != nativeFunctions.end()) {
            std::vector<Value> args = evalArgs(call->args);
            return nfIt->second(args);
        }
        std::cerr << "interpreter error: unknown function '" << call->callee << "'.\n";
        std::exit(EXIT_FAILURE);
        });

    onExpr<ConcatExpr>([](ConcatExpr* c) -> Value {
        std::string res;
        for (auto& piece : c->pieces) res += toString(evaluateExpr(piece));
        return makeString(res);
        });

    onExpr<MemberExpr>([](MemberExpr* m) -> Value {
        MemberTarget target;
        if (!resolveMember(m->object, m->member, target)) {
            std::cerr << "interpreter error: unknown field '" << m->member << "'.\n";
            std::exit(EXIT_FAILURE);
        }
        if (target.isNative) return nativeSingletons[target.nativeName].getters[target.field]();
        return (*target.objectFields)[target.field];
        });

    onExpr<MethodCallExpr>([](MethodCallExpr* mc) -> Value {
        // Native singleton? (Window.Init(...), Chart.Line(...), Stats.Mean(...))
        if (auto name = std::dynamic_pointer_cast<NameExpr>(mc->object)) {
            auto nativeIt = nativeSingletons.find(name->name);
            if (nativeIt != nativeSingletons.end()) {
                auto methodIt = nativeIt->second.methods.find(mc->method);
                if (methodIt != nativeIt->second.methods.end()) {
                    std::vector<Value> args = evalArgs(mc->args);
                    return methodIt->second(args);
                }
            }
        }
        // User-defined class instance method.
        Value obj = evaluateExpr(mc->object);
        if (obj.type == ValueType::Object && obj.objectValue) {
            auto clsIt = classMethods.find(obj.typeName);
            if (clsIt != classMethods.end()) {
                auto methodIt = clsIt->second.find(mc->method);
                if (methodIt != clsIt->second.end()) {
                    return callUserMethod(obj.objectValue, methodIt->second, mc->args);
                }
            }
        }
        std::cerr << "interpreter error: unknown method '" << mc->method << "'.\n";
        std::exit(EXIT_FAILURE);
        });
}


// Statement dispatch

using StmtHandler = std::function<ExecResult(Stmt*)>;
static std::unordered_map<std::type_index, StmtHandler> stmtHandlers;

template <typename NodeT, typename Fn>
static void onStmt(Fn fn) {
    stmtHandlers[std::type_index(typeid(NodeT))] = [fn](Stmt* s) { return fn(static_cast<NodeT*>(s)); };
}

static ExecResult normal() { return {}; }

static ExecResult execStmt(const StmtPtr& stmt) {
    auto it = stmtHandlers.find(std::type_index(typeid(*stmt)));
    if (it == stmtHandlers.end()) {
        std::cerr << "interpreter error: no handler for this statement type.\n";
        std::exit(EXIT_FAILURE);
    }
    return it->second(stmt.get());
}

static ExecResult execBlock(const std::vector<StmtPtr>& body) {
    bool chainMatched = false;

    for (auto& stmt : body) {
        if (auto i = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            chainMatched = truthy(evaluateExpr(i->condition));
            if (chainMatched) {
                ExecResult r = execBlock(i->body);
                if (r.state != ExecState::Normal) return r;
            }
            continue;
        }
        if (auto ei = std::dynamic_pointer_cast<ElifStmt>(stmt)) {
            if (!chainMatched && truthy(evaluateExpr(ei->condition))) {
                chainMatched = true;
                ExecResult r = execBlock(ei->body);
                if (r.state != ExecState::Normal) return r;
            }
            continue;
        }
        if (auto el = std::dynamic_pointer_cast<ElseStmt>(stmt)) {
            if (!chainMatched) {
                ExecResult r = execBlock(el->body);
                if (r.state != ExecState::Normal) return r;
            }
            chainMatched = false;
            continue;
        }

        chainMatched = false;
        ExecResult r = execStmt(stmt);
        if (r.state != ExecState::Normal) return r;
    }
    return {};
}

static std::string unmangleTypeName(const std::string& type) {
    if (type.size() > 4 && type.rfind("__", 0) == 0 && type.compare(type.size() - 2, 2, "__") == 0) {
        return type.substr(2, type.size() - 4);
    }
    return type;
}

static ExecResult execVarDecl(VarDecl* decl) {
    std::string typeName = unmangleTypeName(decl->type);
    if (auto bpIt = structBlueprints.find(typeName); bpIt != structBlueprints.end()) {
        if (auto lit = std::dynamic_pointer_cast<ListLit>(decl->init)) {
            lvalue(decl->name) = instantiateFromListLit(typeName, bpIt->second, lit);
        }
        else {
            lvalue(decl->name) = instantiateBlueprint(typeName, bpIt->second);
        }
        return normal();
    }
    if (auto bpIt = classBlueprints.find(typeName); bpIt != classBlueprints.end()) {
        if (auto lit = std::dynamic_pointer_cast<ListLit>(decl->init)) {
            lvalue(decl->name) = instantiateFromListLit(typeName, bpIt->second, lit);
        }
        else {
            lvalue(decl->name) = instantiateBlueprint(typeName, bpIt->second);
        }
        return normal();
    }
    // Plain value (int/string/List<T>/...)
    lvalue(decl->name) = decl->init ? evaluateExpr(decl->init) : Value{};
    return normal();
}

static void assignToLvalue(const ExprPtr& target, const Value& value) {
    if (auto name = std::dynamic_pointer_cast<NameExpr>(target)) {
        lvalue(name->name) = value;
        return;
    }
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(target)) {
        MemberTarget mt;
        if (resolveMember(m->object, m->member, mt)) {
            if (mt.isNative) nativeSingletons[mt.nativeName].setters[mt.field](value);
            else (*mt.objectFields)[mt.field] = value;
        }
        return;
    }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(target)) {
        Value base = evaluateExpr(idx->base); // shares storage via listValue
        int i = (int)asNumber(evaluateExpr(idx->index));
        if (base.type == ValueType::List && base.listValue && i >= 0 && (size_t)i < base.listValue->size()) {
            (*base.listValue)[i] = value;
        }
    }
}

static void setupStmtDispatch() {
    onStmt<VarDecl>([](VarDecl* decl) { return execVarDecl(decl); });

    onStmt<AssignStmt>([](AssignStmt* a) { lvalue(a->name) = evaluateExpr(a->value); return normal(); });

    onStmt<ExprAssignStmt>([](ExprAssignStmt* ea) {
        assignToLvalue(ea->target, evaluateExpr(ea->value));
        return normal();
        });

    onStmt<ReturnStmt>([](ReturnStmt* r) {
        ExecResult result;
        result.state = ExecState::Return;
        if (r->value) result.value = evaluateExpr(r->value);
        return result;
        });

    onStmt<PrintCode>([](PrintCode* p) {
        if (p->value) std::cout << toString(evaluateExpr(p->value));
        if (p->newline) std::cout << '\n';
        return normal();
        });

    onStmt<PrintMacCode>([](PrintMacCode* p) {
        auto c = std::dynamic_pointer_cast<ConcatExpr>(p->value);
        if (c && !c->pieces.empty()) {
            auto fmtLit = std::dynamic_pointer_cast<StringLit>(c->pieces[0]);
            if (fmtLit) {
                std::string raw = unquote(fmtLit->value, '"');
                size_t argIdx = 1;
                for (size_t i = 0; i < raw.size(); i++) {
                    if (raw[i] == '{' && i + 1 < raw.size() && raw[i + 1] == '}') {
                        if (argIdx < c->pieces.size()) std::cout << toString(evaluateExpr(c->pieces[argIdx++]));
                        i++;
                    }
                    else {
                        std::cout << raw[i];
                    }
                }
            }
        }
        else if (p->value) {
            std::cout << toString(evaluateExpr(p->value));
        }
        if (p->newline) std::cout << '\n';
        return normal();
        });

    onStmt<ReadCode>([](ReadCode* in) {
        if (in->prompt) std::cout << toString(evaluateExpr(in->prompt));
        Value v; v.type = ValueType::String;
        std::cin >> v.stringValue;
        assignToLvalue(in->target, v);
        return normal();
        });

    onStmt<ReadLine>([](ReadLine* inStr) {
        if (inStr->prompt) std::cout << toString(evaluateExpr(inStr->prompt));
        Value v; v.type = ValueType::String;
        std::getline(std::cin, v.stringValue);
        assignToLvalue(inStr->target, v);
        return normal();
        });

    onStmt<ExprStmt>([](ExprStmt* es) { evaluateExpr(es->expr); return normal(); });

    onStmt<WhileStmt>([](WhileStmt* w) {
        while (truthy(evaluateExpr(w->condition))) {
            ExecResult r = execBlock(w->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
        }
        return normal();
        });

    onStmt<ForRangeStmt>([](ForRangeStmt* f) {
        Value from = evaluateExpr(f->from);
        Value to = evaluateExpr(f->to);
        Value& loopVar = lvalue(f->varName);
        loopVar = from;
        while (loopVar.intValue <= to.intValue) {
            ExecResult r = execBlock(f->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
            loopVar.intValue++;
        }
        return normal();
        });

    onStmt<ContinueStmt>([](ContinueStmt*) { ExecResult r; r.state = ExecState::Continue; return r; });
    onStmt<BreakStmt>([](BreakStmt*) { ExecResult r; r.state = ExecState::Break; return r; });

    onStmt<LambFuncDecl>([](LambFuncDecl* lf) {
        static std::vector<std::unique_ptr<FunctionDecl>> storage;
        auto fn = std::make_unique<FunctionDecl>();
        fn->name = lf->name; fn->params = lf->params; fn->returnType = lf->returnType; fn->body = lf->body;
        userFunctions[lf->name] = fn.get();
        storage.push_back(std::move(fn));
        return normal();
        });
    onStmt<CFuncDecl>([](CFuncDecl* cf) {
        static std::vector<std::unique_ptr<FunctionDecl>> storage;
        auto fn = std::make_unique<FunctionDecl>();
        fn->name = cf->name; fn->params = cf->params; fn->returnType = cf->returnType; fn->body = cf->body;
        userFunctions[cf->name] = fn.get();
        storage.push_back(std::move(fn));
        return normal();
        });

    onStmt<ClearStmt>([](ClearStmt*) {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
        return normal();
        });

    onStmt<RepeatCode>([](RepeatCode* rp) {
        int count = (int)asNumber(evaluateExpr(rp->value));
        for (int i = 0; i < count; i++) {
            ExecResult r = execBlock(rp->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
        }
        return normal();
        });

    onStmt<ForeverCode>([](ForeverCode* fr) {
        while (true) {
            ExecResult r = execBlock(fr->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
        }
        return normal();
        });
}

static TypeBlueprint buildBlueprint(std::vector<StmtPtr>& body) {
    TypeBlueprint bp;
    for (auto& s : body) {
        if (auto v = std::dynamic_pointer_cast<VarDecl>(s)) {
            bp.fieldOrder.push_back(v->name);
            bp.defaults[v->name] = v->init ? evaluateExpr(v->init) : Value{};
        }
    }
    return bp;
}

static void setupProgram(Program& program) {
    for (auto& fn : program.functions) userFunctions[fn.name] = &fn;

    for (auto& cls : program.classes) {
        TypeBlueprint bp = buildBlueprint(cls.publicBody);
        TypeBlueprint privateBp = buildBlueprint(cls.privateBody);
        bp.fieldOrder.insert(bp.fieldOrder.end(), privateBp.fieldOrder.begin(), privateBp.fieldOrder.end());
        bp.defaults.insert(privateBp.defaults.begin(), privateBp.defaults.end());
        classBlueprints[cls.name] = std::move(bp);

        auto& methods = classMethods[cls.name];
        auto registerMethods = [&](std::vector<StmtPtr>& body) {
            for (auto& s : body) {
                if (auto m = std::dynamic_pointer_cast<CFDecl>(s)) methods[m->name] = m.get();
            }
            };
        registerMethods(cls.publicBody);
        registerMethods(cls.privateBody);
    }

    for (auto& str : program.struc) {
        structBlueprints[str.name] = buildBlueprint(str.body);
    }
}

void Interpreter(Program& program) {
    scopes.assign(1, {});
    structBlueprints.clear();
    classBlueprints.clear();
    classMethods.clear();
    userFunctions.clear();

    setupExprDispatch();
    setupStmtDispatch();
    setupNativeLibrary();

    setupProgram(program);

    auto it = userFunctions.find("main");
    if (it != userFunctions.end()) {
        execBlock(it->second->body);
    }
    else {
        std::cerr << "interpreter error: no 'main' function found.\n";
    }
}