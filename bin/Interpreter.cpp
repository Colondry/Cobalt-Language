#include "Interpreter.hpp"
#include "value.hpp"
#include "runtime.hpp"
#include "state.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <vector>
#include <limits>

static std::vector<std::map<std::string, Value>> scopes{ {} }; // scopes[0] == globals

static Value& lvalue(const std::string& name) {
    // Search from innermost to outermost scope for an existing binding.
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    // Not found anywhere: create it in the innermost scope.
    return scopes.back()[name];
}

// Instance fields for class objects
static std::map<std::string, std::map<std::string, Value>> classInstances;

static std::map<std::string, std::map<std::string, Value>> structInstances;


// class name -> (method name -> method AST)
static std::map<std::string, std::map<std::string, const CFDecl*>> classMethods;
// top-level user functions, by name
static std::map<std::string, const FunctionDecl*> userFunctions;

static std::string toString(const Value& v);
static Value evaluateExpr(const ExprPtr& e);
static ExecResult execStmt(const StmtPtr& stmt);
static ExecResult execBlock(const std::vector<StmtPtr>& body);

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

static Value callUserFunction(const FunctionDecl* fn, const std::vector<ExprPtr>& argExprs) {
    std::map<std::string, Value> locals;
    for (size_t i = 0; i < fn->params.size() && i < argExprs.size(); i++) {
        locals[fn->params[i].name] = evaluateExpr(argExprs[i]);
    }
    scopes.push_back(std::move(locals));
    ExecResult r = execBlock(fn->body);
    scopes.pop_back();
    return r.value;
}

static Value callMethod(const std::string& objectName, const CFDecl* method, const std::vector<ExprPtr>& argExprs) {
    std::map<std::string, Value> locals;
    for (size_t i = 0; i < method->params.size() && i < argExprs.size(); i++) {
        locals[method->params[i].name] = evaluateExpr(argExprs[i]);
    }

    auto& fields = classInstances[objectName];
    for (auto& kv : fields) {
        if (!locals.count(kv.first)) locals[kv.first] = kv.second;
    }
    scopes.push_back(std::move(locals));
    ExecResult r = execBlock(method->body);
    for (auto& kv : fields) {
        auto found = scopes.back().find(kv.first);
        if (found != scopes.back().end()) kv.second = found->second;
    }
    scopes.pop_back();
    return r.value;
}

