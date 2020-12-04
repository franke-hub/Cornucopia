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
//       Editor.cpp
//
// Purpose-
//       Editor: Implement Editor.h
//
// Last change date-
//       2020/12/04
//
//----------------------------------------------------------------------------
#include <mutex>                    // For std::mutex, std::lock_guard

#include <assert.h>                 // For assert
#include <stdio.h>                  // For printf
#include <stdlib.h>                 // For various
#include <string.h>                 // For strcmp
#include <unistd.h>                 // For close, ftruncate
#include <sys/stat.h>               // For stat
#include <xcb/xcb.h>                // For XCB interfaces
#include <xcb/xproto.h>             // For XCB types

#include <pub/Signals.h>            // For pub::signals
#include <pub/Thread.h>             // For pub::Thread::sleep
#include <pub/Trace.h>              // For pub::Trace

#include "Xcb/Global.h"             // For xcb::opt_hcdm,opt_verbose; debugging
#include "Xcb/Device.h"             // For xcb::Device
#include <Xcb/Keysym.h>             // For xcb_keycode_t symbols
#include "Xcb/Layout.h"             // For xcb::Layout
#include "Xcb/TestWindow.h"         // For xcb::TestWindow
#include "Xcb/Widget.h"             // For xcb::Widget, our base class
#include "Xcb/Window.h"             // For xcb::Window

#include "Editor.h"                 // Editor Globals
#include "EdFile.h"                 // For EdFile, EdLine, EdPool
#include "EdFind.h"                 // For EdFind
#include "EdFull.h"                 // For EdFull
#include "EdMenu.h"                 // For EdMenu
#include "EdMisc.h"                 // For EdMisc TODO: REMOVE
#include "EdPool.h"                 // For EdPool
#include "EdTabs.h"                 // For EdTabs
#include "EdText.h"                 // For EdText

//----------------------------------------------------------------------------
// Constants for parameterization
//----------------------------------------------------------------------------
enum // Compilation controls
{  HCDM= false                      // Hard Core Debug Mode?
,  USE_BRINGUP= false               // Extra brinbup diagnostics?
}; // Compilation controls

//----------------------------------------------------------------------------
// External data areas
//----------------------------------------------------------------------------
Editor*                Editor::editor= nullptr; // The Editor singleton

//----------------------------------------------------------------------------
// Internal data areas
//----------------------------------------------------------------------------
static std::mutex      mutex;       // Singleton mutex

//----------------------------------------------------------------------------
//
// Subroutine-
//       use_debug
//
// Purpose-
//       Test whether or not debugging is active
//
//----------------------------------------------------------------------------
static inline bool use_debug( void ) // Is debugging active?
{ return HCDM || USE_BRINGUP || xcb::opt_hcdm; }

//----------------------------------------------------------------------------
//
// Method-
//       Editor::Editor
//
// Purpose-
//       Constructor
//
//----------------------------------------------------------------------------
   Editor::Editor(                  // Constructor
     int               argi,        // Argument index
     int               argc,        // Argument count
     char*             argv[])      // Argument array
