////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ParseDecl.cpp										
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
// This file implements decl, declstmt rules (methods)
// and related helper functions
////------------------------------------------------------////

#include "Parser.hpp"
#include "Fox/AST/ASTContext.hpp"

using namespace fox;


UnitDecl* Parser::parseUnit(const FileID& fid, IdentifierInfo* unitName, const bool& isMainUnit)
{
	// <fox_unit>	= {<declaration>}1+

	// Assert that unitName != nullptr
	assert(unitName && "Unit name cannot be nullptr!");
	assert(fid && "FileID cannot be invalid!");

	// Create the unit
	auto* unit = new(ctxt_) UnitDecl(unitName, fid);

	// Create a RAIIDeclContext
	RAIIDeclContext raiidr(*this, unit);

	bool declHadError = false;

	// Parse declarations 
	while (true)
	{
		if (auto decl = parseDecl())
		{
			unit->addDecl(decl.get());
			continue;
		}
		else
		{
			if (!decl.wasSuccessful())
				declHadError = true;

			// EOF/Died -> Break.
			if (isDone())
				break;
			// No EOF? There's an unexpected token on the way that prevents us from finding the decl.
			else
			{
				// Report an error in case of "not found";
				if (decl.wasSuccessful())
				{
					// Report the error with the current token being the error location
					Token curtok = getCurtok();
					assert(curtok && "Curtok must be valid since we have not reached eof");
					diags_.report(DiagID::parser_expected_decl, curtok.getRange());
				}

				if (resyncToNextDecl())
					continue;
				else
					break;
			}
		}

	}

	if (unit->getDeclCount() == 0)
	{
		if(!declHadError)
			diags_.report(DiagID::parser_expected_decl_in_unit, fid);
		return nullptr;
	}
	else
	{
		assert(unit->isValid());
		ctxt_.addUnit(unit, isMainUnit);
		return unit;
	}
}

Parser::DeclResult Parser::parseFuncDecl()
{
	/*
		<func_decl>	= "func" <id> '(' [<param_decl> {',' <param_decl>}*] ')'[':' <type>] <compound_statement>
		// Note about [':' <type>], if it isn't present, the function returns void
	*/

	// FIXME:
		// Improve the error recovery on a missing '(' or ')' 

	// "func"
	auto fnKw = consumeKeyword(KeywordType::KW_FUNC);
	if (!fnKw)
		return DeclResult::NotFound();

	auto* rtr = new(ctxt_) FuncDecl();
	SourceLoc begLoc = fnKw.getBegin();
	SourceLoc headEndLoc;

	// Boolean that's set to false if the declaration is not
	// valid. If set to false, we return an error instead of a result.
	// This is done because function decls are pretty big, and if we were
	// to abandon immediately on first error (e.g. missing identifier)
	// the recovery could be tough.
	bool isValid = true;
	bool foundLeftRoundBracket = true;

	// <id>
	if (auto foundID = consumeIdentifier())
		rtr->setIdentifier(foundID.get());
	else
	{
		reportErrorExpected(DiagID::parser_expected_iden);
		isValid = false;
		rtr->setIdentifier(identifiers_.getInvalidID());
	}

	// Before creating a RAIIDeclContext, record this function in the parent DeclContext 
	// (only if it's valid)
	if(isValid)
		recordDecl(rtr);

	// Create a RAIIDeclContext to record every decl within this function
	RAIIDeclContext raiiDC(*this, rtr);

	// '('
	if (!consumeBracket(SignType::S_ROUND_OPEN))
	{
		// Report the error only if we found the identifier
		// earlier.
		if(isValid)
			reportErrorExpected(DiagID::parser_expected_opening_roundbracket);
		isValid = foundLeftRoundBracket = false;
	}

	// [<param_decl> {',' <param_decl>}*]
	if (auto first = parseParamDecl())
	{
		rtr->addParam(first.getAs<ParamDecl>());
		while (true)
		{
			if (consumeSign(SignType::S_COMMA))
			{
				if (auto param = parseParamDecl())
					rtr->addParam(param.getAs<ParamDecl>());
				else if(param.wasSuccessful()) 
					reportErrorExpected(DiagID::parser_expected_argdecl);
			}
			else
				break;
		}
	}

	// ')'
	if (auto rightParens = consumeBracket(SignType::S_ROUND_CLOSE))
		headEndLoc = rightParens;
	else 
	{
		isValid = false;
		reportErrorExpected(DiagID::parser_expected_closing_roundbracket);

		// no '(' and no ')' -> abandon
		if (!foundLeftRoundBracket) 
			return DeclResult::Error();

		// We'll attempt to recover to the '{' too, so if we find the body of the function
		// we can at least parse that.
		if (!resyncToSign(SignType::S_ROUND_CLOSE, /* stopAtSemi */ true, /*consumeToken*/ false))
			return DeclResult::Error();
		headEndLoc = consumeBracket(SignType::S_ROUND_CLOSE);
	}
	
	// [':' <type>]
	if (auto colon = consumeSign(SignType::S_COLON))
	{
		if (auto rtrTy = parseType())
		{
			rtr->setReturnType(rtrTy.get());
			headEndLoc = rtrTy.getSourceRange().getEnd();
		}
		else 
		{
			isValid = false;

			if (rtrTy.wasSuccessful())
				reportErrorExpected(DiagID::parser_expected_type);

			if (!resyncToSign(SignType::S_CURLY_OPEN, true, false))
				return DeclResult::Error();
			// If resynced successfully, use the colon as the end of the header
			headEndLoc = colon;
		}
	}
	else // if no return type, the function returns void.
		rtr->setReturnType(ctxt_.getVoidType());

	// <compound_statement>
	auto compStmt = parseCompoundStatement(/* mandatory = yes */ true);

	if (!compStmt || !isValid)
		return DeclResult::Error();

	auto* body = dyn_cast<CompoundStmt>(compStmt.get());
	assert(body && "Not a compound stmt");

	SourceRange range(begLoc, body->getRange().getEnd());
	assert(headEndLoc && range && "Invalid loc info");

	rtr->setBody(body);
	rtr->setLocs(range, headEndLoc);
	assert(rtr->isValid());
	return DeclResult(rtr);
}