static Value evaluateExpr(const ExprPtr& e) {
    Value result;
    result.type = ValueType::String;
    if (!e) return result;

    if (auto n = std::dynamic_pointer_cast<NumberLit>(e)) {
        Value v;
        if (n->value.find('.') != std::string::npos) {
            v.type = ValueType::Double;
            v.doubleValue = std::stod(n->value);
        }
        else {
            v.type = ValueType::Int;
            v.intValue = std::stoi(n->value);
        }
        return v;
    }
    if (auto s = std::dynamic_pointer_cast<StringLit>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = unquote(s->value, '"');
        return v;
    }
    if (auto c = std::dynamic_pointer_cast<CharLit>(e)) {
        Value v;
        v.type = ValueType::Char;
        std::string unq = unquote(c->value, '\'');
        v.charValue = unq.empty() ? '\0' : unq[0];
        return v;
    }
    if (auto id = std::dynamic_pointer_cast<NameExpr>(e)) {
        return lvalue(id->name);
    }
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) {
        Value operand = evaluateExpr(u->operand);
        Value v;
        if (u->op == "-") {
            if (isFloatingType(operand.type)) {
                v.type = ValueType::Double;
                v.doubleValue = -asNumber(operand);
            }
            else {
                v.type = ValueType::Int;
                v.intValue = -(int)asNumber(operand);
            }
        }
        else if (u->op == "!") {
            v.type = ValueType::Bool;
            v.boolValue = !truthy(operand);
        }
        return v;
    }
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) {
        Value lhs = evaluateExpr(b->lhs);
        Value rhs = evaluateExpr(b->rhs);

        Value v;

        if (b->op == "+") {
            if (lhs.type == ValueType::String || rhs.type == ValueType::String) {
                v.type = ValueType::String;
                v.stringValue = toString(lhs) + toString(rhs);
            }
            else if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) {
                v.type = ValueType::Double;
                v.doubleValue = asNumber(lhs) + asNumber(rhs);
            }
            else {
                v.type = ValueType::Int;
                v.intValue = lhs.intValue + rhs.intValue;
            }
        }
        else if (b->op == "*") {
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) {
                v.type = ValueType::Double;
                v.doubleValue = asNumber(lhs) * asNumber(rhs);
            }
            else {
                v.type = ValueType::Int;
                v.intValue = lhs.intValue * rhs.intValue;
            }
        }
        else if (b->op == "/") {
            if (isFloatingType(lhs.type) || isFloatingType(rhs.type)) {
                v.type = ValueType::Double;
                v.doubleValue = asNumber(lhs) / asNumber(rhs);
            }
            else {
                v.type = ValueType::Int;
                v.intValue = lhs.intValue / rhs.intValue;
            }
        }
        else if (b->op == "%") {
            v.type = ValueType::Int;
            v.intValue = lhs.intValue % rhs.intValue;
        }
        else if (b->op == "==") { v.type = ValueType::Bool; v.boolValue = asNumber(lhs) == asNumber(rhs) && toString(lhs) == toString(rhs); }
        else if (b->op == "!=") { v.type = ValueType::Bool; v.boolValue = !(toString(lhs) == toString(rhs) && asNumber(lhs) == asNumber(rhs)); }
        else if (b->op == "<") { v.type = ValueType::Bool; v.boolValue = asNumber(lhs) < asNumber(rhs); }
        else if (b->op == "<=") { v.type = ValueType::Bool; v.boolValue = asNumber(lhs) <= asNumber(rhs); }
        else if (b->op == ">") { v.type = ValueType::Bool; v.boolValue = asNumber(lhs) > asNumber(rhs); }
        else if (b->op == ">=") { v.type = ValueType::Bool; v.boolValue = asNumber(lhs) >= asNumber(rhs); }
        else if (b->op == "&&") { v.type = ValueType::Bool; v.boolValue = truthy(lhs) && truthy(rhs); }
        else if (b->op == "||") { v.type = ValueType::Bool; v.boolValue = truthy(lhs) || truthy(rhs); }
        return v;
    }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = toString(evaluateExpr(idx->base)) + "[" + toString(evaluateExpr(idx->index)) + "]";
        return v;
    }
    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) {
        Value& target = lvalue(pi->name);
        target.intValue++;
        return target;
    }
    if (auto pm = std::dynamic_pointer_cast<PostMinExpr>(e)) {
        Value& target = lvalue(pm->name);
        target.intValue--;
        return target;
    }
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = "[";
        for (size_t i = 0; i < lit->items.size(); i++) {
            if (i) v.stringValue += ", ";
            v.stringValue += toString(evaluateExpr(lit->items[i]));
        }
        v.stringValue += "]";
        return v;
    }
    if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) {
        auto it = userFunctions.find(call->callee);
        if (it != userFunctions.end()) {
            return callUserFunction(it->second, call->args);
        }
        Value v;
        v.type = ValueType::String;
        v.stringValue = call->callee + "(";
        for (size_t i = 0; i < call->args.size(); i++) {
            if (i) v.stringValue += ", ";
            v.stringValue += toString(evaluateExpr(call->args[i]));
        }
        v.stringValue += ")";
        return v;
    }
    if (auto c = std::dynamic_pointer_cast<ConcatExpr>(e)) {
        Value v;
        v.type = ValueType::String;
        std::string res;
        for (auto& piece : c->pieces) res += toString(evaluateExpr(piece));
        v.stringValue = res;
        return v;
    }
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(e)) {
        if (auto obj = std::dynamic_pointer_cast<NameExpr>(m->object)) {
            return classInstances[obj->name][m->member];
        }
        result.stringValue = "/* unsupported member access */";
        return result;
    }
    if (auto mc = std::dynamic_pointer_cast<MethodCallExpr>(e)) {
        if (auto obj = std::dynamic_pointer_cast<NameExpr>(mc->object)) {
            auto clsIt = classMethods.find(obj->name);
            if (clsIt != classMethods.end()) {
                auto methodIt = clsIt->second.find(mc->method);
                if (methodIt != clsIt->second.end()) {
                    return callMethod(obj->name, methodIt->second, mc->args);
                }
            }
        }
        result.stringValue = "/* unresolved method call */";
        return result;
    }
    result.stringValue = "/* unknown expr */";
    return result;
}

static void executePrintStmt(const ExprPtr& expr) {
    if (expr) std::cout << toString(evaluateExpr(expr));
}

static ExecResult execContinueStmt() {
    ExecResult result;
    result.state = ExecState::Continue;
    return result;
}

static ExecResult execBreakStmt() {
    ExecResult result;
    result.state = ExecState::Break;
    return result;
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
            chainMatched = false; // chain ends here regardless
            continue;
        }

        chainMatched = false; // any non-elif/else statement breaks the chain
        ExecResult r = execStmt(stmt);
        if (r.state != ExecState::Normal) return r;
    }
    return {};
}

