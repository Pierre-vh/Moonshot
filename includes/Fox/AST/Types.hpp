//----------------------------------------------------------------------------//
// This file is a part of The Moonshot Project.        
// See the LICENSE.txt file at the root of the project for license information.            
// File : Types.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file contains the TypeBase hierarchy.
//
//  Unlike Stmt/Decl/Expr hierarchies, this hierarchy is exclusively
//  allocated/created through static member functions. This is done because
//  most types are unique.
//
//----------------------------------------------------------------------------//

#pragma once

#include <string>
#include <cstdint>
#include <list>
#include "llvm/ADT/PointerIntPair.h"
#include "ASTAligns.hpp"
#include "Type.hpp"

namespace fox {
  // Kinds of Types
  enum class TypeKind : std::uint8_t {
    #define TYPE(ID,PARENT) ID,
    #define TYPE_RANGE(ID,FIRST,LAST) First_##ID = FIRST, Last_##ID = LAST,
    #include "TypeNodes.def"
  };

  // Forward Declarations
  class ASTContext;

  // TypeBase
  //    Common base for types
  //
  // Usually, you should never use raw TypeBase* pointers unless you have
  // a valid reason. Always use the Type wrapper. (see Type.hpp)
  class alignas(align::TypeBaseAlignement) TypeBase {
    public:
      /* Returns the type's name in a user friendly form, 
         e.g. "int", "string" */
      std::string toString() const;

      /* Returns the type's name in a developer-friendly form, 
         e.g. "Array(int)" instead of [int]" */
      std::string toDebugString() const;

      void dump() const;

      TypeKind getKind() const;

      // Returns true if this is a bound type.
      //
      // A bound type is a type that can be materialized. In short,
      // every type is a bound type except types that countains 
      // CellTypes with no substitution somewhere in their hierarchy.
      bool isBound() const;

      // Returns the element type if this is an ArrayType, otherwise returns
      // nullptr.
      Type unwrapIfArray();

      // If this type is an LValue, returns it's element type, else
      // returns this.
      Type getRValue();

      // If this is a bound type, ignores LValues and dereferences this type. 
      // If this is an unbound type, returns nullptr.
      Type getAsBoundRValue();

      // If this type is a CellType, dereference it recursively 
      // until we reach a CellType with no substitution or a
      // concrete type.
      Type deref();

      bool isStringType() const;
      bool isCharType() const;
      bool isFloatType() const;
      bool isBoolType() const;
      bool isIntType() const;
      bool isVoidType() const;

      template<typename Ty>
      bool is() const {
        return isa<Ty>(this);
      }

      template<typename Ty>
      const Ty* getAs() const {
        return dyn_cast<Ty>(this);
      }

      template<typename Ty>
      Ty* getAs() {
        return dyn_cast<Ty>(this);
      }

      template<typename Ty>
      const Ty* castTo() const {
        return cast<Ty>(this);
      }

      template<typename Ty>
      Ty* castTo() {
        return cast<Ty>(this);
      }

    protected:
      TypeBase(TypeKind tc);

      // Prohibit the use of builtin placement new & delete
      void *operator new(std::size_t) throw() = delete;
      void operator delete(void *) throw() = delete;
      void* operator new(std::size_t, void*) = delete;

      // Only allow allocation through the ASTContext
      // This operator is "protected" so only the ASTContext can create types.
      void* operator new(std::size_t sz, ASTContext &ctxt, 
      std::uint8_t align = alignof(TypeBase));

      // Companion operator delete to silence C4291 on MSVC
      void operator delete(void*, ASTContext&, std::uint8_t) {}
    
      // Calculates the value of isBound_ and set isBoundCalculated_ to true.
      void calculateIsBound() const;
      void setIsBound(bool val) const;

    private:
      void initBitfields();

      //------------Bitfields------------//
      // Cached values
      mutable bool isBound_ : 1;
      mutable bool isBoundCalculated_ : 1;
      //-----------6 Bits left-----------//