Parser::DeclResult Parser::parseParamDecl()
{
	// <param_decl> = <id> ':' <qualtype>

	// <id>
	auto id = consumeIdentifier();
	if (!id)
		return DeclResult::NotFound();

	// ':'
	if (!consumeSign(SignType::S_COLON))
	{
		reportErrorExpected(DiagID::parser_expected_colon);
		return DeclResult::Error();
	}

	// <qualtype>
	auto qt = parseQualType();
	if (!qt)
	{
		if (qt.wasSuccessful())
			reportErrorExpected(DiagID::parser_expected_type);
		return DeclResult::Error();
	}

	SourceLoc begLoc = id.getSourceRange().getBegin();
	SourceLoc endLoc = qt.getRange().getEnd();

	SourceRange range(begLoc, endLoc);
	assert(range && "Invalid loc info");

	auto* rtr = new(ctxt_) ParamDecl(
			id.get(),
			qt.get(),
			range,
			qt.getRange()
		);
	assert(rtr->isValid());
	recordDecl(rtr);
	return DeclResult(rtr);
}

Parser::DeclResult Parser::parseVarDecl()
{
	// <var_decl> = "let" <id> ':' <qualtype> ['=' <expr>] ';'
	// "let"
	auto letKw = consumeKeyword(KeywordType::KW_LET);
	if (!letKw)
		return DeclResult::NotFound();
	
	SourceLoc begLoc = letKw.getBegin();
	SourceLoc endLoc;
	SourceRange tyRange;

	IdentifierInfo* id;
	QualType ty;
	Expr* iExpr = nullptr;

	// <id>
	if (auto foundID = consumeIdentifier())
		id = foundID.get();
	else
	{
		reportErrorExpected(DiagID::parser_expected_iden);
		if (auto res = resyncToSign(SignType::S_SEMICOLON, /* stopAtSemi (true/false doesn't matter when we're looking for a semi) */ false, /*consumeToken*/ true))
		{
			// Recovered? Act like nothing happened.
			return DeclResult::NotFound();
		}
		return DeclResult::Error();
	}

	// ':'
	if (!consumeSign(SignType::S_COLON))
	{
		reportErrorExpected(DiagID::parser_expected_colon);
		return DeclResult::Error();
	}

	// <qualtype>
	if (auto typespecResult = parseQualType())
	{
		ty = typespecResult.get();
		tyRange = typespecResult.getRange();
		if (ty.isReference())
		{
			diags_.report(DiagID::parser_ignored_ref_vardecl, typespecResult.getRange());
			ty.setIsReference(false);
		}
	}
	else
	{
		if (typespecResult.wasSuccessful())
			reportErrorExpected(DiagID::parser_expected_type);
		if (auto res = resyncToSign(SignType::S_SEMICOLON, /*stopAtSemi*/ true, /*consumeToken*/ true))
			return DeclResult::NotFound(); // Recovered? Act like nothing happened.
		return DeclResult::Error();
	}

	// ['=' <expr>]
	if (consumeSign(SignType::S_EQUAL))
	{
		if (auto expr = parseExpr())
			iExpr = expr.get();
		else
		{
			if (expr.wasSuccessful())
				reportErrorExpected(DiagID::parser_expected_expr);
			// Recover to semicolon, return if recovery wasn't successful 
			if (!resyncToSign(SignType::S_SEMICOLON, /*stopAtSemi (true/false doesn't matter when we're looking for a semi)*/ false, /*consumeToken*/ false))
				return DeclResult::Error();
		}
	}

	// ';'
	endLoc = consumeSign(SignType::S_SEMICOLON);
	if (!endLoc)
	{
		reportErrorExpected(DiagID::parser_expected_semi);
			
		if (!resyncToSign(SignType::S_SEMICOLON, /*stopAtSemi*/ false, /*consumeToken*/ false))
			return DeclResult::Error();

		endLoc = consumeSign(SignType::S_SEMICOLON);
	}

	SourceRange range(begLoc, endLoc);
	assert(range && "Invalid loc info");

	auto rtr = new(ctxt_) VarDecl(id, ty, iExpr, range, tyRange);
	assert(rtr->isValid());
	recordDecl(rtr);
	return DeclResult(rtr);
}

