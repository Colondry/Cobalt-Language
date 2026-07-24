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
#include <limits>

static std::map<std::string, Value> variables;
static std::string toString(const Value& v);

static Value evaluateExpr(const ExprPtr& e);

ExecResult execStmt(const StmtPtr& stmt);

static Value evaluateExpr(const ExprPtr& e) {
    Value result;
    result.type = ValueType::String;
    if (auto n = std::dynamic_pointer_cast<NumberLit>(e)) {
        Value v;
        v.type = ValueType::Int;
        v.intValue = std::stoi(n->value);
        return v;
    } if (auto s = std::dynamic_pointer_cast<StringLit>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = s->value;
        return v;
    }
    if (auto id = std::dynamic_pointer_cast<NameExpr>(e)) {
        return variables[id->name];
    }
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) {
        Value operand = evaluateExpr(u->operand);
        Value v;
        v.type = ValueType::Int;
        if (u->op == "-") {
            v.intValue = -operand.intValue;
        } else if (u->op == "!") {
            v.boolValue = !operand.boolValue;
            v.type = ValueType::Bool;
        }
        return v;
    }
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) {
        Value lhs = evaluateExpr(b->lhs);
        Value rhs = evaluateExpr(b->rhs);

        Value v;
        v.type = ValueType::Bool;

        if (b->op == "+") {
            if (lhs.type == ValueType::String ||
                rhs.type == ValueType::String)
            {
                v.type = ValueType::String;
                v.stringValue = toString(lhs) + toString(rhs);
            }
            else
            {
                v.type = ValueType::Int;
                v.intValue = lhs.intValue + rhs.intValue;
            }
        }
        else if (b->op == "*") {
            v.type = ValueType::Int;
            v.intValue = lhs.intValue * rhs.intValue;
        }
        else if (b->op == "/") {
            v.type = ValueType::Int;
            v.intValue = lhs.intValue / rhs.intValue;
        }
        else if (b->op == "%") {
            v.type = ValueType::Int;
            v.intValue = lhs.intValue % rhs.intValue;
        }
        else if (b->op == "==")
            v.boolValue = lhs.intValue == rhs.intValue;
        else if (b->op == "!=")
            v.boolValue = lhs.intValue != rhs.intValue;
        else if (b->op == "<")
            v.boolValue = lhs.intValue < rhs.intValue;
        else if (b->op == "<=")
            v.boolValue = lhs.intValue <= rhs.intValue;
        else if (b->op == ">")
            v.boolValue = lhs.intValue > rhs.intValue;
        else if (b->op == ">=")
            v.boolValue = lhs.intValue >= rhs.intValue;
        else if (b->op == "&&") {
            v.type = ValueType::Bool;
            v.boolValue = lhs.boolValue && rhs.boolValue;
        }
        else if (b->op == "||") {
            v.type = ValueType::Bool;
            v.boolValue = lhs.boolValue || rhs.boolValue;
        }
        return v;
    }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = evaluateExpr(idx->base).stringValue + "[" + evaluateExpr(idx->index).stringValue + "]";
        return v;
    }
    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) {
        variables[pi->name].intValue++;

        return variables[pi->name];
    }
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = "[";
        for (size_t i = 0; i < lit->items.size(); i++) {
            if (i) v.stringValue += ", ";
            v.stringValue += evaluateExpr(lit->items[i]).stringValue;
        }
        v.stringValue += "]";
        return v;
    }
    if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) {
        Value v;
        v.type = ValueType::String;
        v.stringValue = call->callee + "(";
        for (size_t i = 0; i < call->args.size(); i++) {
            if (i) v.stringValue += ", ";
            v.stringValue += evaluateExpr(call->args[i]).stringValue;
        }
        v.stringValue += ")";
        return v;
    }
    if (auto c = std::dynamic_pointer_cast<ConcatExpr>(e)) {
        std::string res = "(";
        for (size_t i = 0; i < c->pieces.size(); i++) {
            if (i) res += " + ";
            res += evaluateExpr(c->pieces[i]).stringValue;
        }
        res += ")";
        result.stringValue = res;
        return result;
    }
    result.stringValue = "/* unknown expr */";
    return result;
}

