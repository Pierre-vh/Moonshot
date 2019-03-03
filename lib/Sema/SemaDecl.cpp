//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : SemaDecl.cpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  This file implements Sema methods related to Decls and most of the 
//  decl checking logic.
//----------------------------------------------------------------------------//

#include "Fox/Sema/Sema.hpp"
#include "Fox/AST/ASTVisitor.hpp"
#include "Fox/AST/ASTWalker.hpp"
#include "Fox/Common/Errors.hpp"
#include "Fox/Common/DiagnosticEngine.hpp"
#include <map>

using namespace fox;


class Sema::DeclChecker : Checker, DeclVisitor<DeclChecker, void> {
  using Inherited = DeclVisitor<DeclChecker, void>;
  friend Inherited;
  public:
    DeclChecker(Sema& sema) : Checker(sema) {}

    void check(Decl* decl) {
      assert(decl);
      assert(decl->isUnchecked() && "Decl has already been checked!");
      visit(decl);
    }

  private:
    //----------------------------------------------------------------------//
    // Diagnostic methods
    //----------------------------------------------------------------------//
    // The diagnose family of methods are designed to print the most relevant
    // diagnostics for a given situation.
    //----------------------------------------------------------------------//

    // Diagnoses an illegal variable redeclaration. 
    // "decl" is the illegal redecl, "decls" is the list of previous decls.
    //
    // If the decl shouldn't be considered a illegal redeclaration, returns
    // false.
    bool diagnoseIllegalRedecl(NamedDecl* decl, const NamedDeclVec& decls) {
      // Find the original decl
      NamedDecl* earliest = findOriginalDecl(decl->getBeginLoc(), decls);
      // If there's a earliest decl, diagnose. 
      // (We might not have one if this is the first decl)
      if(earliest)
        diagnoseIllegalRedecl(earliest, decl);
      return (bool)earliest;
    }

    // Helper diagnoseIllegalRedecl
    DiagID getAppropriateDiagForRedecl(NamedDecl* original, NamedDecl* redecl) {
      // This is a "classic" var redeclaration
      if (isVarOrParamDecl(original) && isVarOrParamDecl(redecl)) {
        return isa<ParamDecl>(original) ?
          DiagID::invalid_param_redecl : DiagID::invalid_var_redecl;
      }
      // Invalid function redeclaration
      else if (isa<FuncDecl>(original) && isa<FuncDecl>(redecl))
        return DiagID::invalid_redecl;
      // Redecl as a different kind of symbol
      else
        return DiagID::invalid_redecl_diff_symbol_kind;
    }

    // Diagnoses an illegal redeclaration where "redecl" is an illegal
    // redeclaration of "original"
    void diagnoseIllegalRedecl(NamedDecl* original, NamedDecl* redecl) {
      // Quick checks
      Identifier id = original->getIdentifier();
      assert(original && redecl && "args cannot be null!");
      assert((id == redecl->getIdentifier())
        && "it's a redeclaration but names are different?");

      DiagID diagID = getAppropriateDiagForRedecl(original, redecl);
      getDiags()
        .report(diagID, redecl->getIdentifierRange())
        .addArg(id);
      getDiags()
        .report(DiagID::first_decl_seen_here, original->getIdentifierRange())
        .addArg(id);
    }

    // Diagnoses an incompatible variable initializer.
    void diagnoseInvalidVarInitExpr(VarDecl* decl, Expr* init) {
      Type initType = init->getType();
      Type declType = decl->getValueType();

      if(!Sema::isWellFormed({initType, declType})) return;

      getDiags()
        .report(DiagID::invalid_vardecl_init_expr, init->getSourceRange())
        .addArg(initType)
        .addArg(declType)
        .setExtraRange(decl->getTypeLoc().getSourceRange());
    }

    //----------------------------------------------------------------------//
    // "visit" methods
    //----------------------------------------------------------------------//
    // Theses visit() methods will perform the necessary tasks to check a
    // single declaration.
    //
    // Theses methods may call visit on the children of the declaration, or 
    // call Sema checking functions to perform Typechecking of other node
    // kinds.
    //----------------------------------------------------------------------//

    void visit(Decl* decl) {
      decl->setCheckState(Decl::CheckState::Checking);
      Inherited::visit(decl);
      decl->setCheckState(Decl::CheckState::Checked);
    }

    void visitParamDecl(ParamDecl* decl) {
      // Check this decl for being an illegal redecl
      checkForIllegalRedecl(decl);
    }

    void visitVarDecl(VarDecl* decl) {
      // Check this decl for being an illegal redecl
      checkForIllegalRedecl(decl);

      // Check the init expr
      if (Expr* init = decl->getInitExpr()) {
        // Check the init expr
        bool ok = getSema().typecheckExprOfType(init, decl->getValueType());
        // Replace the expr
        decl->setInitExpr(init);
        // If the type didn't match, diagnose
        if(!ok)
          diagnoseInvalidVarInitExpr(decl, init);
      }
    }

