////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Parser.cpp										
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
//	This file implements methods that aren't tied to Expression,
//	Statements or Declarations.
////------------------------------------------------------////

#include "Parser.hpp"

#include <sstream>
#include <cassert>
#include <iostream>
#include "Moonshot/Fox/AST/IdentifierTable.hpp"
#include "Moonshot/Fox/Basic/Context.hpp"
#include "Moonshot/Fox/Basic/Exceptions.hpp"

using namespace Moonshot;

Parser::Parser(Context& c, ASTContext& astctxt, TokenVector& l) : context_(c), astcontext_(astctxt), tokens_(l)
{
	setupParser();
}

UnitParsingResult Parser::parseUnit()
{
	// <fox_unit>	= {<declaration>}1+
	auto unit = std::make_unique<ASTUnit>();

	// Create recovery "lock" object, since recovery is disabled for top level declarations. 
	// It'll be re-enabled by parseFunctionDeclaration
	auto lock = createRecoveryDisabler();

	// Gather some flags
	const bool showRecoveryMessages = context_.flagsManager.isSet(FlagID::parser_showRecoveryMessages);

	// Parse declarations 
	while (true)
	{
		// Parse a declaration
		auto decl = parseDecl();
		// If the declaration was parsed successfully : continue the cycle.
		if (decl.isUsable())
		{
			unit->addDecl(std::move(decl.result));
			continue;
		}
		else
		{
			// Parsing was successful & EOF -> Job's done !
			if (isDone())
				break;
			// No EOF? There's an unexpected token on the way, report it & recover!
			else
			{
				// Unlock, so we're allowed to recover here.
				auto unlock = createRecoveryEnabler();

				// Report an error in case of "not found";
				if (decl.wasSuccessful())	
					errorExpected("Expected a declaration");

				if (showRecoveryMessages)
					genericError("Attempting recovery to next declaration.");

				if (resyncToNextDecl())
				{
					if(showRecoveryMessages)
						genericError("Recovered successfully."); // Note : add a position, like "Recovered successfuly at line x"
					continue;
				}
				else
				{
					if(showRecoveryMessages)
						genericError("Couldn't recover.");
					break;
				}
			}
		}

	}

	if (parserState_.curlyBracketsCount)
		genericError(std::to_string(parserState_.curlyBracketsCount) + " '}' still missing after parsing this unit.");

	if (parserState_.roundBracketsCount)
		genericError(std::to_string(parserState_.roundBracketsCount) + " ')' still missing after parsing this unit.");

	if (parserState_.squareBracketsCount)
		genericError(std::to_string(parserState_.squareBracketsCount) + " ']' still missing after parsing this unit.");

	if (unit->getDeclCount() == 0)
	{
		// Unit reports an error if notfound, because it's a mandatory rule.
		genericError("Expected one or more declaration in unit.");
		// Return empty result
		return UnitParsingResult();
	}
	else
		return UnitParsingResult(std::move(unit));
}

void Parser::setupParser()
{
	parserState_.tokenIterator = tokens_.begin();
	lastUnexpectedTokenIt_ = tokens_.begin();
}

IdentifierInfo* Parser::consumeIdentifier()
{
	Token t = getCurtok();
	if (t.isIdentifier())
	{
		consumeAny();

		IdentifierInfo* ptr = t.getIdentifierInfo();;
		assert(ptr && "Token's an identifier but contains a nullptr IdentifierInfo?");
		return ptr;
	}
	return nullptr;
}

bool Parser::consumeSign(const SignType & s)
{
	assert(!isBracket(s) && "This method shouldn't be used to match brackets ! Use consumeBracket instead!");
	if (getCurtok().is(s))
	{
		consumeAny();
		return true;
	}
	return false;
}

