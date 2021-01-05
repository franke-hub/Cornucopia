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
//       Editor.h
//
// Purpose-
//       Editor: Global data areas
//
// Last change date-
//       2020/12/25
//
//----------------------------------------------------------------------------
#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <xcb/xproto.h>             // For xcb_keysym_t
#include <pub/List.h>               // For pub::List

#include "Xcb/Active.h"             // For xcb::Active
#include "Xcb/Device.h"             // For xcb::Device
#include "Xcb/Widget.h"             // For xcb::Widget, our base class
#include "Xcb/Window.h"             // For xcb::Window

//----------------------------------------------------------------------------
// Forward references
//----------------------------------------------------------------------------
class EdFile;                       // Editor file descriptor
class EdFind;                       // Editor find Popup
class EdFull;                       // Editor full Window (experimental)
class EdHist;                       // Editor history view
class EdMark;                       // Editor mark controller
class EdMenu;                       // Editor menu Layout
class EdPool;                       // Editor pool allocators
class EdTabs;                       // Editor tabs Layout
class EdText;                       // Editor text Window
class EdView;                       // Editor view

//----------------------------------------------------------------------------
//
// Class-
//       Editor
//
// Purpose-
//       Constructor/destructor (for namespace editor)
//
//----------------------------------------------------------------------------
class Editor {                      // Editor constuctor/destructor
//----------------------------------------------------------------------------
// Editor::Constructor/Destructor
//----------------------------------------------------------------------------
public:
   Editor(                          // Constructor
     int               argi,        // Argument index
     int               argc,        // Argument count
     char*             argv[]);     // Argument array

   ~Editor( void );                 // Destructor

//----------------------------------------------------------------------------
// Editor::debug (Debugging display)
//----------------------------------------------------------------------------
static void
   debug(                           // Debugging display
     const char*       info= nullptr); // Associated info
}; // class Editor

//----------------------------------------------------------------------------
//
// Namespace-
//       editor
//
// Purpose-
//       Editor: Global data areas
//
//----------------------------------------------------------------------------
namespace editor {                  // The Editor namespace
//----------------------------------------------------------------------------
// editor::Global attributes
//----------------------------------------------------------------------------
extern xcb::Device*    device;      // The root Device
extern xcb::Window*    window;      // The test Window
extern EdText*         text;        // The Text Window

extern pub::List<EdFile> file_list; // The list of EdFiles
extern EdFile*         file;        // The current File object

extern EdMark*         mark;        // The Mark Handler
extern EdView*         data;        // The data view
extern EdHist*         hist;        // The history view
extern EdView*         view;        // The active view

extern std::string     locate_string; // The locate string
extern std::string     change_string; // The change string

extern pub::List<EdPool> filePool;  // File allocation EdPool
extern pub::List<EdPool> textPool;  // Text allocation EdPool

//----------------------------------------------------------------------------
//
// Method-
//       editor::command
//
// Purpose-
//       Process a command.
//
// Implementation notes-
//       Implemented in EdBifs.cpp
//
//----------------------------------------------------------------------------
const char*                         // Error message, nullptr if none
   command(                         // Process a command
     char*             buffer);     // (MODIFIABLE) command buffer

//----------------------------------------------------------------------------
//
// Method-
//       editor::allocate
//
// Purpose-
//       Allocate file/line text
//
//----------------------------------------------------------------------------
char*                               // The (immutable) text
   allocate(                        // Get (immutable) text
     size_t            length);     // Of this length (includes '\0' delimit)

//----------------------------------------------------------------------------
//
// Method-
//       editor::do_change
//
// Purpose-
//       Change next occurance of string.
//
//----------------------------------------------------------------------------
const char*                         // Return message, nullptr if OK
   do_change( void );               // Change next occurance of string

//----------------------------------------------------------------------------
//
// Method-
//       editor::do_exit
//
// Purpose-
//       (Safely) remove a file from the ring.
//
//----------------------------------------------------------------------------
void
   do_exit( void );                 // Safely remove a file from the ring

//----------------------------------------------------------------------------
//
// Method-
//       editor::do_history
//
// Purpose-
//       Invert history view.
//
//----------------------------------------------------------------------------
void
   do_history( void );              // Invert history view

//----------------------------------------------------------------------------
//
// Method-
//       editor::do_locate
//
// Purpose-
//       Locate next occurance of string.
//
//----------------------------------------------------------------------------
const char*                         // Return message, nullptr if OK
   do_locate(                       // Locate next
     int               offset= 1);  // Use offset 0 for locate_change

//----------------------------------------------------------------------------
//
// Method-
//       editor::do_test
//
// Purpose-
//       Bringup test.
//
//----------------------------------------------------------------------------
void
   do_test( void );                 // Bringup test

//----------------------------------------------------------------------------
//
// Method-
//       editor::exit
//
// Purpose-
//       Unconditional editor (normal) exit.
//
//----------------------------------------------------------------------------
void
   exit( void );                    // Unconditional editor (normal) exit

//----------------------------------------------------------------------------
//
// Method-
//       editor::insert_file
//
// Purpose-
//       Insert file onto the file list
//
//----------------------------------------------------------------------------
EdFile*                             // The last file inserted
   insert_file(                     // Insert file(s) onto the file list
     const char*       name= nullptr); // The file name (wildards allowed)

//----------------------------------------------------------------------------
//
// Method-
//       editor::key_to_name
//
// Purpose-
//       BRINGUP: Convert xcb_keysym_t to its name. (TODO: REMOVE)
//
//----------------------------------------------------------------------------
const char*                         // The symbol name, "???" if unknown
   key_to_name(xcb_keysym_t key);   // Convert xcb_keysym_t to name

//----------------------------------------------------------------------------
//
// Method-
//       editor::put_message
//
// Purpose-
//       Add message to file's message list
//
//----------------------------------------------------------------------------
void
   put_message(                     // Write message
     const char*       _mess,       // Message text
     int               _type= 0);   // Message mode (default EdMess::T_INFO)

//----------------------------------------------------------------------------
//
// Method-
//       editor::remove_file
//
// Purpose-
//       Remove active file from the file list
//
//----------------------------------------------------------------------------
void
   remove_file( void );             // Remove active file from the file list

//----------------------------------------------------------------------------
//
// Method-
//       editor::set_font
//
// Purpose-
//       Set the font
//
//----------------------------------------------------------------------------
int                                 // Return code, 0 OK
   set_font(                        // Set the font
     const char*       font= nullptr); // To this font name

//----------------------------------------------------------------------------
//
// Method-
//       editor::un_changed
//
// Purpose-
//       If any file has changed, activate it.
//
//----------------------------------------------------------------------------
bool                                // TRUE iff editor is in unchanged state
   un_changed( void );              // Activate a changed file

//----------------------------------------------------------------------------
//
// Method-
//       editor::join
//       editor::start
//
// Purpose-
//       Virtual thread implementation
//
//----------------------------------------------------------------------------
void
   join( void );                    // Wait for "Thread"

void
   start( void );                   // Start "Thread"
}  // namespace editor
#endif // EDITOR_H_INCLUDED