    void visitFuncDecl(FuncDecl* decl) {
      // Also, tell it that we're entering its DeclContext.
      auto raiiDC = getSema().enterDeclCtxtRAII(decl);
      // Check if this is an invalid redecl
      checkForIllegalRedecl(decl);
      // Check it's parameters
      for (ParamDecl* param : *decl->getParams())
        visit(param);
      getSema().checkStmt(decl->getBody());
    }

    void visitUnitDecl(UnitDecl* unit) {
      // Tell Sema that we're inside this unit's DC
      auto dcGuard = getSema().enterDeclCtxtRAII(unit);
      // And just visit every decl inside the UnitDecl
      for(Decl* decl : unit->getDecls())
        visit(decl);
    }

    //----------------------------------------------------------------------//
    // Helper checking methods
    //----------------------------------------------------------------------//
    // Various semantics-related helper methods 
    //----------------------------------------------------------------------//

    // Checks if "decl" is a illegal redeclaration.
    // If "decl" is a illegal redecl, the appropriate diagnostic is emitted
    // and this function returns false.
    // Returns true if "decl" is legal redeclaration or not a redeclaration
    // at all.
    void checkForIllegalRedecl(NamedDecl* decl) {        
      Identifier id = decl->getIdentifier();
      LookupResult lookupResult;
      // Build Lookup options:
      LookupOptions options;
      // If the Decl is local, only look in local decl contexts.
      options.onlyLookInLocalDeclContexts = decl->isLocal();
      options.shouldIgnore = [&](NamedDecl* result){
        // Ignore if result == decl
        if(result == decl) return true;
        // Ignore if result isn't from the same file
        if(result->getFileID() != decl->getFileID()) return true;
        return false;  // else, don't ignore.
      };
      getSema().doUnqualifiedLookup(lookupResult, id, decl->getBeginLoc(),
                                    options);
      // If there are no matches, this cannot be a redecl
      if (lookupResult.size() == 0)
        return;
      else {
        // if we only have 1 result, and it's a ParamDecl
        NamedDecl* found = lookupResult.getIfSingleResult();
        if (found && isa<ParamDecl>(found) && !isa<ParamDecl>(decl)) {
          assert(decl->isLocal() && "Global declaration is conflicting with "
                 "a parameter declaration?");
          // Redeclaration of a ParamDecl by non ParamDecl is allowed
          // (a variable decl can shadow a parameter)
          return;
        }
        // Else, diagnose.
        bool isRedecl = diagnoseIllegalRedecl(decl, lookupResult.getDecls());
        // If it's indeed an illegal redeclaration, mark it.
        decl->setIsIllegalRedecl(isRedecl);
        return;
      }
    }

    //----------------------------------------------------------------------//
    // Other helper methods
    //----------------------------------------------------------------------//
    // Non semantics related helper methods
    //----------------------------------------------------------------------//
    
    // Searches a lookup result to find the decl that should be considered
    // the "original" decl when diagnosing for illegal redeclarations.
    //
    // The return result may be nullptr!
    NamedDecl* 
    findOriginalDecl(SourceLoc loc, const NamedDeclVec& decls) {
      assert(decls.size() && "Empty decl set");
      // First, add the decls to our own vector, but ignore:
      //  - unchecked decls
      //  - illegal redecls
      //  - decls that came after our decl
      SmallVector<NamedDecl*, 4> candidates; 
      candidates.reserve(decls.size());
      for (NamedDecl* decl : decls) {
        if(decl->isUnchecked()) continue;
        if(decl->isIllegalRedecl()) continue;
        if(decl->getBeginLoc().comesBefore(loc))
          candidates.push_back(decl);
      }

      // Maybe we already have our result
      if(candidates.size() == 0) return nullptr;
      if(candidates.size() == 1) return candidates[0];

      // Else, find the latest decl in the vector
      NamedDecl* candidate = nullptr;
      FileID file = loc.getFileID();
      for (NamedDecl* decl : candidates) {
        SourceLoc declLoc = decl->getBeginLoc();
        // If we have no candidate, take this decl as the first candidate
        if (!candidate) {
          candidate = decl;
          continue;
        }
        SourceLoc candLoc = candidate->getBeginLoc();
        // if this decl has been declared after the candidate, it
        // becomes the new candidate
        if (candLoc.comesBefore(declLoc))
          candidate = decl;
      }
      return candidate;
    }

    // Return true if decl is a VarDecl or ParamDecl
    bool isVarOrParamDecl(NamedDecl* decl) {
      return isa<ParamDecl>(decl) || isa<VarDecl>(decl);
    }

};

void Sema::checkUnitDecl(UnitDecl* decl) {
  checkDecl(decl);
}

void Sema::checkDecl(Decl* decl) {
  DeclChecker(*this).check(decl);
}