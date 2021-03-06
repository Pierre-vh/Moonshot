//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : Token.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file contains the Token class and TokenKind enum.
//----------------------------------------------------------------------------//

#pragma once

#include "Fox/AST/Identifier.hpp"
#include "Fox/Common/FoxTypes.hpp"
#include "Fox/Common/SourceLoc.hpp"
#include "Fox/Common/LLVM.hpp"
#include "llvm/ADT/SmallVector.h"
#include <cstddef>
#include <iosfwd>

namespace fox {
  class ASTContext;
  class DiagnosticEngine;
  class SourceManager;

  /// TokenKind
  ///    The different kind of tokens that exist
  enum class TokenKind : std::uint8_t {
    #define TOKEN(ID) ID,
    #include "TokenKinds.def"
  };

  /// Token
  ///    Provides information about a lexed token: its string, location and kind.
  struct Token  {
    public:
      using Kind = TokenKind;
      /// Creates an invalid token
      Token() = default;
      /// Creates a normal token
      Token(Kind kind, string_view str, SourceRange range);

      /// \returns true if this token's kind != TokenKind::Invalid
      bool isValid() const;
      /// \returns true if this token is the EOF token
      bool isEOF() const;
      /// \returns isValid()
      operator bool() const;

      /// \returns true if this token's kind matches "kind"
      bool is(Kind kind) const;

      /// dumps this token's data to out (without loc info)
      void dump(std::ostream& out) const;

      /// dumps this token's data to out (with loc info)
      void dump(std::ostream& out, SourceManager& srcMgr, 
        bool printFileName) const;

      /// The SourceRange of this token
      const SourceRange range;

      /// A string-view (in the file's buffer) of this token.
      const string_view str;

      /// The Kind of token this is
      const Kind kind = Kind::Invalid;
  };

  /// A Vector of Tokens.
  using TokenVector = SmallVector<Token, 4>;
}
