//----------------------------------------------------------------------------
//
//       Copyright (C) 2010 Frank Eskesen.
//
//       This file is free content, distributed under the GNU General
//       Public License, version 3.0.
//       (See accompanying file LICENSE.GPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/gpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       IncrInstruction.java
//
// Purpose-
//       Increment instruction.
//
// Last change date-
//       2010/01/01
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Class-
//       IncrInstruction
//
// Purpose-
//       Increment value.
//
//----------------------------------------------------------------------------
public class IncrInstruction extends Instruction
{
//----------------------------------------------------------------------------
//
// Method-
//       IncrInstruction.IncrInstruction
//
// Purpose-
//       Constructor.
//
//----------------------------------------------------------------------------
public
   IncrInstruction( )               // Constructor
{
   super();
}

public
   IncrInstruction(                 // Constructor
     int               amod)        // Modifier
{
   super(amod);
}

public
   IncrInstruction(                 // Constructor
     int               amod,        // Modifier
     int               addr)        // Address
{
   super(amod, addr);
}

//----------------------------------------------------------------------------
//
// Method-
//       IncrInstruction.execute
//
// Purpose-
//       Execute this Instruction.
//
//----------------------------------------------------------------------------
public void                         // Execute this Instruction
   execute(                         // Execute this Instruction
     Cpu               cpu)         // Associated Cpu
   throws Exception
{
   Addr                addr;

   addr= (Addr)cpu.dFetch(getAddr(cpu));
   addr.setAddr(addr.getAddr() + 1);
}
} // Class IncrInstruction

