//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : Identifier.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/AST/Identifier.hpp"
#include "Fox/Common/Errors.hpp"
#include <ostream>

using namespace fox;

Identifier::Identifier(const char* ptr) : ptr_(ptr) {
	assert(ptr && "Cannot create an identifier with a null string!");
}

Identifier::Identifier() : ptr_(nullptr) {}

string_view Identifier::getStr() const {
  return string_view(ptr_);
}

bool Identifier::operator==(const Identifier& other) const {
	return ptr_ == other.ptr_;
}

bool Identifier::operator!=(const Identifier& other) const {
	return ptr_ != other.ptr_;
}

bool Identifier::operator<(const Identifier& other) const {
	return ptr_ < other.ptr_;
}

bool Identifier::operator==(const string_view other) const {
	return getStr() == other;
}

bool Identifier::operator!=(const string_view other) const {
	return getStr() != other;
}

bool Identifier::operator<(const string_view other) const {
	return getStr() < other;
}

const char* Identifier::c_str() const {
	return ptr_;
}

bool Identifier::isNull() const {
	return (ptr_ == nullptr);
}

Identifier::operator bool() const {
	return !isNull();
}

std::ostream& fox::operator<<(std::ostream& os, Identifier id) {
  os << (id.isNull() ? "<null id>" : id.getStr());
  return os;
}
