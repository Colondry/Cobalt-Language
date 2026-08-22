#include "deadcode.hpp"
#include <unordered_set>

// ---------- side-effect check ----------
static bool hasSideEffects(const ExprPtr& e) {
    if (!e) return false;
    if (std::dynamic_pointer_cast<NumberLit>(e)) return false;
    if (std::dynamic_pointer_cast<StringLit>(e)) return false;
    if (std::dynamic_pointer_cast<CharLit>(e)) return false;
    if (std::dynamic_pointer_cast<NameExpr>(e)) return false;
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) return hasSideEffects(u->operand);
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) return hasSideEffects(b->lhs) || hasSideEffects(b->rhs);
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e)) return hasSideEffects(idx->base) || hasSideEffects(idx->index);
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        for (auto& i : lit->items) if (hasSideEffects(i)) return true;
        return false;
    }
    if (auto fl = std::dynamic_pointer_cast<FracLit>(e)) {
        for (auto& i : fl->items) if (hasSideEffects(i)) return true;
        return false;
    }
    if (auto c = std::dynamic_pointer_cast<ConcatExpr>(e)) {
        for (auto& p : c->pieces) if (hasSideEffects(p)) return true;
        return false;
    }
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(e)) return hasSideEffects(m->object);
    if (auto mm = std::dynamic_pointer_cast<MethodMemberExpr>(e)) return hasSideEffects(mm->object);
    // CallExpr, MethodCallExpr, NamespaceCallExpr, PostIncExpr, PostMinExpr and anything else unrecognized: assume impure.
    return true;
}

// ---------- usage collection ----------
static void collectUses(const ExprPtr& e, std::unordered_set<std::string>& used) {
    if (!e) return;
    if (auto n = std::dynamic_pointer_cast<NameExpr>(e)) { used.insert(n->name); return; }
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) { collectUses(u->operand, used); return; }
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) { collectUses(b->lhs, used); collectUses(b->rhs, used); return; }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e)) { collectUses(idx->base, used); collectUses(idx->index, used); return; }
    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) { used.insert(pi->name); return; }
    if (auto pm = std::dynamic_pointer_cast<PostMinExpr>(e)) { used.insert(pm->name); return; }
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) { for (auto& it : lit->items) collectUses(it, used); return; }
    if (auto fl = std::dynamic_pointer_cast<FracLit>(e)) { for (auto& it : fl->items) collectUses(it, used); return; }
    if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) { for (auto& a : call->args) collectUses(a, used); return; }
    if (auto c = std::dynamic_pointer_cast<ConcatExpr>(e)) { for (auto& p : c->pieces) collectUses(p, used); return; }
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(e)) { collectUses(m->object, used); return; }
    if (auto mc = std::dynamic_pointer_cast<MethodCallExpr>(e)) {
        collectUses(mc->object, used);
        for (auto& a : mc->args) collectUses(a, used);
        return;
    }
    if (auto nc = std::dynamic_pointer_cast<NamespaceCallExpr>(e)) {
        collectUses(nc->object, used);
        for (auto& a : nc->args) collectUses(a, used);
        return;
    }
    if (auto mm = std::dynamic_pointer_cast<MethodMemberExpr>(e)) { collectUses(mm->object, used); return; }
    // NumberLit / StringLit / CharLit: nothing to record.
}

