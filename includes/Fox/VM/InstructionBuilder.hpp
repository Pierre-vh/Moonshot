//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : InstructionBuilder.hpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  This file contains the InstructionBuilder, which is a class that helps
//  build instructions buffers readable by the VM.
//----------------------------------------------------------------------------//

#pragma once

#include <cstdint>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "Fox/Common/LLVM.hpp"

namespace fox {
  enum class Opcode : std::uint8_t;

  class InstructionBuilder {
    public:
      #define SIMPLE_INSTR(ID) InstructionBuilder& create##ID##Instr();
      #define TERNARY_INSTR(ID) InstructionBuilder&\
        create##ID##Instr(std::uint8_t a, std::uint8_t b, std::uint8_t c);
      #define SMALL_BINARY_INSTR(ID) InstructionBuilder&\
        create##ID##Instr(std::uint8_t a, std::uint8_t b);
      #define BINARY_INSTR(ID) InstructionBuilder&\
        create##ID##Instr(std::uint8_t a, std::uint16_t d);
      #define UNARY_INSTR(ID) InstructionBuilder&\
        create##ID##Instr(std::uint32_t val);
      #define SIGNED_UNARY_INSTR(ID) InstructionBuilder&\
        create##ID##Instr(std::int32_t val);
      #include "Instructions.def"

      void reset();
      std::uint32_t getLastInstr() const;
      ArrayRef<std::uint32_t> getInstrs() const;

    private:
      InstructionBuilder& 
      createSimpleInstr(Opcode op);

      InstructionBuilder& 
      createABCInstr(Opcode op, std::uint8_t a, std::uint8_t b, std::uint8_t c);

      InstructionBuilder& 
      createADInstr(Opcode op, std::uint8_t a, std::uint16_t d);

      InstructionBuilder& 
      createSignedUnaryInstr(Opcode op, std::int32_t val);

      InstructionBuilder& 
      createUnaryInstr(Opcode op, std::uint32_t val);

      void pushInstr(std::uint32_t instr);

      SmallVector<std::uint32_t, 4> instrsBuff_;
  };
}