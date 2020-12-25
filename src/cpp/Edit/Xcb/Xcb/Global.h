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
//       Xcb/Global.h
//
// Purpose-
//       Global data areas and utilities
//
// Last change date-
//       2020/12/19
//
//----------------------------------------------------------------------------
#ifndef XCB_GLOBAL_H_INCLUDED
#define XCB_GLOBAL_H_INCLUDED

#include <errno.h>                  // For errno
#include <string.h>                 // For strerrno
#include <xcb/xcb.h>                // For generic_error_t

#include <pub/config.h>             // For _ATTRIBUTE_* macros

#include "Xcb/Types.h"              // For namespace xcb types

namespace xcb {
//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#define CHECKSTOP(name) checkstop(__LINE__, name)
#define XCBCHECK(xc, name) xcbcheck(__LINE__, name, xc)
#define XCBDEBUG(xc, name) xcbdebug(__LINE__, name, xc)

// ENQUEUE/NOQUEUE may be used with Pixmaps or Windows
#define ENQUEUE(name, op) enqueue(__LINE__, name, op)
#define NOQUEUE(name, op) noqueue(__LINE__, name, op)

//----------------------------------------------------------------------------
// (Settable) options
//----------------------------------------------------------------------------
extern int             opt_hcdm;    // Hard Core Debug Mode?
extern const char*     opt_test;    // Bringup test?
extern int             opt_verbose; // Debugging verbosity

//----------------------------------------------------------------------------
//
// uint32_t-
//       xcb::keystate
//
// Purpose-
//       Maintain keyboard state not maintained by xcb
//
//----------------------------------------------------------------------------
enum
{  KS_RESERVED_XCB= 0x0000ffff      // XCB reserved, i.e. XCB_KEY_BUT_MASK_*
,  KS_INS=          0x00010000      // Insert state
};
extern uint32_t        keystate;    // THE Global keyboard state

//----------------------------------------------------------------------------
//
// Subroutine-
//       user_debug
//
// Purpose-
//       Write to stderr, write to trace if opt_hcdm
//
//----------------------------------------------------------------------------
void                                // (If opt_hcdm, also writes to trace)
   user_debug(                      // Write message to stderr
     const char*       fmt,         // The PRINTF format string
                       ...)         // PRINTF argruments
   _ATTRIBUTE_PRINTF(1, 2);

//----------------------------------------------------------------------------
//
// Subroutine-
//       xcb::oops
//
// Purpose-
//       Return strerror(errno)
//
//----------------------------------------------------------------------------
static inline const char* oops( void ) { return strerror(errno); }

//----------------------------------------------------------------------------
//
// Subroutine-
//       xcb::checkstop
//
// Purpose-
//       Handle checkstop condition.
//
//----------------------------------------------------------------------------
extern void
   checkstop(                       // Check stop
     int               line,        // Line number
     const char*       name);       // Function name

//----------------------------------------------------------------------------
//
// Subroutine-
//       xcb::xcbcheck
//
// Purpose-
//       Validate an XCB result.
//
//----------------------------------------------------------------------------
extern void
   xcbcheck(                        // Verify XCB function result
     int               line,        // Line number
     const char*       name,        // Function name
     int               xc);         // Assertion (must be TRUE)

extern void
   xcbcheck(                        // Verify XCB function result
     int               line,        // Line number
     const char*       name,        // Function name
     xcb_generic_error_t* xc);      // Generic error

extern void
   xcbcheck(                        // Verify XCB function result
     int               line,        // Line number
     const char*       name,        // Function name
     void*             xc);         // Object pointer (Must not be nullptr)

//----------------------------------------------------------------------------
//
// Subroutine-
//       xcb::xcbdebug
//
// Purpose-
//       Display an XCB result.
//
//----------------------------------------------------------------------------
extern void
   xcbdebug(                        // Log XCB function result
     int               line,        // Line number
     const char*       name,        // Function name
     int               xc);         // Return code

extern void
   xcbdebug(                        // Log XCB function result
     int               line,        // Line number
     const char*       name,        // Function name
     void*             xc);         // Pointer

//----------------------------------------------------------------------------
//
// Subroutine-
//       xcb::xcberror
//
// Purpose-
//       Error response debugging display.
//
//----------------------------------------------------------------------------
extern void
   xcberror(                        // Error response debugging display
     xcb_generic_error_t*
                       error);      // The error response
}  // namespace xcb
#endif // XCB_GLOBAL_H_INCLUDED