static void collectUsesInBlock(const std::vector<StmtPtr>& body, std::unordered_set<std::string>& used) {
    for (auto& s : body) {
        if (auto v = std::dynamic_pointer_cast<VarDecl>(s))              collectUses(v->init, used);
        else if (auto a = std::dynamic_pointer_cast<AssignStmt>(s))      collectUses(a->value, used);
        else if (auto ea = std::dynamic_pointer_cast<ExprAssignStmt>(s)) { collectUses(ea->target, used); collectUses(ea->value, used); }
        else if (auto r = std::dynamic_pointer_cast<ReturnStmt>(s))      collectUses(r->value, used);
        else if (auto p = std::dynamic_pointer_cast<PrintCode>(s))       collectUses(p->value, used);
        else if (auto p = std::dynamic_pointer_cast<PrintMacCode>(s))    collectUses(p->value, used);
        else if (auto rd = std::dynamic_pointer_cast<ReadCode>(s))       { collectUses(rd->prompt, used); collectUses(rd->target, used); }
        else if (auto rl = std::dynamic_pointer_cast<ReadLine>(s))       { collectUses(rl->prompt, used); collectUses(rl->target, used); }
        else if (auto es = std::dynamic_pointer_cast<ExprStmt>(s))       collectUses(es->expr, used);
        else if (auto i = std::dynamic_pointer_cast<IfStmt>(s))          { collectUses(i->condition, used); collectUsesInBlock(i->body, used); }
        else if (auto ei = std::dynamic_pointer_cast<ElifStmt>(s))       { collectUses(ei->condition, used); collectUsesInBlock(ei->body, used); }
        else if (auto el = std::dynamic_pointer_cast<ElseStmt>(s))       collectUsesInBlock(el->body, used);
        else if (auto w = std::dynamic_pointer_cast<WhileStmt>(s))       { collectUses(w->condition, used); collectUsesInBlock(w->body, used); }
        else if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(s))    { collectUses(f->from, used); collectUses(f->to, used); collectUsesInBlock(f->body, used); }
        else if (auto rp = std::dynamic_pointer_cast<RepeatCode>(s))     { collectUses(rp->value, used); collectUsesInBlock(rp->body, used); }
        else if (auto fr = std::dynamic_pointer_cast<ForeverCode>(s))    collectUsesInBlock(fr->body, used);
        // CFDecl / LambFuncDecl (nested fn decls) intentionally left alone
    }
}

static void pruneNestedBodies(const StmtPtr& s, std::vector<std::string>& warnings) {
    if (auto i = std::dynamic_pointer_cast<IfStmt>(s))       i->body  = pruneUnusedVars(i->body, warnings);
    else if (auto ei = std::dynamic_pointer_cast<ElifStmt>(s)) ei->body = pruneUnusedVars(ei->body, warnings);
    else if (auto el = std::dynamic_pointer_cast<ElseStmt>(s)) el->body = pruneUnusedVars(el->body, warnings);
    else if (auto w = std::dynamic_pointer_cast<WhileStmt>(s)) w->body  = pruneUnusedVars(w->body, warnings);
    else if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(s)) f->body = pruneUnusedVars(f->body, warnings);
    else if (auto rp = std::dynamic_pointer_cast<RepeatCode>(s))  rp->body = pruneUnusedVars(rp->body, warnings);
    else if (auto fr = std::dynamic_pointer_cast<ForeverCode>(s)) fr->body = pruneUnusedVars(fr->body, warnings);
}

std::vector<StmtPtr> pruneUnusedVars(const std::vector<StmtPtr>& body, std::vector<std::string>& warnings) {
    for (auto& s : body) pruneNestedBodies(s, warnings);

    std::unordered_set<std::string> used;
    collectUsesInBlock(body, used);

    // 3. Drop any VarDecl at this level whose name was never read.
    std::vector<StmtPtr> out;
    out.reserve(body.size());
    for (auto& s : body) {
        if (auto v = std::dynamic_pointer_cast<VarDecl>(s)) {
            if (!used.count(v->name)) {
                if (hasSideEffects(v->init)) {
                    auto es = std::make_shared<ExprStmt>();
                    es->expr = v->init;
                    out.push_back(es);
                    warnings.push_back("unused variable '" + v->name + "' -- declaration removed, initializer kept for its side effect");
                }
                else {
                    warnings.push_back("unused variable '" + v->name + "' -- removed");
                }
                continue;
            }
        }
        out.push_back(s);
    }
    return out;
}