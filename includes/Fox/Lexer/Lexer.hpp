//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : Lexer.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file contains the Fox Lexer.
//----------------------------------------------------------------------------//

#pragma once

#include "Token.hpp"
#include "Fox/Common/SourceLoc.hpp"
#include "Fox/Common/string_view.hpp"

namespace fox {
  class DiagnosticEngine;
  class SourceManager;
  class Lexer  {
    public:
      Lexer(SourceManager& srcMgr, DiagnosticEngine& diags);

      void lexFile(FileID file);
  
      TokenVector& getTokens();
      std::size_t numTokens() const;  

      DiagnosticEngine& diagEngine;
      SourceManager& sourceMgr;

    private:
      // Pushes the current token with the kind "kind"
      void pushTok(TokenKind kind);

      // Pushes the current token character as a token with the kind "kind" 
      // (calls resetToken() + pushToken())
      void beginAndPushToken(TokenKind kind);

      // Calls advance(), then pushes the current token with the kind 'kind'
      void advanceAndPushTok(TokenKind kind);

      // Returns true if we reached EOF.
      bool isEOF() const;

      // Begins a new token
      void resetToken();

      // Entry point of the lexing process
      void lex();

      // Lex an identifier or keyword
      void lexIdentifierOrKeyword();
      // Lex an int or double literal
      void lexIntOrDoubleConstant();
      // Lex any number of char items until we find a char that can't be
      // a char item, or if we find the (unescaped) delimiter or EOF.
      // Returns true if the delimiter was found
      bool lexCharItems(FoxChar delimiter);
      // Lex a piece of text delimited by single quotes '
      void lexSingleQuoteText();
      // Lex a piece of text delimited by double quotes "
      void lexDoubleQuoteText();
      // Lex an integer literal
      void lexIntConstant();

      // Handles a single-line comment
      void skipLineComment();
      // Handles a multi-line comment
      void skipBlockComment();

      // Returns false if the character cannot be considered a <char_item>
      bool canBeCharItem(FoxChar ch) const;

      // Returns true if 'ch' is a valid identifier head.
      bool isValidIdentifierHead(FoxChar ch) const;
      // Returns true if 'ch' is a valid identifier character.
      bool isValidIdentifierChar(FoxChar ch) const;

      // Returns the current character being considered
      FoxChar getCurChar() const;
      // Peeks the next character that will be considered
      FoxChar peekNextChar() const;

      // Advances to the next codepoint in the input.
      // Returns false if we reached EOF.
      bool advance();

      SourceLoc getLocOfPtr(const char* ptr) const;
      SourceLoc getCurPtrLoc() const;
      SourceLoc getCurtokBegLoc() const;
      SourceRange getCurtokRange() const;
      string_view getCurtokStringView() const;

      FileID fileID_;

      const char* fileBeg_  = nullptr;
      const char* fileEnd_ = nullptr;
      // The pointer to the first byte of the token
      const char* tokBegPtr_  = nullptr;
      // The pointer to the current character being considered.
      // (pointer to the first byte in the UTF8 codepoint)
      const char* curPtr_  = nullptr;

      TokenVector tokens_;
  };
}
