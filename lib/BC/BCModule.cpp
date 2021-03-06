//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : BCModule.cpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/BC/BCModule.hpp"
#include "Fox/Common/QuotedString.hpp"
#include "llvm/ADT/ArrayRef.h"

using namespace fox;

static func_id_t to_func_id_t(std::size_t value) {
  assert((value <= bc_limits::max_func_id) && "func_id_t's limit reached.");
  return func_id_t(value);
}

BCFunction& BCModule::createFunction() {
  functions_.push_back(
    std::make_unique<BCFunction>(to_func_id_t(numFunctions()))
  );
  return *functions_.back();
}

BCFunction& BCModule::createGlobalVariable() {
  globalVarInitializers_.push_back(
    std::make_unique<BCFunction>(to_func_id_t(numGlobals()))
  );
  return *globalVarInitializers_.back();
}

std::size_t BCModule::addStringConstant(string_view str) {
  std::size_t idx = strConstants_.size();
  strConstants_.push_back(str.to_string());
  return idx;
}

const std::string& BCModule::getStringConstant(std::size_t idx) const {
  assert((idx < strConstants_.size()) && "out-of-range");
  return strConstants_[idx];
}

ArrayRef<std::string> BCModule::getStringConstants() const {
  return strConstants_;
}

std::size_t BCModule::addIntConstant(FoxInt value) {
  std::size_t idx = intConstants_.size();
  intConstants_.push_back(value);
  return idx;
}

FoxInt BCModule::getIntConstant(std::size_t idx) const {
  assert((idx < intConstants_.size()) && "out-of-range");
  return intConstants_[idx];
}

ArrayRef<FoxInt> BCModule::getIntConstants() const {
  return intConstants_;
}

std::size_t BCModule::addDoubleConstant(FoxDouble value) {
  std::size_t idx = doubleConstants_.size();
  doubleConstants_.push_back(value);
  return idx;
}

FoxDouble BCModule::getDoubleConstant(std::size_t idx) const {
  assert((idx < doubleConstants_.size()) && "out-of-range");
  return doubleConstants_[idx];
}

ArrayRef<FoxDouble> BCModule::getDoubleConstants() const {
  return doubleConstants_;
}

bool BCModule::empty() const {
  return functions_.empty()
      && empty_constants();
}

bool BCModule::empty_constants() const {
  return doubleConstants_.empty()
      && intConstants_.empty() 
      && strConstants_.empty();
}

void BCModule::dump(std::ostream& out) const {
  if (empty()) {
    out << "[Empty BCModule]\n";
    return;
  }
  dumpConstants(out);
  dumpGlobVarInitializers(out);
  dumpFunctions(out);
}

void BCModule::dumpConstants(std::ostream& out) const {
  if (empty_constants()) {
    out << "[No Constants]\n";
    return;
  }

  out << "[Constants]\n";

  if(std::size_t size = intConstants_.size()) {
    out << "  [Integers: " << size << " constants]\n";
    for (std::size_t idx = 0; idx < size; ++idx) 
      out << "    " << idx << "\t| " << intConstants_[idx] << '\n';
  }
  else 
    out << "  [No Integer Constants]\n";

  if(std::size_t size = doubleConstants_.size()) {
    out << "  [Floating-Point: " << size << " constants]\n";
    for (std::size_t idx = 0; idx < size; ++idx) 
      out << "    " << idx << "\t| " << doubleConstants_[idx] << '\n';
  }
  else 
    out << "  [No Floating-Point Constants]\n";

  if(std::size_t size = strConstants_.size()) {
    out << "  [Strings: " << size << " constants]\n";
    for (std::size_t idx = 0; idx < size; ++idx) {
      out << "    " << idx << "\t| ";
      printQuotedString(strConstants_[idx], out, '"');
      out << '\n';
    }
  }
  else 
    out << "  [No String Constants]\n";
}

void BCModule::dumpGlobVarInitializers(std::ostream& out) const {
  std::size_t size = globalVarInitializers_.size();
  if(size == 0) {
    out << "[No Globals]\n";
    return;
  }
  out << "[Globals: " << size << "]\n";
  for (auto& gvi : globalVarInitializers_) {
    gvi->dump(out, "Initializer of Global");
  }
}

void BCModule::dumpFunctions(std::ostream& out) const {
  std::size_t size = functions_.size();
  if (size == 0) {
    out << "[No Functions]\n";
    return;
  }
  const BCFunction* entry = getEntryPoint();
  out << "[Functions: " << size << ']';
  out << "[Entry Point:";
  if(entry)
    out << " Function #" << entry->getID();
  else 
    out << " None";
  out << "]\n";
  for (auto& fn : functions_) {
    // Print a newline before each function dump so it's more readable.
    out << '\n';
    fn->dump(out);
  }
}
