////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Lexer.h											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file declares the lexer class.						
//															
// The lexer is the 1st step of the interpretation process.
// It takes the source file, in the form of a string, as input, and outputs a std::<vector> of token.				
//															
// It performs a lexical analysis. A Fairly simple one in our case, using a DFA. (state machine)
//															
//															
// Tokens (see Token.h/.cpp for declaration and definition is the dissected entry, separated in small bits			
// each "bit" is identified to recognize one of the main types : keywords,identifiers,values,etc..				
////------------------------------------------------------////

#pragma once

#include <string>		// std::string
#include <cwctype>		// std::iswspace
#include <vector>		// std::vector
#include <functional>	// std::function
#include <sstream>		// std::stringstream (sizeToStr())
#include <map>			// std::map
#include <memory>

#include "../../Common/Context/Context.h"
#include "../../Common/Exceptions/Exceptions.h"

#include "Token.h"

namespace Moonshot
{
	enum class dfaState
	{
		S_BASE, // basestate
		S_STR,	// string literals
		S_LCOM,	// line comment
		S_MCOM,	// multiline comment
		S_WORDS,// basic (keywords,signs) state
		S_CHR	// char literals
	};
	class Lexer 
	{
		public:
			Lexer(Context& curctxt);
			~Lexer();

			void lexStr(const std::string &data);		// Main function.
			
			void iterateResults(std::function<void(const token&)> func);	// Some function that could be useful one day : takes a lambda with a token as argument.
																			// The lambda will then be called with each token of the output, in order.
			void logAllTokens() const;					// log all token using E_LOG. Useful for debugging.

			token getToken(const std::size_t &vtpos) const;	// returns the n th token in result_
			std::size_t resultSize() const;					// returns result_.size()
		
			// Options struct
			struct options_struct
			{
				bool logPushedTokens = false;
				bool logTotalTokensCount = false;
			}options;
		private:

			void pushTok();					// push token
			void cycle();					// one dfa "cycle";
			void runFinalChecks();			// runs the final checks. this is called after the lexing process ended.
			// DFA state functions. I split this into various functions to make the code more readable in the cycle() function.
			void runStateFunc();			// call the correct function, depending on cstate_
			void dfa_goto(const dfaState &ns); 	// Go to state X (changes cstate)
			// States functions
			void fn_S_BASE();
			void fn_S_STR();	// string literals
			void fn_S_LCOM();	// one line comment
			void fn_S_MCOM();	// Multiple line comments
			void fn_S_WORDS();	// "basic" state (push to tok until separator is met)
			void fn_S_CHR();	// Char literals

			// Utils
			char eatChar();										// returns the current char and run updatePos (returns inputstr_[pos_] and do pos_+=1)
			void addToCurtok(const char &c);					// adds the current character to curtok_
			bool isSep(const char &c) const;					// is the current char a separator? (= a sign. see kSign_dict)
			char peekNext() const;								// peeks the next character
			bool isEscapeChar(const char& c) const;				// Checks if C is \ AND if the state is adequate for it to be qualified as an escape char.
			bool shouldIgnore(const char& c) const;				// Checks if the char is valid to be pushed. If it isn't and it should be ignored, returns true

			// error management made easy
			void reportLexerError(std::string errmsg) const;

			// Member Variables
				// Context
				Context& context_;
				// Utilities
				bool		escapeFlag_ = false;			// escaping with backslash flag
				dfaState	cstate_ = dfaState::S_BASE;		// curren dfa state. begins at S_BASE;
				std::string inputstr_;					// the input
				size_t		pos_ = 0;					// position in the input string;
				std::string curtok_;					// the token that's being constructed.
				text_pos	ccoord_;					// current coordinates.
				// Output
				std::vector<token>	result_;		// the lexer's output !
	};
}