bool Parser::consumeBracket(const SignType & s)
{
	assert(isBracket(s) && "This method should only be used on brackets ! Use consumeSign to match instead!");
	auto tok = getCurtok();
	if (tok.isSign())
	{
		if (!tok.is(s))
			return false;
		switch (s)
		{
			case SignType::S_CURLY_OPEN:
				if (parserState_.curlyBracketsCount < kMaxBraceDepth)
					parserState_.curlyBracketsCount++;
				else
					throw std::overflow_error("Max Brackets Depth Exceeded");
				break;
			case SignType::S_CURLY_CLOSE:
				if (parserState_.curlyBracketsCount)		// Don't let unbalanced parentheses create an underflow.
					parserState_.curlyBracketsCount--;
				break;
			case SignType::S_SQ_OPEN:
				if (parserState_.squareBracketsCount < kMaxBraceDepth)
					parserState_.squareBracketsCount++;
				else
					throw std::overflow_error("Max Brackets Depth Exceeded");
				break;
			case SignType::S_SQ_CLOSE:
				if (parserState_.squareBracketsCount)		// Don't let unbalanced parentheses create an underflow.
					parserState_.squareBracketsCount--;
				break;
			case SignType::S_ROUND_OPEN:
				if (parserState_.roundBracketsCount < kMaxBraceDepth)
					parserState_.roundBracketsCount++;
				else
					throw std::overflow_error("Max Brackets Depth Exceeded");
				break;
			case SignType::S_ROUND_CLOSE:
				if (parserState_.roundBracketsCount)		// Don't let unbalanced parentheses create an underflow.
					parserState_.roundBracketsCount--;
				break;
			default:
				throw std::exception("Unknown bracket type"); // Should be unreachable.
		}
		consumeAny();
		return true;
	}
	return false;
}

bool Parser::consumeKeyword(const KeywordType & k)
{
	if (getCurtok().is(k))
	{
		consumeAny();
		return true;
	}
	return false;
}

void Parser::consumeAny(char n)
{
	for (; (n > 0) && (parserState_.tokenIterator != tokens_.end());n--)
		parserState_.tokenIterator++;
}

void Parser::revertConsume(char n)
{
	for (; (n > 0) && (parserState_.tokenIterator != tokens_.begin()); n--)
		parserState_.tokenIterator--;
}

bool Parser::isBracket(const SignType & s) const
{
	switch (s)
	{
		case SignType::S_CURLY_OPEN:
		case SignType::S_CURLY_CLOSE:
		case SignType::S_SQ_OPEN:
		case SignType::S_SQ_CLOSE:
		case SignType::S_ROUND_OPEN:
		case SignType::S_ROUND_CLOSE:
			return true;
		default:
			return false;
	}
}

const Type* Parser::parseBuiltinTypename()
{
	// <builtin_type_name> 	= "int" | "float" | "bool" | "string" | "char"
	Token t = getCurtok();
	if (t.isKeyword())
	{
		consumeAny();
		switch (t.getKeywordType())
		{
			case KeywordType::KW_INT:	return  astcontext_.getPrimitiveIntType();
			case KeywordType::KW_FLOAT:	return  astcontext_.getPrimitiveFloatType();
			case KeywordType::KW_CHAR:	return	astcontext_.getPrimitiveCharType();
			case KeywordType::KW_STRING:return	astcontext_.getPrimitiveStringType();
			case KeywordType::KW_BOOL:	return	astcontext_.getPrimitiveBoolType();
		}
		revertConsume();
	}
	return nullptr;
}

std::pair<const Type*, bool> Parser::parseType()
{
	// <type> = <builtin_type_name> { '[' ']' }
	// <builtin_type_name> 
	if (auto ty = parseBuiltinTypename())
	{
		//  { '[' ']' }
		while (consumeBracket(SignType::S_SQ_OPEN))
		{
			// Set ty to the ArrayType of ty.
			ty = astcontext_.getArrayTypeForType(ty);
			// ']'
			if (!consumeBracket(SignType::S_SQ_CLOSE))
			{
				errorExpected("Expected ']'");
				// Try to recover
				if (resyncToSign(SignType::S_SQ_CLOSE,/*stopAtSemi */ true ,/*shouldConsumeToken*/ true))
					continue;
				// else, return an error.
				return { nullptr , false };
			}
		}
		// found, return
		return { ty, true };
	}
	// notfound, return
	return { nullptr, true };
}