Parser::Result<QualType> Parser::parseQualType()
{
	// 	<qualtype>	= ["const"] ['&'] <type>
	QualType ty;
	bool hasFoundSomething = false;
	SourceLoc begLoc;
	SourceLoc endLoc;

	// ["const"]
	if (auto kw = consumeKeyword(KeywordType::KW_CONST))
	{
		begLoc = kw.getBegin();
		hasFoundSomething = true;
		ty.setIsConst(true);
	}

	// ['&']
	if (auto ampersand = consumeSign(SignType::S_AMPERSAND))
	{
		// If no begLoc, the begLoc is the ampersand.
		if (!begLoc)
			begLoc = ampersand;
		hasFoundSomething = true;
		ty.setIsReference(true);
	}

	// <type>
	if (auto type = parseType())
	{
		ty.setType(type.get());

		// If no begLoc, the begLoc is the type's begLoc.
		if (!begLoc)
			begLoc = type.getSourceRange().getBegin();

		endLoc = type.getSourceRange().getEnd();
	}
	else
	{
		if (hasFoundSomething)
		{
			if (type.wasSuccessful())
				reportErrorExpected(DiagID::parser_expected_type);
			return Result<QualType>::Error();
		}
		else 
			return Result<QualType>::NotFound();
	}

	// Success!
	assert(ty && "Type cannot be null");
	assert(begLoc && "begLoc must be valid");
	assert(endLoc && "endLoc must be valid");
	return Result<QualType>(ty, SourceRange(begLoc,endLoc));
}

Parser::DeclResult Parser::parseDecl()
{
	// <declaration> = <var_decl> | <func_decl>

	// <var_decl>
	if (auto vdecl = parseVarDecl())
		return vdecl;
	else if (!vdecl.wasSuccessful())
		return DeclResult::Error();

	// <func_decl>
	if (auto fdecl = parseFuncDecl())
		return fdecl;
	else if (!fdecl.wasSuccessful())
		return DeclResult::Error();

	return DeclResult::NotFound();
}