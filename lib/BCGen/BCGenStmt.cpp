//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : BCGenStmt.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/BCGen/BCGen.hpp"
#include "Fox/AST/Stmt.hpp"
#include "Fox/AST/ASTVisitor.hpp"
#include "Fox/Common/Errors.hpp"

using namespace fox;
class BCGen::StmtGenerator : public Generator,
                             private StmtVisitor<StmtGenerator, void> {
  using Visitor = StmtVisitor<StmtGenerator, void>;
  friend Visitor;
  public:
    StmtGenerator(BCGen& gen, BCModuleBuilder& builder): 
      Generator(gen, builder) {}

    void generate(Stmt* stmt) {
      visit(stmt);
    }

  private:
    //------------------------------------------------------------------------//
    // "emit" and "gen" methods 
    // 
    // These methods perform some generalized tasks related to bytecode
    // emission
    //------------------------------------------------------------------------//

    void genNode(ASTNode node) {
      if(Decl* decl = node.dyn_cast<Decl*>())
        fox_unimplemented_feature("Local Decl BCGen");
      else if(Expr* expr = node.dyn_cast<Expr*>()) 
        bcGen.genExpr(builder, expr);
      else if(Stmt* stmt = node.dyn_cast<Stmt*>()) 
        visit(stmt);
      else 
        fox_unreachable("Unknown ASTNode kind");
    }

    //------------------------------------------------------------------------//
    // "visit" methods 
    // 
    // Theses methods will perfom the actual tasks required to emit
    // the bytecode for a statement.
    //------------------------------------------------------------------------//

    void visitCompoundStmt(CompoundStmt* stmt) {
      // Just visit all the nodes
      for(ASTNode node : stmt->getNodes())
        genNode(node);
    }

    void visitConditionStmt(ConditionStmt*) {
      fox_unimplemented_feature("ConditionStmt BCGen");
    }

    void visitWhileStmt(WhileStmt*) {
      fox_unimplemented_feature("WhileStmt BCGen");
    }

    void visitReturnStmt(ReturnStmt*) {
      fox_unimplemented_feature("ReturnStmt BCGen");
    }
};

//----------------------------------------------------------------------------//
// BCGen Entrypoints
//----------------------------------------------------------------------------//

void BCGen::genStmt(BCModuleBuilder& builder, Stmt* stmt) {
  assert(stmt && "stmt is null");
  StmtGenerator(*this, builder).generate(stmt);
}