:  ring(), filePool(), textPool(), locate_string(), change_string()
{
   if( xcb::opt_hcdm )
     xcb::debugh("Editor(%p)::Editor\n", this);

   // Initialize singleton
   {{ std::lock_guard<decltype(mutex)> lock(mutex);
      if( editor ) throw "Multiple Editors"; // Enforce singleton
      editor= this;
   }}

   // Allocate initial textPool
   textPool.fifo(new EdPool(EdPool::MIN_SIZE));

   // Allocate Device
   device= new xcb::Device();

   // Allocate sub-objects
   find= new EdFind();              // In progress
   full= new EdFull();              // In progress
   menu= new EdMenu();              // In progress
   tabs= new EdTabs();              // In progress
   text= new EdText();              // Operational

   //-------------------------------------------------------------------------
   // Load the text files
   for(int i= argi; i<argc; i++) {
     ring.fifo(new EdFile(argv[i]));
   }
   if( argi >= argc ) {             // Always have something
     ring.fifo(new EdFile(nullptr)); // Even if it's an empty file
   }

   // Select-a-Config ========================================================
   if( xcb::opt_test ) {            // If optional test selected
     std::string test= xcb::opt_test; // The test name (For string == function)
     if( test == "fullwindow" ) {   // Only EdText visible
       // Result: EdText takes entire screen. EdFull not visibie.
       //   Device->EdFull->EdText
       full->insert(text);
       device->insert(full);

     } else if( test == "windowfull" ) { // Large blank screen
       // Result: EdFull takes entire screen. EdText not visibie.
       //   Device->EdText->EdFull
       text->insert(full);
       device->insert(text);

     } else if( test == "findwindow" ) { // EdFind overlayed with EdText
       // Result: EdFind over EdText window, as expected (F12 controlled)
       //   Device->EdText->EdFind
       window= new EdFind();        // (Cannot use find: duplicate delete)
       device->insert(text);
       device->insert(window);

     } else if( test == "miscwindow" ) { // EdText overlayed with EdMisc
       // Result: EdMisc over EdText window, as expected
       //   Device->EdText->EdMisc
       window= new EdMisc(text, "Misc00", 64, 64);
       device->insert(text);

     } else if( test == "testwindow" ) { // EdText overlayed with TestWindow
       // Result: TextWindow over EdText window, as expected
       //   Device->EdText->TestWindow
       window= new xcb::TestWindow();
       text->insert(window);
       device->insert(text);

     } else if( test == "textwindow" ) { // EdText (only)
       device->insert(text);

     } else if( test == "bot-only" ) { // EdText visible
       // Result: EdMisc only appears after screen enlarged
       //   Device->Row->(EdText,EdMisc)
       // PROBLEM: CLOSE Window gets SIGABRT, unable to diagnose
       //          (only occurs after screen enlarged)
       xcb::RowLayout* row= new xcb::RowLayout(device, "Row");
       window= new EdMisc(nullptr, "Bottom", 64, 64);
       row->insert( text );
       row->insert( window );

     } else if( test == "top-only" ) { // (small horizontal window)
       // Result: EdText only appears after screen enlarged
       //   Device->Row->(EdTabs,EdText)
       //   (No problem with CLOSE Window)
       xcb::RowLayout* row= new xcb::RowLayout(device, "Row");
       row->insert( tabs );         // Apparently visible
       row->insert( text );         // Not visible

     } else if( test == "left-only" ) { // (small vertial window)
       // Result: EdText only appears after screen enlarged
       //   Device->Col->(EdMisc,EdText)
       //   (CLOSE Window checkstop: Bad Window when closing window)
       xcb::ColLayout* col= new xcb::ColLayout(device, "Col");
       window= new EdMisc(nullptr, "Left", 14, 64);
       col->insert(window);         // Apparently visible
       col->insert(text);           // Not visible

     } else if( test == "layout" ) { // (small horizontal window)
       // Result: EdText does not appear even after screen enlarged
       //   (No expose events. EdText parent "Left")
       //   Device->Row->(EdMenu,EdTabs,Col->(Left,EdText),Bottom)
       //   (CLOSE Window OK, No EdText so no Ctrl-Q)
       xcb::RowLayout* row= new xcb::RowLayout(device, "Row");
       row->insert( menu );
       row->insert( tabs );

       xcb::ColLayout* col= new xcb::ColLayout(row, "Col"); // (Row->insert(col))
       if( false ) row->insert( col ); // (Tests duplicate insert)
       col->insert( new EdMisc(nullptr, "Left", 14, 64) );
       col->insert( text );
       row->insert( new EdMisc(nullptr, "Bottom", 64, 14));

     } else {
       xcb::user_debug("Test(%s) not available\n", xcb::opt_test);
       exit(EXIT_FAILURE);
     }

     xcb::user_debug("Test(%s) selected\n", xcb::opt_test);
   } else {                         // Default: textwindow ==================
     device->insert(text);
   }
   // Select-a-Config ========================================================
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::~Editor
//
// Purpose-
//       Destructor
//
//----------------------------------------------------------------------------
   Editor::~Editor( void )          // Destructor
{
   // Remove and delete Files
   for(;;) {
     EdFile* file= ring.remq();
     if( file == nullptr )
       break;

     delete file;
   }

   // Remove and delete storage pools
   for(;;) {
     EdPool* pool= textPool.remq();
     if( pool == nullptr )
       break;

     delete pool;
   }

   for(;;) {
     EdPool* pool= filePool.remq();
     if( pool == nullptr )
       break;

     delete pool;
   }

   // Delete allocated objects
   delete window;
   delete text;
   delete tabs;
   delete menu;
   delete full;
   delete find;

   // Remove the Editor singleton
   std::lock_guard<decltype(mutex)> lock(mutex);
   if( editor == this )
     editor= nullptr;
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::do_done
//
// Purpose-
//       Handle DONE function: Safely exit all EdFiles
//
// Implementation note-
//       TODO: NOT (PROPERLY) CODED YET
//
//----------------------------------------------------------------------------
int                                 // Return code, 0OK
   Editor::do_done( void )          // Handle DONE
{  device->operational= false; return 0; } // TODO: Safely exit

//----------------------------------------------------------------------------
//
// Method-
//       Editor::do_quit
//
// Purpose-
//       Handle QUIT function: Unconditionally remove EdFile from ring
//
//----------------------------------------------------------------------------
void
   Editor::do_quit(                 // Handle QUIT (Unconditional)
     EdFile*           file)        // For this EdFile
{
   EdFile* next= file->get_prev();
   if( next == nullptr ) {
     next= file->get_next();
     if( next == nullptr )          // If no more files
       device->operational= false;  // No need to stay around
   }

   if( next ) {
     ring.remove(file, file);       // Remove the File from the Ring
     delete file;                   // And delete it

     text->activate(next);
       text->draw();
   }
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::do_test
//
// Purpose-
//       Bringup test. Currently flips window visibility.
//
//----------------------------------------------------------------------------
void
   Editor::do_test( void )          // Bringup test
{
   if( window && window->get_parent() ) { // If TestWindow active
     fprintf(stderr, "Editor(%p)::do_test\n", this);
     if( window->state & xcb::Window::WS_VISIBLE )
       window->hide();
     else
       window->show();
     device->draw();
   } else
     fprintf(stderr, "Editor(%p)::do_test NOT CONFIGURED\n", this);
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::get_text
//
// Purpose-
//       Allocate file/line text
//
//----------------------------------------------------------------------------
char*                               // The (immutable) text
   Editor::get_text(                // Get (immutable) text
     size_t            length)      // Of this length (includes '\0' delimit)
{
   char* text= nullptr;             // Not allocated (yet)
   EdPool* pool= textPool.get_head(); // Get text pool
   if( pool == nullptr ) {          // If no pool yet
     pool= new EdPool(EdPool::MIN_SIZE); // Create one now
     textPool.lifo(pool);           // And use it
   }

   text= pool->malloc(length);
   if( text == nullptr ) {
     if( length > (EdPool::MIN_SIZE / 8) ) { // If large allocation
       pool= new EdPool(length);    // All filePools are 100% allocated
       text= pool->malloc(length);
       filePool.lifo(pool);
     } else {                       // If small allocation
       for(pool= pool->get_next(); pool; pool= pool->get_next() ) {
         text= pool->malloc(length); // Try all textPools
         if( text )
           break;
       }

       if( text == nullptr ) {      // If a new EdPool is required
         if( xcb::opt_hcdm )
           xcb::debugh("Editor.get_text(%zd) New pool\n", length);
         pool= new EdPool(EdPool::MIN_SIZE);
         text= pool->malloc(length);
         textPool.lifo(pool);
       }
     }
   }

   if( xcb::opt_hcdm && xcb::opt_verbose > 1 )
     xcb::debugf("%p= Editor::allocate(%zd)\n", text, length);
   return text;
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::key_to_name
//
// Purpose-
//       BRINGUP: Convert xcb_keysym_t to its name. (TODO: REMOVE)
//
//----------------------------------------------------------------------------
const char*
   Editor::key_to_name(xcb_keysym_t key) // Convert xcb_keysym_t to name
{
   if( key >= 0x0020 && key <= 0x007f ) { // If text key
     static char buffer[2];
     buffer[0]= char(key);
     buffer[1]= '\0';
     return buffer;
   }

   switch( key ) {                  // Handle control key
     case XK_BackSpace:
       return "BackSpace";

     case XK_Tab:
       return "Tab";

     case XK_ISO_Left_Tab:
       return "Left_Tab";

     case XK_Return:
       return "Return";

     case XK_Scroll_Lock:
       return "Scroll_Lock";

     case XK_Escape:
       return "Escape";

     case XK_Delete:
       return "Delete";

     case XK_Insert:
       return "Insert";

     case XK_Home:
       return "Home";

     case XK_End:
       return "End";

     case XK_Menu:
       return "Menu";

     case XK_Left:
       return "Left arrow";

     case XK_Up:
       return "Up arrow";

     case XK_Right:
       return "Right arrow";

     case XK_Down:
       return "Down arrow";

     default:
       break;
   }

   return "???";
}

//----------------------------------------------------------------------------
//
// Method-
//       Editor::set_font
//
// Purpose-
//       Set the font
//
//----------------------------------------------------------------------------
int                                 // Return code, 0 OK
   Editor::set_font(                // Set the font
     const char*       font)        // To this font name
{  return text->set_font(font); }   // (Defer to EdText)

//----------------------------------------------------------------------------
// Editor::Virtual Thread simulation methods
//----------------------------------------------------------------------------
void
   Editor::join( void )
{  }

void
   Editor::start( void )
{
   // Initialize the configuration
   device->configure();

   // Set initial file
   text->activate(ring.get_head());

   // Start the Device
   device->draw();
   device->run();
}