ExecResult executePrintStmt(const ExprPtr& expr)
{
    Value v = evaluateExpr(expr);

    switch(v.type)
    {
        case ValueType::Int:
            std::cout << v.intValue;
            break;

        case ValueType::String:
            std::cout << v.stringValue;
            break;

        case ValueType::Bool:
            std::cout << v.boolValue;
            break;
        
            case ValueType::Float:
                std::cout << v.floatValue;
                break;

            case ValueType::Double:
                std::cout << v.doubleValue;
                break;

            case ValueType::Char:
                std::cout << v.charValue;
                break;

            case ValueType::Byte:
                std::cout << (int)v.byteValue;
                break;

        default:
            break;
    }

    std::cout << '\n';

    return {};
}
ExecResult execContinueStmt() {
    ExecResult result;
    result.state = ExecState::Continue;
    return result;
}

ExecResult execBreakStmt() {
    ExecResult result;
    result.state = ExecState::Break;
    return result;
}

static int depth = 0;

ExecResult execBlock(const std::vector<StmtPtr>& body) {
    Value v;
    v.type = ValueType::String;
    for (auto& stmt : body)
    {
        ExecResult r = execStmt(stmt);

        if (r.state != ExecState::Normal)
            return r;
    }

    return {};
}

ExecResult execStmt(const StmtPtr& stmt) {
    Value v;
    v.type = ValueType::String;
    if (auto decl = std::dynamic_pointer_cast<VarDecl>(stmt)) {
        v.type = ValueType::String;
        if (decl->init) {
            variables[decl->name] = evaluateExpr(decl->init);
        } else {
            variables[decl->name] = Value{};
        }
        return {};
    }
    if (auto a = std::dynamic_pointer_cast<AssignStmt>(stmt)) {
        variables[a->name] = evaluateExpr(a->value);
    }
    if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        ExecResult result;
        result.state = ExecState::Return;
        result.value = evaluateExpr(r->value);
        return result;
    }
    if (auto p = std::dynamic_pointer_cast<PrintCode>(stmt)) {
        return executePrintStmt(p->value);
    }
    if (auto in = std::dynamic_pointer_cast<InputCode>(stmt)) {
        if (in->prompt) {
            std::cout << evaluateExpr(in->prompt).stringValue;
        }
        std::cin >> variables[in->varName].stringValue;
        return {};
    }
    if (auto inStr = std::dynamic_pointer_cast<InputString>(stmt)) {
        if (inStr->prompt) {
            std::cout << evaluateExpr(inStr->prompt).stringValue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, variables[inStr->varName].stringValue);
        return {};
    }
    if (auto es = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        v.stringValue = evaluateExpr(es->expr).stringValue + ";\n";
        return {};
    }
    if (auto i = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        Value condValue = evaluateExpr(i->condition);
        if (condValue.boolValue) {
            ExecResult result = execBlock(i->body);
            if (result.state != ExecState::Normal) {
                return result;
            }
        }
    }
    if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        Value condValue = evaluateExpr(w->condition);
        while(evaluateExpr(w->condition).boolValue)
        {
            ExecResult r =
                execBlock(w->body);

            if(r.state==ExecState::Break)
                break;

            if(r.state==ExecState::Continue)
                continue;

            if(r.state==ExecState::Return)
                return r;
        }
    }
    if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(stmt)) {
        Value from =
            evaluateExpr(f->from);

        Value to =
            evaluateExpr(f->to);

        variables[f->varName] = from;

        while(variables[f->varName].intValue <= to.intValue)
        {
            ExecResult r =
                execBlock(f->body);

            if(r.state==ExecState::Break)
                break;

            if(r.state==ExecState::Return)
                return r;

            variables[f->varName].intValue++;
        }
    }
    if (auto c = std::dynamic_pointer_cast<ContinueStmt>(stmt)) {
        return execContinueStmt();
    }
    if (auto b = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        return execBreakStmt();
    }

    std::cerr << "codeGen error: no emitter for this statement type.\n";
    std::exit(EXIT_FAILURE);
}

void Interpreter(Program& program) {
    for (auto fn : program.functions) {
        if (fn.name == "main") {
            execBlock(fn.body);
            return;
        }
    }
}

static std::string toString(const Value& v) {
    switch (v.type) {
        case ValueType::Int:    return std::to_string(v.intValue);
        case ValueType::String: return v.stringValue;
        case ValueType::Bool:   return v.boolValue ? "true" : "false";
        default:                return std::string{};
    }
}