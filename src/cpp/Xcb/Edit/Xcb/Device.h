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
//       Xcb/Device.h
//
// Purpose-
//       XCB device driver
//
// Last change date-
//       2020/12/02
//
//----------------------------------------------------------------------------
#ifndef XCB_DEVICE_H_INCLUDED
#define XCB_DEVICE_H_INCLUDED

#include <xcb/xcb.h>                // For XCB interfaces
#include <xcb/xproto.h>             // For XCB types
#include <pub/Signals.h>            // For pub::signals

#include "Xcb/Widget.h"             // For Widget
#include "Xcb/Window.h"             // Our base class

namespace xcb {
//----------------------------------------------------------------------------
//
// Struct-
//       xcb::DeviceEvent
//
// Purpose-
//       Generic Device Event
//
// Implementation note-
//       For events not associated with a Pixmap or Window:
//         xcb_ge_generic_event_t      // GE generic event
//         xcb_keymap_notify_event_t   // Keymap notify event
//         xcb_mapping_notify_event_t  // Mapping notify event
//
//----------------------------------------------------------------------------
struct DeviceEvent {                // Device Event descriptor
xcb_generic_event_t*   event;       // (Generic) event

// Constructors ==============================================================
   DeviceEvent(                     // Default/event constructor
     xcb_generic_event_t* _event= nullptr)
:  event(_event) {}

// Destructor ================================================================
   ~DeviceEvent( void ) = default;
}; // struct DeviceEvent

//----------------------------------------------------------------------------
//
// Class-
//       xcb::Device
//
// Purpose-
//       xcb::Device object, the root xcb::Window.
//
//----------------------------------------------------------------------------
class Device : public Window {      // XCB device driver
//----------------------------------------------------------------------------
// xcb::Device::Attributes
//----------------------------------------------------------------------------
public:
pub::signals::Signal<DeviceEvent>
                       signal;      // The DeviceEvent signal
Display*               display= nullptr; // X11 Display
bool                   operational= true; // TRUE while operational

// Defaults
xcb_rectangle_t        geom= {0,0,0,0}; // Device geometry

//----------------------------------------------------------------------------
// xcb::Device::Constructors/Destructors/Operators
//----------------------------------------------------------------------------
public:
   Device( void );                  // Constructor

virtual
   ~Device( void );                 // Destructor

//----------------------------------------------------------------------------
// xcb::Device::Methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Implementation note-
//   For all other Widgets, configure() is the *last* configure method.
//   Device::configure() *begins* the configuation process
virtual void
   configure( void );               // Configure everything

virtual void
   debug_tree(                      // Debug the Widget tree
     const char*       info= nullptr); // Caller's information

virtual void
   draw( void );                    // Draw everything

virtual Pixmap*                     // The Pixmap, if located
   locate(                          // Recursively locate
     xcb_drawable_t    target);     // This XCB Window/Pixmap

xcb_keysym_t                        // The associated symbol
   to_keysym(                       // Convert to Keysym
     const xcb_key_press_event_t*
                       event) const; // This key press (or release) event

//----------------------------------------------------------------------------
// xcb::Device::Thread simulation
//----------------------------------------------------------------------------
public:
void
   join( void );                    // Wait for Thread completion

virtual void
   run( void );                     // Event processor

void
   start( void );                   // Start the Thread
}; // class xcb::Device
}  // namespace xcb
#endif // XCB_DEVICE_H_INCLUDED
