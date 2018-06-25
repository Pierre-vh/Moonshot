////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ASTContext.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "ASTContext.hpp"

using namespace fox;

ASTContext::ASTContext()
{
	// Init builtin types
	initBuiltinTypes();
}

UnitDecl * ASTContext::getMainUnit()
{
	return mainUnit_;
}

UnitDecl * ASTContext::addUnit(std::unique_ptr<UnitDecl> unit, const bool& isMainUnit)
{
	units_.emplace_back(std::move(unit));

	auto ptr = units_.back().get();
	if (isMainUnit)
		mainUnit_ = ptr;
	return ptr;
}

PrimitiveType* ASTContext::getPrimitiveIntType()
{
	return primitiveIntTy_.get();
}

PrimitiveType* ASTContext::getPrimitiveFloatType()
{
	return primitiveFloatTy_.get();
}

PrimitiveType* ASTContext::getPrimitiveCharType()
{
	return primitiveCharTy_.get();
}

PrimitiveType* ASTContext::getPrimitiveBoolType()
{
	return primitiveBoolTy_.get();
}

PrimitiveType* ASTContext::getPrimitiveStringType()
{
	return primitiveStringTy_.get();
}

PrimitiveType* ASTContext::getPrimitiveVoidType()
{
	return primitiveVoidTy_.get();
}

void ASTContext::initBuiltinTypes()
{
	primitiveVoidTy_	= std::make_unique<PrimitiveType>(PrimitiveType::Kind::VoidTy);

	primitiveIntTy_		= std::make_unique<PrimitiveType>(PrimitiveType::Kind::IntTy);
	primitiveFloatTy_	= std::make_unique<PrimitiveType>(PrimitiveType::Kind::FloatTy);
	primitiveBoolTy_	= std::make_unique<PrimitiveType>(PrimitiveType::Kind::BoolTy);

	primitiveStringTy_	= std::make_unique<PrimitiveType>(PrimitiveType::Kind::StringTy);
	primitiveCharTy_	= std::make_unique<PrimitiveType>(PrimitiveType::Kind::CharTy);
}

ArrayType * ASTContext::getArrayTypeForType(Type * ty)
{
	// Effective STL, Item 24 by Scott Meyers : https://stackoverflow.com/a/101980
	auto lb = arrayTypes_.lower_bound(ty);
	if (lb != arrayTypes_.end() && !(arrayTypes_.key_comp()(ty, lb->first)))
	{
		// Key already exists, return lb->second.get()
		return (lb->second).get();
	}
	else
	{
		// Key does not exists, insert & return.
		auto insertionResult = arrayTypes_.insert(lb,{ ty, std::make_unique<ArrayType>(ty) });
		return (insertionResult->second).get();
	}
}