////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ExprTests.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// Main tests for Expressions 
// This tests : Expression Lexing, Parsing, Typechecking and Evaluation (RTExprVisitor)
////------------------------------------------------------////

#pragma once

#include "../ITest.hpp"

#include "Moonshot/Fox/Semantic/TypeCheck.hpp"
#include "Moonshot/Fox/Eval/Expr/RTExprVisitor.hpp"

namespace Moonshot::Test
{
	class ExprTests : public ITest
	{
		public:
			ExprTests() = default;

			virtual std::string getTestName() const override;
			virtual bool runTest(Context & context) override;
	};
}