static ExecResult execStmt(const StmtPtr& stmt) {
    if (auto decl = std::dynamic_pointer_cast<VarDecl>(stmt)) {
        lvalue(decl->name) = decl->init ? evaluateExpr(decl->init) : Value{};
        return {};
    }
    if (auto a = std::dynamic_pointer_cast<AssignStmt>(stmt)) {
        lvalue(a->name) = evaluateExpr(a->value);
        return {};
    }
    if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        ExecResult result;
        result.state = ExecState::Return;
        if (r->value) result.value = evaluateExpr(r->value);
        return result;
    }
    if (auto p = std::dynamic_pointer_cast<PrintCode>(stmt)) {
        executePrintStmt(p->value);
        if (p->newline) std::cout << '\n';
        return {};
    }
    if (auto in = std::dynamic_pointer_cast<ReadCode>(stmt)) {
        if (in->prompt) std::cout << toString(evaluateExpr(in->prompt));
        Value& target = lvalue(in->varName);
        target.type = ValueType::String;
        std::cin >> target.stringValue;
        return {};
    }
    if (auto inStr = std::dynamic_pointer_cast<ReadLine>(stmt)) {
        if (inStr->prompt) std::cout << toString(evaluateExpr(inStr->prompt));
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        Value& target = lvalue(inStr->varName);
        target.type = ValueType::String;
        std::getline(std::cin, target.stringValue);
        return {};
    }
    if (auto es = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        evaluateExpr(es->expr);
        return {};
    }
    if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        while (truthy(evaluateExpr(w->condition))) {
            ExecResult r = execBlock(w->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Continue) continue;
            if (r.state == ExecState::Return) return r;
        }
        return {};
    }
    if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(stmt)) {
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
        return {};
    }
    if (auto c = std::dynamic_pointer_cast<ContinueStmt>(stmt)) {
        return execContinueStmt();
    }
    if (auto b = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        return execBreakStmt();
    }
    if (auto cd = std::dynamic_pointer_cast<ClearStmt>(stmt)) {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
        return {};
    }
    if (auto rp = std::dynamic_pointer_cast<RepeatCode>(stmt)) {
        Value value = evaluateExpr(rp->value);
        int x = 0;
        while (x < value.intValue) {
            ExecResult r = execBlock(rp->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
            x++;
        }
        return {};
    }
    if (auto fr = std::dynamic_pointer_cast<ForeverCode>(stmt)) {
        while (true) {
            ExecResult r = execBlock(fr->body);
            if (r.state == ExecState::Break) break;
            if (r.state == ExecState::Return) return r;
        }
        return {};
    }

    std::cerr << "interpreter error: no handler for this statement type.\n";
    std::exit(EXIT_FAILURE);
}

// Registers every top-level function, struct and every class's fields/methods
static void setupProgram(Program& program) {
    for (auto& fn : program.functions) {
        userFunctions[fn.name] = &fn;
    }

    for (auto& cls : program.classes) {

        auto& fields = classInstances[cls.name];
        auto& methods = classMethods[cls.name];

        auto registerBody = [&](std::vector<StmtPtr>& body) {
            for (auto& s : body) {
                if (auto v = std::dynamic_pointer_cast<VarDecl>(s)) {
                    fields[v->name] = v->init ? evaluateExpr(v->init) : Value{};
                }
                else if (auto m = std::dynamic_pointer_cast<CFDecl>(s)) {
                    methods[m->name] = m.get();
                }
            }
            };
        registerBody(cls.publicBody);
        registerBody(cls.privateBody);
    }

    for (auto& str : program.struc) {
        auto& fields = structInstances[str.name];
        std::cout << "Done struct!\n";

        auto registerBody = [&](std::vector<StmtPtr>& body) {
            for (auto& s : body) {
                if (auto v = std::dynamic_pointer_cast<VarDecl>(s)) {
                    fields[v->name] = v->init ? evaluateExpr(v->init) : Value{};
                }
            }
            };
        registerBody(str.body);
        std::cout << "Done struct!\n";
    }
}

void Interpreter(Program& program) {
    scopes.assign(1, {});
    classInstances.clear();
    classMethods.clear();
    userFunctions.clear();

    setupProgram(program);

    auto it = userFunctions.find("main");
    if (it != userFunctions.end()) {
        execBlock(it->second->body);
    }
    else {
        std::cerr << "interpreter error: no 'main' function found.\n";
    }
}

static std::string toString(const Value& v) {
    switch (v.type) {
    case ValueType::Int:    return std::to_string(v.intValue);
    case ValueType::Float:  return std::to_string(v.floatValue);
    case ValueType::Double: return std::to_string(v.doubleValue);
    case ValueType::String: return v.stringValue;
    case ValueType::Bool:   return v.boolValue ? "true" : "false";
    case ValueType::Char:   return std::string(1, v.charValue);
    case ValueType::Byte:   return std::to_string((int)v.byteValue);
    default:                return std::string{};
    }
}