Token& Parser::getCurtok()
{
	if (!isDone())
		return *(parserState_.tokenIterator);
	return nullTok_;
}

bool Parser::resyncToSign(const SignType & sign, const bool & stopAtSemi, const bool & shouldConsumeToken)
{
	return resyncToSign(std::vector<SignType>({ sign }), stopAtSemi, shouldConsumeToken);
}

bool Parser::resyncToSign(const std::vector<SignType>& signs, const bool & stopAtSemi, const bool & shouldConsumeToken)
{
	// Note, this function is heavily based on CLang's http://clang.llvm.org/doxygen/Parse_2Parser_8cpp_source.html#l00245

	// Return immediately if recovery is not allowed
	if (!parserState_.isRecoveryAllowed)
		return false;

	// Always skip the first token if it's not in signs
	bool isFirst = true;
	// Keep going until we reach EOF.
	for(;!isDone();consumeAny())
	{
		// Check curtok
		auto tok = getCurtok();
		for (auto it = signs.begin(); it != signs.end(); it++)
		{
			if (tok.is(*it))
			{
				if (shouldConsumeToken)
				{
					// if it's a bracket, pay attention to it!
					if (isBracket(*it))
						consumeBracket(*it);
					else
						consumeAny();
				}
				return true;
			}
		}
		// Check isFirst
		if (isFirst)
		{
			isFirst = false;
			continue;
		}

		// Check if it's a sign for special behaviours
		if (tok.isSign())
		{
			switch (tok.getSignType())
			{
				// If we find a '(', '{' or '[', call this function recursively to match it's counterpart
				case SignType::S_CURLY_OPEN:
					resyncToSign(SignType::S_CURLY_CLOSE, false, true);
					break;
				case SignType::S_SQ_OPEN:
					resyncToSign(SignType::S_SQ_CLOSE, false, true);
					break;
				case SignType::S_ROUND_OPEN:
					resyncToSign(SignType::S_ROUND_CLOSE, false, true);
					break;
				// If we find a ')', '}' or ']' we  :
					// Check if it belongs to a unmatched counterpart, if so, stop resync attempt.
					// If it doesn't have an opening counterpart skip it.
				case SignType::S_CURLY_CLOSE:
					if (parserState_.curlyBracketsCount)
						return false;
					consumeBracket(SignType::S_CURLY_CLOSE);
					break;
				case SignType::S_SQ_CLOSE:
					if (parserState_.squareBracketsCount)
						return false;
					consumeBracket(SignType::S_SQ_CLOSE);
					break;
				case SignType::S_ROUND_CLOSE:
					if (parserState_.roundBracketsCount)
						return false;
					consumeBracket(SignType::S_ROUND_CLOSE);
					break;
				case SignType::S_SEMICOLON:
					if (stopAtSemi)
						return false;
					break;
			}
		}
	}
	// If reached eof, die & return false.
	die();
	return false;
}

