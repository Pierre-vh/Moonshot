////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ParseStmt.cpp										
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
//	This file implements statements rules. parseStmt, parseVarDeclstmt,etc.									
////------------------------------------------------------////

#include "Parser.hpp"

using namespace Moonshot;

using sign = Token::sign;
using keyword = Token::keyword;

// Context and Exceptions
#include "Moonshot/Common/Context/Context.hpp"
#include "Moonshot/Common/Exceptions/Exceptions.hpp"
//Nodes
#include "Moonshot/Fox/AST/Nodes/ASTCompoundStmt.hpp"
#include "Moonshot/Fox/AST/Nodes/IASTStmt.hpp"
#include "Moonshot/Fox/AST/Nodes/ASTCondStmt.hpp"
#include "Moonshot/Fox/AST/Nodes/ASTWhileStmt.hpp"
#include "Moonshot/Fox/AST/Nodes/ASTVarDecl.hpp"
#include "Moonshot/Fox/AST/Nodes/ASTNullStmt.hpp"
#include "Moonshot/Fox/AST/Nodes/ASTFunctionDecl.hpp"

ParsingResult<ASTCompoundStmt*> Parser::parseCompoundStatement(const bool& isMandatory)
{
	auto rtr = std::make_unique<ASTCompoundStmt>(); // return value
	if (matchSign(sign::B_CURLY_OPEN))
	{
		// Parse all statements
		while (auto parseres = parseStmt())
		{
			if (rtr->statements_.size())
			{
				// Don't push another null statement if the last statement is already a null one.
				if (dynamic_cast<ASTNullStmt*>(rtr->statements_.back().get()) &&
					dynamic_cast<ASTNullStmt*>(parseres.result_.get()))
					continue;
			}
			rtr->statements_.push_back(std::move(parseres.result_));
		}
		// Match the closing curly bracket
		if (!matchSign(sign::B_CURLY_CLOSE))
		{
			errorExpected("Expected a closing curly bracket '}' at the end of the compound statement,");
			if (resyncToDelimiter(sign::B_CURLY_CLOSE))
				return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
			return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::FAILED_AND_DIED);
		}
		return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::SUCCESS,std::move(rtr));
	}
	
	if (isMandatory)
	{
		errorExpected("Expected a '{'");
		if (resyncToDelimiter(sign::B_CURLY_CLOSE))
			return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
		return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::FAILED_AND_DIED);
	}
	return ParsingResult<ASTCompoundStmt*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<IASTStmt*> Parser::parseWhileLoop()
{
	// Rule : <while_loop> 	= <wh_kw>  '(' <expr> ')'	<compound_statement> 
	if (matchKeyword(keyword::D_WHILE))
	{
		std::unique_ptr<ASTWhileStmt> rtr = std::make_unique<ASTWhileStmt>();
		// <parens_expr>
		if (auto parensExprRes = parseParensExpr(true)) // true -> parensExpr is mandatory.
			rtr->expr_ = std::move(parensExprRes.result_);
			//  no need for failure cases, the function parseParensExpr manages failures by itself when the mandatory flag is set.
		// <stmt>
		if (auto parseres = parseStmt())
			rtr->body_ = std::move(parseres.result_);
		else
		{
			errorExpected("Expected a Statement after while loop declaration,");
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
		}
		// Return
		return ParsingResult<IASTStmt*>(ParsingOutcome::SUCCESS, std::move(rtr));
	}
	return ParsingResult<IASTStmt*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<ASTFunctionDecl*> Parser::parseFunctionDeclaration()
{
	/* 
		<func_decl> = "func" <id> '(' [<arg_list_decl>] ')'[':' <type>] <compound_statement>	// Note about type_spec : if it is not present, the function returns void.
	*/
	// "func"
	if (matchKeyword(keyword::D_FUNC))
	{
		auto rtr = std::make_unique<ASTFunctionDecl>();
		// <id>
		if (auto mID_res = matchID())
			rtr->name_ = mID_res.result_;
		else
		{
			rtr->name_ = "<noname>";
			errorExpected("Expected an identifier");
		}

		// '('
		if (matchSign(sign::B_ROUND_OPEN))
		{
			// [<arg_list_decl>]
			auto pArgDeclList = parseArgDeclList();
			if (pArgDeclList)
				rtr->args_ = pArgDeclList.result_;
			// ')'
			if (!matchSign(sign::B_ROUND_CLOSE))
			{
				if (pArgDeclList.getFlag() != ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY)
					errorExpected("Expected a ')'");
				if(!resyncToDelimiter(sign::B_ROUND_CLOSE))
					return ParsingResult<ASTFunctionDecl*>(ParsingOutcome::FAILED_AND_DIED);
			}
		}
		else
		{
			errorExpected("Expected '('");
			if (!resyncToDelimiter(sign::B_ROUND_CLOSE))
				return ParsingResult<ASTFunctionDecl*>(ParsingOutcome::FAILED_AND_DIED);
		}
		// [':' <type>]
		if (matchSign(sign::P_COLON))
		{
			if (auto tyMatchRes = matchTypeKw())
				rtr->returnType_ = tyMatchRes.result_;
			else
				errorExpected("Expected a type keyword");
		}
		else
			rtr->returnType_ = TypeIndex::Void_Type;

		// <compound_statement>
		if (auto cp_res = parseCompoundStatement(true))
		{
			rtr->body_ = std::move(cp_res.result_);
			return ParsingResult<ASTFunctionDecl*>(ParsingOutcome::SUCCESS, std::move(rtr));
		}
		else
			return ParsingResult<ASTFunctionDecl*>(cp_res.getFlag());
	}
	return ParsingResult<ASTFunctionDecl*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<FoxFunctionArg> Parser::parseArgDecl()
{
	// <id>
	if (auto mID_res = matchID())
	{
		FoxFunctionArg rtr;
		rtr.name_ = mID_res.result_;
		// ':'
		if (!matchSign(sign::P_COLON))
		{
			errorExpected("Expected ':'");
			return ParsingResult<FoxFunctionArg>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
		}
		// ["const"]
		if (matchKeyword(keyword::T_CONST))
			rtr.type_.setConstAttribute(true);
		// ['&']
		if (matchSign(sign::S_AMPERSAND))
			rtr.isRef_ = true;
		else
			rtr.isRef_ = false;

		if (auto mty_res = matchTypeKw())
		{
			rtr.type_.setType(mty_res.result_);
			return ParsingResult<FoxFunctionArg>(ParsingOutcome::SUCCESS, rtr);
		}
		else
		{
			errorExpected("Expected type name");
			return ParsingResult<FoxFunctionArg>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
		}
	}
	return ParsingResult<FoxFunctionArg>(ParsingOutcome::NOTFOUND);
}

ParsingResult<std::vector<FoxFunctionArg>> Parser::parseArgDeclList()
{
	if (auto firstArg_res = parseArgDecl())
	{
		std::vector<FoxFunctionArg> rtr;
		rtr.push_back(firstArg_res.result_);
		while (true)
		{
			if (matchSign(sign::P_COMMA))
			{
				if (auto pArgDecl_res = parseArgDecl())
					rtr.push_back(pArgDecl_res.result_);
				else 
				{
					if (pArgDecl_res.getFlag() == ParsingOutcome::NOTFOUND)
						errorExpected("Expected an argument declaration");
					return ParsingResult<std::vector<FoxFunctionArg>>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
				}
			}
			else
				break;
		}
		return ParsingResult<std::vector<FoxFunctionArg>>(ParsingOutcome::SUCCESS,rtr);
	}
	else 
		return ParsingResult<std::vector<FoxFunctionArg>>(firstArg_res.getFlag());
}

ParsingResult<IASTStmt*> Parser::parseCondition()
{
	//<condition> = "if" <parens_expr> <statement> ["else" <statement>]
	auto rtr = std::make_unique<ASTCondStmt>();
	bool has_if = false;
	// "if"
	if (matchKeyword(keyword::D_IF))
	{
		// <parens_expr>
		if (auto parensExprRes = parseParensExpr(true)) // true -> parensExpr is mandatory.
			rtr->cond_ = std::move(parensExprRes.result_);

		// <statement>
		auto ifStmtRes = parseStmt();
		if (ifStmtRes)
			rtr->then_ = std::move(ifStmtRes.result_);
		else
		{
			errorExpected("Expected a statement after if condition,");
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
		}
		has_if = true;
	}
	// "else"
	if (matchKeyword(keyword::D_ELSE))
	{
		// <statement>
		if (auto stmt = parseStmt())
			rtr->else_ = std::move(stmt.result_);
		else
		{
			errorExpected("Expected a statement after else,");
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_WITHOUT_ATTEMPTING_RECOVERY);
		}
		if (!has_if)
			genericError("Else without matching if.");
	}
	if(has_if)
		return ParsingResult<IASTStmt*>(ParsingOutcome::SUCCESS, std::move(rtr));
	return ParsingResult<IASTStmt*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<IASTStmt*> Parser::parseStmt()
{
	// <stmt>	= <var_decl> | <expr_stmt> | <condition> | <while_loop> | <compound_statement> | (<rtr_stmt> -> to be implemented)
	if (auto parseres = parseExprStmt())
		return parseres;
	else if (auto parseres = parseVarDeclStmt())
		return parseres;
	else if (auto parseres = parseCondition())
		return parseres;
	else if (auto parseres = parseWhileLoop())
		return parseres;
	else if (auto parseres = parseCompoundStatement())
		return ParsingResult<IASTStmt*>(parseres.getFlag(), std::move(parseres.result_));
	else
		return ParsingResult<IASTStmt*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<IASTStmt*> Parser::parseVarDeclStmt()
{
	//<var_decl> = <let_kw> <id> <type_spec> ['=' <expr>] ';'
	std::unique_ptr<IASTExpr> initExpr = 0;

	bool isVarConst = false;
	FoxType varType = TypeIndex::InvalidIndex;
	std::string varName;

	if (matchKeyword(keyword::D_LET))
	{
		// ##ID##

		if (auto match = matchID())
		{
			varName = match.result_;
		}
		else
		{
			errorExpected("Expected an identifier");
			if (resyncToDelimiter(sign::P_SEMICOLON))
				return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_AND_DIED);
		}
		// ##TYPESPEC##
		if (auto typespecResult = parseTypeSpec())
			varType = typespecResult.result_;
		else
		{
			errorExpected("Expected a ':'");
			if (resyncToDelimiter(sign::P_SEMICOLON))
				return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_AND_DIED);
		}

		// ##ASSIGNEMENT##
		// '=' <expr>
		if (matchSign(sign::S_EQUAL))
		{
			if (auto parseres = parseExpr())
				initExpr = std::move(parseres.result_);
			else
			{
				errorExpected("Expected an expression");
				if (resyncToDelimiter(sign::P_SEMICOLON))
					return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
				return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_AND_DIED);
			}
		}
		// ';'
		if (!matchSign(sign::P_SEMICOLON))
		{
			errorExpected("Expected semicolon after expression in variable declaration,");
			if (resyncToDelimiter(sign::P_SEMICOLON))
				return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_AND_DIED);
		}

		// If parsing was ok : 
		FoxVariableAttr v_attr(varName, varType);
		if (initExpr) // Has init expr?
			return ParsingResult<IASTStmt*>(
				ParsingOutcome::SUCCESS,
				std::make_unique<ASTVarDecl>(v_attr,std::move(initExpr))
			);
		else
			return ParsingResult<IASTStmt*>(
				ParsingOutcome::SUCCESS,
				std::make_unique<ASTVarDecl>(v_attr, nullptr)
			);
	}
	return ParsingResult<IASTStmt*>(ParsingOutcome::NOTFOUND);
}

