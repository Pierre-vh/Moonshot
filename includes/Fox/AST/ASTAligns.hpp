//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : ASTAligns.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file contains various constants to be used as the minimum
// alignment for some AST classes
//----------------------------------------------------------------------------//

#pragma once

#include <cstddef>
#include "ASTFwdDecl.hpp"

namespace fox {
  // Declare the FreeBits and Alignment variables
  // Usage of DECLARE: DECLARE(Class name, Number of free bits desired)
  #define DECLARE(CLASS, FREE_BITS_DESIRED)\
  constexpr std::size_t CLASS##FreeLowBits = FREE_BITS_DESIRED##u; \
  constexpr std::size_t CLASS##Alignement = 1u << FREE_BITS_DESIRED##u

  DECLARE(TypeBase, 1);
  DECLARE(Expr, 3);
  DECLARE(Decl, 3);
  DECLARE(Stmt, 3);
  DECLARE(DeclContext, 3);
  #undef DECLARE
}

// Specialize llvm::PointerLikeTypeTraits for each class.
// This is important for multiple LLVM ADT classes, such as
// PointerUnion
namespace llvm {
  template <class T> struct PointerLikeTypeTraits;
  #define LLVM_DEFINE_PLTT(CLASS) \
  template <> struct PointerLikeTypeTraits<::fox::CLASS*> { \
    enum { NumLowBitsAvailable = ::fox::CLASS##FreeLowBits }; \
    static inline void* getAsVoidPointer(::fox::CLASS* ptr) {return ptr;} \
    static inline ::fox::CLASS* getFromVoidPointer(void* ptr) \
    {return static_cast<::fox::CLASS*>(ptr);} \
  }

  LLVM_DEFINE_PLTT(TypeBase);
  LLVM_DEFINE_PLTT(Expr);
  LLVM_DEFINE_PLTT(Decl);
  LLVM_DEFINE_PLTT(Stmt);
  LLVM_DEFINE_PLTT(DeclContext);

  #undef LLVM_DEFINE_PLTT

  // For PointerLikeTypeTraits
  static_assert(alignof(void*) >= 2, "void* pointer alignment too small");
}

