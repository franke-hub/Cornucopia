//----------------------------------------------------------------------------
//
//       Copyright (c) 2007-2020 Frank Eskesen.
//
//       This file is free content, distributed under the Lesser GNU
//       General Public License, version 3.0.
//       (See accompanying file LICENSE.LGPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/lgpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       Test_bug.cpp
//
// Purpose-
//       Test debugging methods.
//
// Last change date-
//       2020/10/03
//
//----------------------------------------------------------------------------
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "pub/Debug.h"

using _PUB_NAMESPACE::Debug;
using namespace _PUB_NAMESPACE::debugging;

//----------------------------------------------------------------------------
// Internal data areas
//----------------------------------------------------------------------------
static pub::Debug      debug;         // Debug object

//----------------------------------------------------------------------------
//
// Subroutine-
//       main
//
// Purpose-
//       Mainline code.
//
//----------------------------------------------------------------------------
extern int                          // Return code
   main(int, char**)                // Mainline code
//   int               argc,        // Argument count (UNUSED)
//   char*             argv[])      // Argument array (UNUSED)
{
   char                buff[128];

   for(size_t i=0; i<sizeof(buff); i++)
     buff[i]= 0x80 + i;

   debug_set_mode(Debug::MODE_DEFAULT);
   debugf("Standard mode:\n");
   debugf("This appears in %s and %s\n", "TRACE", "STDOUT");
   errorf("This appears in %s and %s\n", "TRACE", "STDERR");
   tracef("This appears in %s ONLY\n",   "TRACE");
   debugh("This appears in %s and %s\n", "TRACE", "STDOUT");
   errorh("This appears in %s and %s\n", "TRACE", "STDERR");
   traceh("This appears in %s ONLY\n",   "TRACE");

   debug_set_mode(Debug::MODE_IGNORE);
   errno= ESRCH;
   errorp("Ignore mode: (ignored) This appears in STDERR (only)");
   debugf("Ignore mode:\n");
   errorf("Ignore mode:\n");
   tracef("Ignore mode:\n");
   debugh("Ignore mode:\n");
   errorh("Ignore mode:\n");
   traceh("Ignore mode:\n");

   debug_set_mode(Debug::MODE_INTENSIVE);
   debugf("Intensive mode:\n");
   debugf("This appears in %s and %s\n", "TRACE", "STDOUT");
   errorf("This appears in %s and %s\n", "TRACE", "STDERR");
   tracef("This appears in %s ONLY\n",   "TRACE");
   debugh("This appears in %s and %s\n", "TRACE", "STDOUT");
   errorh("This appears in %s and %s\n", "TRACE", "STDERR");
   traceh("This appears in %s ONLY\n",   "TRACE");

   return 0;                        // Normal completion
}
