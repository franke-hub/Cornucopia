//----------------------------------------------------------------------------
//
//       Copyright (C) 2020 Frank Eskesen.
//
//       This file is free content, distributed under the GNU General
//       Public License, version 3.0.
//       (See accompanying file LICENSE.GPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/gpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       EdMark.h
//
// Purpose-
//       Editor: Line/block mark descriptor
//
// Last change date-
//       2021/01/04
//
//----------------------------------------------------------------------------
#ifndef EDMARK_H_INCLUDED
#define EDMARK_H_INCLUDED

#include <pub/List.h>               // For List

//----------------------------------------------------------------------------
// Forward references
//----------------------------------------------------------------------------
class EdFile;
class EdLine;
class EdRedo;

//----------------------------------------------------------------------------
//
// Class-
//       EdMark
//
// Purpose-
//       Line/block marker
//
//----------------------------------------------------------------------------
class EdMark {                      // Editor mark descriptor
//----------------------------------------------------------------------------
// EdMark::Attributes
//----------------------------------------------------------------------------
public:
// Current mark information
EdFile*                mark_file= nullptr; // The marked file
EdLine*                mark_head= nullptr; // The first marked file line
EdLine*                mark_tail= nullptr; // The last  marked file line

EdLine*                mark_line= nullptr; // The last  marked line
ssize_t                mark_col= -1; // The last column marked (-1 for line)
size_t                 mark_lh= -1; // Mark left-hand column
size_t                 mark_rh= -1; // Mark right-hand column (+1)

// Current copy information (Last copy/cut) Note: move= cut + paste
EdFile*                copy_file= nullptr; // The copied file
pub::List<EdLine>      copy_list;   // The current copy/cut list
size_t                 copy_rows= 0; // The number of copy/cut rows
size_t                 copy_lh= -1; // Copy left-hand column
size_t                 copy_rh= -1; // Copy right-hand column (+1)

//----------------------------------------------------------------------------
// EdMark::Constructor/Destructor
//----------------------------------------------------------------------------
public:
   EdMark( void );                  // Constructor
   ~EdMark( void );                 // Destructor

//----------------------------------------------------------------------------
// EdMark::Methods
//----------------------------------------------------------------------------
public:
void
   debug(                           // Debugging display
     const char*       info= nullptr) const; // Associated info

const char*                         // Error message, nullptr expected
   copy( void );                    // Copy the marked area

const char*                         // Error message, nullptr expected
   cut( void );                     // Remove the marked area

const char*                         // Error message, nullptr expected
   format( void );                  // Format the mark

void
   handle_redo(                     // Handle completed redo operation
     EdFile*           file,        // For this file
     EdRedo*           redo);       // And this REDO

void
   handle_undo(                     // Handle completed undo operation
     EdFile*           file,        // For this file
     EdRedo*           undo);       // And this UNDO

const char*                         // Error message, nullptr expected
   mark(                            // Create/expand/contract the mark
     EdFile*           file,        // For this EdFile
     EdLine*           line,        // And this EdLine
     ssize_t           column= -1); // Start at this column (block copy)

const char*                         // Error message, nullptr expected
   paste(                           // Paste the marked area
     EdFile*           file,        // Into this EdFile
     EdLine*           line,        // After this line
     ssize_t           column= -1); // Start at this column (block copy)

void
   redo(                            // Redo the mark
     EdFile*           file,        // For this EdFile
     EdLine*           head,        // From this EdLine
     EdLine*           tail);       // *To* this EdLine

void
   undo( void );                    // Undo the mark
}; // class EdMark
#endif // EDMARK_H_INCLUDED
