//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : BCGenDecl.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/BCGen/BCGen.hpp"
#include "Registers.hpp"
#include "Fox/AST/Decl.hpp"
#include "Fox/AST/Expr.hpp"
#include "Fox/AST/Stmt.hpp"
#include "Fox/AST/ASTWalker.hpp"
#include "Fox/AST/ASTVisitor.hpp"
#include "Fox/BC/BCBuilder.hpp"
#include "Fox/BC/BCModule.hpp"
#include "Fox/Common/Errors.hpp"

using namespace fox;

//----------------------------------------------------------------------------//
// DeclGenerator 
//----------------------------------------------------------------------------//

class BCGen::LocalDeclGenerator : public Generator,
                                  DeclVisitor<LocalDeclGenerator, void> {
  using Inherited = DeclVisitor<LocalDeclGenerator, void>;
  friend Inherited;
  public:
    LocalDeclGenerator(BCGen& gen, BCBuilder& builder,
                  RegisterAllocator& regAlloc): 
      Generator(gen, builder), regAlloc(regAlloc) {}

    void generate(Decl* decl) {
      visit(decl);
    }

    RegisterAllocator& regAlloc;

  private:
    void visitUnitDecl(UnitDecl*) {
      return fox_unreachable("UnitDecl found at the local level");
    }

    void visitVarDecl(VarDecl* decl) {
      RegisterValue initReg;

      // Generate the initializer if there's one
      if (Expr* init = decl->getInitExpr()) {
        initReg = bcGen.genExpr(builder, regAlloc, init);

        // If possible, store the variable directly in initReg.
        if (initReg.canRecycle()) {
          regAlloc.initVar(decl, &initReg); // discard the RegisterValue directly
          assert(!initReg.isAlive() && "hint not consumed");
          return;
        } 
      }

      // Initialize the variable normally, duplicating the register containing
      // the initializer in the var's designated register.
      RegisterValue var = regAlloc.initVar(decl);
      // Init the var if we have an initializer
      if(initReg)
        builder.createCopyInstr(var.getAddress(), initReg.getAddress());
    }

    void visitParamDecl(ParamDecl*) {
      fox_unimplemented_feature("ParamDecl BCGen");
    }
    void visitFuncDecl(FuncDecl*) {
      return fox_unreachable("FuncDecl found at the local level");
    }
};

//----------------------------------------------------------------------------//
// FuncGenPrologue 
//
// This performs some tasks that are needed in order to correctly generate
// the bytecode for the body of a FuncDecl. One such task is notifying the
// RegisterAllocator of every local variable declaration/usage so it can
// know the number of uses a variable has (to free its register after its last
// use)
//----------------------------------------------------------------------------//

namespace {
  class FuncGenPrologue : ASTWalker, SimpleASTVisitor<FuncGenPrologue, void> {
    using Inherited = SimpleASTVisitor<FuncGenPrologue, void>;
    friend Inherited;
    public:
      FuncGenPrologue(RegisterAllocator& regAlloc) : regAlloc(regAlloc) {}

      void doPrologue(FuncDecl* decl) {
        // Walk the body
        walk(decl->getBody());
      }

      RegisterAllocator& regAlloc;

    private:
      virtual bool handleDeclPre(Decl* decl) override {
        visit(decl);
        // We want to visit the children
        return true;
      }

      virtual std::pair<Expr*, bool> handleExprPre(Expr* expr) override {
        visit(expr);
        // We want to continue the walk and visit the children
        return {expr, true};
      }

      void visitExpr(Expr*) {
        // no-op
      }

      void visitFuncDecl(FuncDecl*) {
        // Fox does not currently allow functions in a local scope.
        fox_unreachable("FuncDecl found inside a FuncDecl");
      }

      void visitUnitDecl(UnitDecl*) {
        fox_unreachable("UnitDecl found inside a FuncDecl");
      }

      void visitParamDecl(ParamDecl*) {
        fox_unimplemented_feature("FuncGenPrologue for ParamDecls");
      }

      void visitVarDecl(VarDecl* decl) {
        assert(decl->isLocal() && "Non-Local VarDecl found in "
          "FuncGenPrologue?");
        regAlloc.addUsage(decl);
      }

      void visitDeclRefExpr(DeclRefExpr* expr) {
        ValueDecl* decl = expr->getDecl();
        assert(decl && "decl is null in DeclRefExpr");
        if (VarDecl* var = dyn_cast<VarDecl>(decl)) { 
          // Local VarDecl
          if(var->isLocal()) 
            regAlloc.addUsage(var);
          // Global VarDecl
          else 
            fox_unimplemented_feature("FuncGenPrologue for DeclRefExpr "
              "of non-local VarDecls");
        }
        else if (ParamDecl* param = dyn_cast<ParamDecl>(decl)) { 
          fox_unimplemented_feature("FuncGenPrologue for DeclRefExpr "
            "of ParamDecls");
        }
        // else, ignore.
      }

      void visitUnresolvedDeclRefExpr(UnresolvedDeclRefExpr*) {
        fox_unreachable("UnresolvedDeclRefExpr found past semantic analysis");
      }
  };
}

//----------------------------------------------------------------------------//
// BCGen Entrypoints
//----------------------------------------------------------------------------//

// TODO: Return a std::unique_ptr<BCFunction> or take a BCFunction& as param
// this should be a BCFunction factory!
void BCGen::genFunc(BCModule& bcmodule, FuncDecl* func) {
  assert(func && "func is null");
  // Create the RegisterAllocator for this Function
  RegisterAllocator regAlloc;
  // Do the prologue so classes like the RegisterAllocator
  // can be given enough information to correctly generate the bytecode.
  FuncGenPrologue(regAlloc).doPrologue(func);
  // Create a builder
  BCBuilder builder(bcmodule.getInstructions());
  // For now, only gen the body.
  genStmt(builder, regAlloc, func->getBody());
  // TODO: Once we gen the function properly, check that the last instruction
  // emitted was a Return, if it wasn't, insert a return void instruction.
  // (we can assert that the function's return type is void because
  //  else the error would have been caught by Semantic Analysis)
}

void BCGen::genGlobalVar(BCBuilder&, VarDecl*) {
  fox_unimplemented_feature("BCGen::genGlobalVar");
}

void BCGen::genLocalDecl(BCBuilder& builder,
                         RegisterAllocator& regAlloc, Decl* decl) {
  assert(decl->isLocal() && "Decl isn't local!");
  LocalDeclGenerator(*this, builder, regAlloc).generate(decl);
}

std::unique_ptr<BCModule> BCGen::genUnit(UnitDecl* unit) {
  assert(unit && "unit is null");
  // Build the module
  // TODO: Move this in the BCGen class
  std::unique_ptr<BCModule> theModule = std::make_unique<BCModule>();
  // Only gen functions for now.
  for (Decl* decl : unit->getDecls()) {
    if (FuncDecl* fn = dyn_cast<FuncDecl>(decl)) {
      genFunc(*theModule, fn);
      break;
    }
  }
  return theModule;
}