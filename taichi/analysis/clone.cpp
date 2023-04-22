#include "taichi/ir/ir.h"
#include "taichi/ir/analysis.h"
#include "taichi/ir/statements.h"
#include "taichi/ir/transforms.h"
#include "taichi/ir/visitors.h"
#include "taichi/program/program.h"

#include <unordered_map>

namespace taichi::lang {

class IRCloner : public IRVisitor {
 private:
  IRNode *other_node;
  std::unordered_map<Stmt *, Stmt *> operand_map_;
  irpass::analysis::IRClonerContext *ctx_{nullptr};

 public:
  enum Phase { register_operand_map, replace_operand, clone_function } phase;

  explicit IRCloner(IRNode *other_node, irpass::analysis::IRClonerContext *ctx)
      : other_node(other_node), ctx_(ctx), phase(register_operand_map) {
    allow_undefined_visitor = true;
    invoke_default_visitor = true;
  }

  void visit(Block *stmt_list) override {
    auto other = other_node->as<Block>();
    for (int i = 0; i < (int)stmt_list->size(); i++) {
      other_node = other->statements[i].get();
      stmt_list->statements[i]->accept(this);
    }
    other_node = other;
  }

  void generic_visit(Stmt *stmt) {
    if (phase == register_operand_map)
      operand_map_[stmt] = other_node->as<Stmt>();
    else {
      TI_ASSERT(phase == replace_operand);
      auto other_stmt = other_node->as<Stmt>();
      TI_ASSERT(stmt->num_operands() == other_stmt->num_operands());
      for (int i = 0; i < stmt->num_operands(); i++) {
        if (operand_map_.find(stmt->operand(i)) == operand_map_.end())
          other_stmt->set_operand(i, stmt->operand(i));
        else
          other_stmt->set_operand(i, operand_map_[stmt->operand(i)]);
      }
    }
  }

  void visit(Stmt *stmt) override {
    generic_visit(stmt);
  }

  void visit(IfStmt *stmt) override {
    generic_visit(stmt);
    auto other = other_node->as<IfStmt>();
    if (stmt->true_statements) {
      other_node = other->true_statements.get();
      stmt->true_statements->accept(this);
      other_node = other;
    }
    if (stmt->false_statements) {
      other_node = other->false_statements.get();
      stmt->false_statements->accept(this);
      other_node = other;
    }
  }

  void visit(WhileStmt *stmt) override {
    generic_visit(stmt);
    auto other = other_node->as<WhileStmt>();
    other_node = other->body.get();
    stmt->body->accept(this);
    other_node = other;
  }

  void visit(RangeForStmt *stmt) override {
    generic_visit(stmt);
    auto other = other_node->as<RangeForStmt>();
    other_node = other->body.get();
    stmt->body->accept(this);
    other_node = other;
  }

  void visit(StructForStmt *stmt) override {
    generic_visit(stmt);
    auto other = other_node->as<StructForStmt>();
    other_node = other->body.get();
    stmt->body->accept(this);
    other_node = other;
  }

  void visit(OffloadedStmt *stmt) override {
    generic_visit(stmt);
    auto other = other_node->as<OffloadedStmt>();

#define CLONE_BLOCK(B)                    \
  if (stmt->B) {                          \
    other->B = std::make_unique<Block>(); \
    other_node = other->B.get();          \
    stmt->B->accept(this);                \
  }

    CLONE_BLOCK(tls_prologue)
    CLONE_BLOCK(bls_prologue)
    CLONE_BLOCK(mesh_prologue)

    if (stmt->body) {
      other_node = other->body.get();
      stmt->body->accept(this);
    }

    CLONE_BLOCK(bls_epilogue)
    CLONE_BLOCK(tls_epilogue)
#undef CLONE_BLOCK

    other_node = other;
  }

  void visit(FrontendFuncCallStmt *stmt) override {
    if (phase == clone_function) {
      stmt->func = ctx_->clone_function(stmt->func);
    }
  }

  void visit(FuncCallStmt *stmt) override {
    if (phase == clone_function) {
      stmt->func = ctx_->clone_function(stmt->func);
    }
  }

  static std::unique_ptr<IRNode> run(IRNode *root,
                                     irpass::analysis::IRClonerContext *ctx) {
    std::unique_ptr<IRNode> new_root = root->clone();
    IRCloner cloner(new_root.get(), ctx);
    if (ctx) {
      cloner.phase = IRCloner::clone_function;
      root->accept(&cloner);
    }
    cloner.phase = IRCloner::register_operand_map;
    root->accept(&cloner);
    cloner.phase = IRCloner::replace_operand;
    root->accept(&cloner);

    return new_root;
  }
};

namespace irpass::analysis {
Function *IRClonerContext::clone_function(Function *func) {
  functions_.push_back(
      std::make_unique<Function>(func->program, func->func_key));
  auto *cloned = functions_.back().get();
  cloned->set_function_body(irpass::analysis::clone(func->ir.get(), this));
  return cloned;
}

std::unique_ptr<IRNode> clone(IRNode *root, IRClonerContext *ctx) {
  return IRCloner::run(root, ctx);
}
}  // namespace irpass::analysis

}  // namespace taichi::lang
