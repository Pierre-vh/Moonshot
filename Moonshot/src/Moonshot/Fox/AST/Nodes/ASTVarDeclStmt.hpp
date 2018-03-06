////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ASTVarDeclStmt.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// The AST Node for Variable Declaration Statements.											
////------------------------------------------------------////

#pragma once

#include "IASTDeclaration.hpp"
#include "ASTExpr.hpp"
#include "Moonshot/Common/Types/Types.hpp"

#include <memory> // std::unique_ptr, std::make_unique

namespace Moonshot 
{
	struct ASTVarDeclStmt : public IASTDeclaration
	{
		public:
			// Create a variable declaration statement by giving the constructor the variable's properties (name,is const and type) and, if there's one, an expression to initialize it.
			ASTVarDeclStmt(const FoxVariableAttr &attr,std::unique_ptr<IASTExpr> iExpr); 

			// Inherited via IASTStmt
			virtual void accept(IVisitor& vis) override;

			FoxVariableAttr vattr_;
			std::unique_ptr<IASTExpr> initExpr_ = nullptr;
	};
}


