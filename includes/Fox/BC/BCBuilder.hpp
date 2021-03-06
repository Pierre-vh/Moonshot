//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : BCBuilder.hpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  This file declares the BCBuilder interface. It is a helper class used to
//  build bytecode buffers.
//----------------------------------------------------------------------------//

#pragma once

#include "BCUtils.hpp"
#include "Fox/BC/BCModule.hpp"
#include "Fox/Common/LLVM.hpp"
#include "Fox/Common/StableVectorIterator.hpp"
#include "llvm/Support/Compiler.h"
#include <cstdint>

namespace fox {
  class BCBuilder {
    public:
      /// A 'stable' iterator for the instruction buffer
      using StableInstrIter = 
        StableVectorIterator<InstructionVector>;
      /// A 'stable' const iterator for the instruction buffer
      using StableInstrConstIter = 
        StableVectorConstIterator<InstructionVector>;

      BCBuilder(InstructionVector& vector, DebugInfo* debugInfo = nullptr);

      #define TERNARY_INSTR(ID, I1, T1, I2, T2, I3, T3)\
        StableInstrIter create##ID##Instr(T1 I1, T2 I2, T3 I3);
      #define BINARY_INSTR(ID, I1, T1, I2, T2)\
        StableInstrIter create##ID##Instr(T1 I1, T2 I2);
      #define UNARY_INSTR(ID, I1, T1)\
        StableInstrIter create##ID##Instr(T1 I1);
      #define SIMPLE_INSTR(ID)\
        StableInstrIter create##ID##Instr();
      #include "Instruction.def"

      /// erases all instructions in the range [beg, end)
      void truncate_instrs(StableInstrIter beg);

      /// \returns true if the builder is empty
      LLVM_NODISCARD bool empty() const;

      /// \returns true if 'it' == getLastInstrIter()
      bool isLastInstr(StableInstrConstIter it) const;

      /// \returns an iterator to the last instruction inserted
      /// in the buffer. The buffer must not be empty.
      StableInstrIter getLastInstrIter();

      /// \returns an iterator to the last instruction inserted
      /// in the buffer. The buffer must not be empty.
      StableInstrConstIter getLastInstrIter() const;

      /// Adds a debug range for an instruction.
      /// \p iter_ must not be the end iterator.
      /// The BCBuilder must have a non-null DebugInfo*
      void addDebugRange(StableInstrConstIter iter, SourceRange range);

      /// \returns true if we have a DebugInfo instance attached.
      bool hasDebugInfo() const;

      /// Removes the last instruction added to this module.
      void popInstr();

      /// The Instruction vector that we are inserting into.
      InstructionVector& vector;
      
      /// The DebugInfo, if present.
      DebugInfo * const debugInfo = nullptr;

    private:
      StableInstrIter insert(Instruction instr);
  };
}