bool Parser::resyncToNextDecl()
{
	// This method skips everything until it finds a "let" or a "func".

	// Return immediately if recovery is not allowed
	if (!parserState_.isRecoveryAllowed)
		return false;

	// Keep on going until we find a "func" or "let"
	for (; !isDone(); consumeAny())
	{
		auto tok = getCurtok();
		// if it's let/func, return.
		if (tok.isKeyword())
		{
			if ((tok.getKeywordType() == KeywordType::KW_FUNC) && (tok.getKeywordType() == KeywordType::KW_LET))
				return true;
		}

		// Check if it's a sign for special brackets-related actions
		if (tok.isSign())
		{
			switch (tok.getSignType())
			{
				// If we find a '(', '{' or '[', call resyncToSign to match it's counterpart
				case SignType::S_CURLY_OPEN:
					resyncToSign(SignType::S_CURLY_CLOSE, false, true);
					break;
				case SignType::S_SQ_OPEN:
					resyncToSign(SignType::S_SQ_CLOSE, false, true);
					break;
				case SignType::S_ROUND_OPEN:
					resyncToSign(SignType::S_ROUND_CLOSE, false, true);
					break;
				// If we find a ')', '}' or ']' we match (consume) it.
				case SignType::S_CURLY_CLOSE:
					consumeBracket(SignType::S_CURLY_CLOSE);
					break;
				case SignType::S_SQ_CLOSE:
					consumeBracket(SignType::S_SQ_CLOSE);
					break;
				case SignType::S_ROUND_CLOSE:
					consumeBracket(SignType::S_ROUND_CLOSE);
					break;
			}
		}
	}
	// If reached eof, die & return false.
	die();
	return false;
}

void Parser::die()
{
	if(context_.flagsManager.isSet(FlagID::parser_showRecoveryMessages))
		genericError("Couldn't recover from error, stopping parsing.");

	parserState_.tokenIterator = tokens_.end();
	parserState_.isAlive = false;
}

void Parser::errorUnexpected()
{
	if (!parserState_.isAlive) return;

	context_.setOrigin("Parser");

	std::stringstream output;
	auto tok = getCurtok();
	if (tok)
	{
		output << "Unexpected token \"" << tok.getAsString() << "\" at line " << tok.getPosition().line;
		context_.reportError(output.str());
	}
	context_.resetOrigin();
}

void Parser::errorExpected(const std::string & s)
{
	if (!parserState_.isAlive) return;

	TokenIteratorTy lastTokenIter = parserState_.tokenIterator;
	if (lastTokenIter != tokens_.begin())
		lastTokenIter--;

	// If needed, print unexpected error message
	if (lastUnexpectedTokenIt_ != parserState_.tokenIterator)
	{
		lastUnexpectedTokenIt_ = parserState_.tokenIterator;
		errorUnexpected();
	}

	context_.setOrigin("Parser");

	std::stringstream output;
	
	output << s << " after \"" << lastTokenIter->getAsString() << "\" at line " << lastTokenIter->getPosition().line;

	context_.reportError(output.str());
	context_.resetOrigin();
}

void Parser::genericError(const std::string & s)
{
	if (!parserState_.isAlive) return;

	context_.setOrigin("Parser");
	context_.reportError(s);
	context_.resetOrigin();
}

bool Parser::isDone() const
{
	return (parserState_.tokenIterator == tokens_.end());
}

bool Parser::isAlive() const
{
	return parserState_.isAlive;
}

Parser::ParserState Parser::createParserStateBackup() const
{
	return parserState_;
}

void Parser::restoreParserStateFromBackup(const Parser::ParserState & st)
{
	parserState_ = st;
}

// ParserState
Parser::ParserState::ParserState() : isAlive(true), isRecoveryAllowed(false)
{

}

// RAIIRecoveryManager
Parser::RAIIRecoveryManager::RAIIRecoveryManager(Parser &parser, const bool & allowsRecovery) : parser_(parser)
{
	recoveryAllowedBackup_ = parser_.parserState_.isRecoveryAllowed;
	parser_.parserState_.isRecoveryAllowed = allowsRecovery;
}

Parser::RAIIRecoveryManager::~RAIIRecoveryManager()
{
	parser_.parserState_.isRecoveryAllowed = recoveryAllowedBackup_;
}

Parser::RAIIRecoveryManager Parser::createRecoveryEnabler()
{
	return RAIIRecoveryManager(*this,true);
}

Parser:: RAIIRecoveryManager Parser::createRecoveryDisabler()
{
	return RAIIRecoveryManager(*this, false);
}