ParsingResult<FoxType> Parser::parseTypeSpec()
{
	bool isConst = false;
	if (matchSign(sign::P_COLON))
	{
		// Match const kw
		if (matchKeyword(keyword::T_CONST))
			isConst = true;
		// Now match the type keyword
		if (auto mTy_res = matchTypeKw())
			return ParsingResult<FoxType>(ParsingOutcome::SUCCESS, FoxType(mTy_res.result_, isConst));

		errorExpected("Expected a type name");
	}
	return ParsingResult<FoxType>(ParsingOutcome::NOTFOUND);
}

ParsingResult<IASTStmt*> Parser::parseExprStmt()
{
	//<expr_stmt> = ';' |<expr> ';'
	if (matchSign(sign::P_SEMICOLON))
		return ParsingResult<IASTStmt*>(ParsingOutcome::SUCCESS,
			std::make_unique<ASTNullStmt>()
		);
	else if (auto node = parseExpr())
	{
		// Found node
		if (matchSign(sign::P_SEMICOLON))
			return ParsingResult<IASTStmt*>(ParsingOutcome::SUCCESS,std::move(node.result_));
		else
		{
			errorExpected("Expected a ';' in expression statement");
			if (resyncToDelimiter(sign::P_SEMICOLON))
				return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_BUT_RECOVERED);
			return ParsingResult<IASTStmt*>(ParsingOutcome::FAILED_AND_DIED);
		}
	}
	return ParsingResult<IASTStmt*>(ParsingOutcome::NOTFOUND);
}