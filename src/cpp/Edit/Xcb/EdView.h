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
//       EdView.h
//
// Purpose-
//       Editor: TextWindow view
//
// Last change date-
//       2020/12/19
//
//----------------------------------------------------------------------------
#ifndef EDVIEW_H_INCLUDED
#define EDVIEW_H_INCLUDED

#include <string>                   // For std::string
#include <sys/types.h>              // For system types
#include <xcb/xproto.h>             // For xcb_gcontext_t, ...

#include <Xcb/Active.h>             // For xcb::Active

#include "Editor.h"                 // For Editor
#include "EdFile.h"                 // For EdFile, EdLine
#include "EdText.h"                 // For EdText

//----------------------------------------------------------------------------
//
// Class-
//       EdView
//
// Purpose-
//       TextWindow view.
//
//----------------------------------------------------------------------------
class EdView {                      // Editor TextWindow view
//----------------------------------------------------------------------------
// EdView::Attributes
//----------------------------------------------------------------------------
public:
xcb::Active            active;      // The Active text buffer
EdLine*                cursor= nullptr; // The Active cursor line

size_t                 col_zero= 0; // Current column[0]
size_t                 row_zero= 0; // Current row[0]
unsigned               col= 0;      // Current screen column
unsigned               row= 0;      // Current screen row

// Graphic context object identifiers are copies, neither allocated nor deleted
xcb_gcontext_t         gc_flip= 0;  // Graphic context: cursor character
xcb_gcontext_t         gc_font= 0;  // Graphic context: normal line
xcb_gcontext_t         gc_mark= 0;  // Graphic context: marked character

//----------------------------------------------------------------------------
// EdView::Constructor/Destructor
//----------------------------------------------------------------------------
public:
   EdView( void );                  // Constructor

virtual
   ~EdView( void );                 // Destructor

//----------------------------------------------------------------------------
//
// Method-
//       EdView::debug
//
// Purpose-
//       Debugging display
//
//----------------------------------------------------------------------------
virtual void
   debug(                           // Debugging display
     const char*       info= nullptr) const; // Associated info

//----------------------------------------------------------------------------
//
// Method-
//       EdView::get_gc
//
// Purpose-
//       Get the current graphic context
//
//----------------------------------------------------------------------------
virtual xcb_gcontext_t               // The current graphic context
   get_gc( void );                   // Get current graphic context

//----------------------------------------------------------------------------
//
// Method-
//       EdView::activate
//
// Purpose-
//       Activate the history line
//
//----------------------------------------------------------------------------
virtual void
   activate( void );                // Activate the history line

//----------------------------------------------------------------------------
//
// Method-
//       EdView::commit
//
// Purpose-
//       Commit the Active data line
//
// Implementation notes-
//       This is the EdText commit, overridden in EdHist
//
//----------------------------------------------------------------------------
virtual void
   commit( void );                   // Commit the Active line

//----------------------------------------------------------------------------
//
// Method-
//       EdView::move_cursor_V
//
// Purpose-
//       Move cursor vertically
//
//----------------------------------------------------------------------------
virtual void
   move_cursor_V(                   // Move cursor vertically
     int             n= 1);         // The relative row (Down positive)
}; // class EdView
#endif // EDVIEW_H_INCLUDED