      const TypeKind kind_;
  };

  // BasicType
  //    Common base for "Basic" Types.
  //    A basic type is a type that can't be unwrapped any further.
  //    Every type is Fox is made of 1 or more Basic type used in conjuction
  //    with other types, such as the LValueType or the ArrayType.
  //  
  class BasicType : public TypeBase {
    public:
      static bool classof(const TypeBase* type) {
        return ((type->getKind() >= TypeKind::First_BasicType) 
          && (type->getKind() <= TypeKind::Last_BasicType));
      }

    protected:
      BasicType(TypeKind tc);
  };

  // PrimitiveType 
  //    A primitive type (void/int/float/char/bool/string)
  class PrimitiveType : public BasicType {
    public:
      enum class Kind : std::uint8_t {
        VoidTy,
        IntTy,
        FloatTy,
        CharTy,
        StringTy,
        BoolTy
      };

      static PrimitiveType* getString(ASTContext& ctxt);
      static PrimitiveType* getChar(ASTContext& ctxt);
      static PrimitiveType* getFloat(ASTContext& ctxt);
      static PrimitiveType* getBool(ASTContext& ctxt);
      static PrimitiveType* getInt(ASTContext& ctxt);
      static PrimitiveType* getVoid(ASTContext& ctxt);

      Kind getPrimitiveKind() const;

      static bool classof(const TypeBase* type) {
        return (type->getKind() == TypeKind::PrimitiveType);
      }

    private:
      PrimitiveType(Kind kd);

      const Kind builtinKind_;
  };

 // ErrorType
  //    A type used to represent that a expression's type
  //    cannot be determined because of an error.
  class ErrorType : public BasicType {
    public:
      // Gets the unique ErrorType instance for the current context.
      static ErrorType* get(ASTContext& ctxt);

      static bool classof(const TypeBase* type) {
        return (type->getKind() == TypeKind::ErrorType);
      }

    private:
      ErrorType();
  };

  // ArrayType
  //    An array of a certain type (can be any type, 
  //    even another ArrayType)
  class ArrayType : public TypeBase {
    public:
      // Returns the UNIQUE ArrayType instance for the given
      // type ty.
      static ArrayType* get(ASTContext& ctxt, Type ty);

      Type getElementType();
      const Type getElementType() const;

      static bool classof(const TypeBase* type) {
        return (type->getKind() == TypeKind::ArrayType);
      }

    private:
      ArrayType(Type elemTy);

      Type elementTy_= nullptr;
  };

  // LValueType
  //    C/C++-like LValue. e.g. This type is the one
  //    of a DeclRef when the declaration it refers to
  //    is not const.
  class LValueType : public TypeBase {
    public:
      // Returns the UNIQUE LValueType instance for the given type "ty"
      static LValueType* get(ASTContext& ctxt, Type ty);

      Type getType();
      const Type getType() const;

      static bool classof(const TypeBase* type) {
        return (type->getKind() == TypeKind::LValueType);
      }

    private:
      LValueType(Type type);

      Type ty_ = nullptr;
  };

  // CellType
  class CellType : public TypeBase {
    public:
      // Creates a new instance of the CellType class
      static CellType* create(ASTContext& ctxt);

      Type getSubst();
      const Type getSubst() const;

      // Returns true if the type has a substitution
      // (type isn't null)
      bool hasSubst() const;

      void setSubst(Type type);

      void reset();

      static bool classof(const TypeBase* type) {
        return (type->getKind() == TypeKind::CellType);
      }

    private:
      // Private because only called by ::create
      CellType();

      // Override the new operator to use the SemaAllocator in the
      // ASTContext to allocate CellTypes.
      void* operator new(std::size_t sz, ASTContext &ctxt, 
        std::uint8_t align = alignof(TypeBase));

      Type subst_ = nullptr;
  };

}
