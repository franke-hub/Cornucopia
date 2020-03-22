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
//       StoreInterface.java
//
// Purpose-
//       Define the StoreInterface.
//
// Last change date-
//       2010/01/01
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Interface-
//       StoreInterface
//
// Purpose-
//       Define the StoreInterface.
//
//----------------------------------------------------------------------------
interface StoreInterface
{
//----------------------------------------------------------------------------
//
// Method-
//       StoreInterface.store
//
// Purpose-
//       Store word.
//
//----------------------------------------------------------------------------
public abstract void
   store(                           // Store a Word
     int               address,     // Address
     Object            word);       // Word to store
} // Class StoreInterface

