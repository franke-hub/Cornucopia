//----------------------------------------------------------------------------
//
//       Copyright (c) 2019-2020 Frank Eskesen.
//
//       This file is free content, distributed under the Lesser GNU
//       General Public License, version 3.0.
//       (See accompanying file LICENSE.LGPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/lgpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       Trace.cpp
//
// Purpose-
//       Trace object methods.
//
// Last change date-
//       2020/06/20
//
//----------------------------------------------------------------------------
#include <atomic>                   // Atomic operation required
#include <time.h>                   // For clock_gettime
#include <mutex>                    // For std::lock_guard
#include <new>                      // For std::bad_alloc
#include <string.h>                 // For memcpy

#include <pub/Debug.h>              // For debugging
#include <pub/utility.h>            // For pub::utility::dump

#include "pub/Trace.h"
using namespace _PUB_NAMESPACE::debugging; // For debugging

namespace _PUB_NAMESPACE {
//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#ifndef CHECK                       // If defined, enhanced error checking
#undef  CHECK
#endif

#ifndef HCDM                        // If defined, Hard Core Debug Mode
#undef  HCDM
#endif

#include <pub/ifmacro.h>

//----------------------------------------------------------------------------
// External data areas
//----------------------------------------------------------------------------
Trace*                 Trace::trace= nullptr; // The common Trace object

//----------------------------------------------------------------------------
//
// Method-
//       Trace::Trace
//
// Purpose-
//       Constructor.
//
// Implementation notes-
//       make() validates alignment and size, initializes storage to zero.
//
//----------------------------------------------------------------------------
   Trace::Trace(                    // Constructor
     uint32_t          size)        // Length of trace table
{
   unsigned zero= sizeof(Trace);    // The first record offset
   zero +=  (ALIGNMENT - 1);
   zero &= ~(ALIGNMENT - 1);
   size -= zero;

   this->zero= zero;
   this->last= size;
   this->size= size;
   this->next= zero;
}

//----------------------------------------------------------------------------
//
// Method-
//       Trace::make
//
// Purpose-
//       Initialize the Trace table
//
//----------------------------------------------------------------------------
Trace*                              // -> Trace instance
   Trace::make(                     // Initialize the Trace table
     void*             addr,        // Address of trace table
     size_t            size)        // Length of trace table
{
   IFCHECK( debugf("%4d HCDM Trace.cpp CHECK active\n", __LINE__); )
   IFHCDM( debugf("%4d HCDM Trace.cpp HCDM active\n", __LINE__); )

   if( addr == nullptr              // Reject invalid parameters
       || size < TABLE_SIZE_MIN || size > TABLE_SIZE_MAX )
     throw std::bad_alloc();

   unsigned diff= (uintptr_t)addr & (ALIGNMENT-1);
   if( diff != 0 ) {                // If misaligned storage
     diff= ALIGNMENT - diff;
     size -= diff;
     addr= (void*)((uintptr_t)addr + diff);
   }
   size &= ~(ALIGNMENT - 1);

   memset(addr, 0, size);           // Initialize to zeros
   Trace* trace= new(addr) Trace((uint32_t)size); // Construct the Trace object
   trace->flag[X_OFFSET]= diff;     // (No real need for this field.)

   return trace;
}

//----------------------------------------------------------------------------
//
// Method-
//       Trace::take
//
// Purpose-
//       Atomically take and reset the commont the Trace object.
//
// Implementation note-
//       Not needed, at least yet. Trace::trace= nullptr suffices.
//
//----------------------------------------------------------------------------
#if 0                               // Limited utility, unused
Trace*                              // Replaced Trace::trace
   Trace::take( void )              // Reset the global Trace object
{
   std::atomic<Trace*>* atomic_p= (std::atomic<Trace*>*)&trace;

   Trace* old= atomic_p->load();
   while( !atomic_p->compare_exchange_weak(old, nullptr) )
     ;

   return old;
}
#endif

//----------------------------------------------------------------------------
//
// Method-
//       Trace::allocate
//
// Purpose-
//       Allocate storage.
//
// Implementation note-
//       Performance critical path.
//       A nullptr resultant only occurs as a result of an application error.
//
//----------------------------------------------------------------------------
void*                               // Resultant
   Trace::allocate(                 // Allocate a trace record
     uint32_t          size)        // of this length
{
   void*               result;      // Resultant

   uint32_t            newV;        // New value
   uint32_t            oldV;        // Old value
   uint32_t            last;        // Last oldV when wrapped

   size +=  (ALIGNMENT - 1);
   size &= ~(ALIGNMENT - 1);
// IFCHECK( // Check size parameter?
     // These size checks are always enabled:
     //   size == 0: An all too common mistake
     //   size > (available size): The for(;;) loop never exits
//// if ( size == 0 || size > (this->size - this->zero) ) // (The real test)
     if ( size == 0 || size > (this->size - sizeof(Trace)) )
       return nullptr;
// ) // IFCHECK

   oldV= next.load();
   for(;;)
   {
     last= 0;                       // Indicate not wrapped
     newV= oldV + size;             // Arithmetic overflow is a user error
IFCHECK( // Check for arithmetic overflow?
     // Arithmetic overflow can only occur when the size parameter plus the
     // size of the table is greater than UINT32_MAX.
     // This is simple for an application to avoid and while the checking
     // is small, it's not zero.
     if( newV < size )              // If arithmetic overflow
       return nullptr;
) // IFCHECK

     result= (char*)this + oldV;
     if ( newV > this->size )       // If wrap
     {
       last= oldV;
       result= (char*)this + zero;
       newV= size + zero;
     }

     if( next.compare_exchange_strong(oldV, newV) )
       break;
   }

IFHCDM( // Clarifies record initialization in progress state
   memset(result, 0, size);         // Indicate record not initialized
   memcpy(result, ".000", 4);       // Clarify: record not initialized
// ((Record*)result)->init(".000", -1); // Alternate clarify option
) // IFHCDM

   if( last ) {                     // Handle wrap
     wrap++;                        // (Atomically) update the wrap count
     this->last= last;              // Last valid location

IFHCDM( // Clean up any last empty area
     // (Candidate for normal use. Doen't occur often, if at all.)
     // This cleanup makes the last trace entry in the trace table slightly
     // easier to look at, and doesn't occur in the normal case.
     if( last < this->size ) {      // If zombie data exists
       char* atlast= (char*)this + last;
       memset(atlast, 0, this->size - last);
       memcpy(atlast, ".END", 4);
     }
) // IFHCDM
   }

   return result;
}

//----------------------------------------------------------------------------
//
// Method-
//       Trace::dump
//
// Purpose-
//       Dump the trace table.
//
// Implementation note-
//       For readability the global Debug mutex is held during the entire
//       dump process.
//
//----------------------------------------------------------------------------
void
   Trace::dump( void ) const        // Dump the trace table
{
   Debug* debug= Debug::get();
   std::lock_guard<decltype(*debug)> lock(*debug);

   tracef("Trace(%p)::dump\n", this);
   tracef("..wrap(%ld) zero(0x%.2x) last(0x%.8x) size(0x%.8x) next(0x%.8x)\n",
          wrap.load(), zero, last, size, next.load());
   utility::dump(debug->get_FILE(), this, size, nullptr);
}

//----------------------------------------------------------------------------
//
// Method-
//       Trace::Record::init
//
// Purpose-
//       Initialize the Trace::Record
//
// Implementation note-
//       Performance critical path.
//
//----------------------------------------------------------------------------
void
   Trace::Record::init(             // Initialize the Trace::Record
     const char*       ident,       // The trace type identifier
     uint32_t          iitem)       // The trace item identifier
{
   struct timespec     clock;       // UTC time base

   clock_gettime(CLOCK_REALTIME, &clock); // Get UTC time base
   uint64_t usec= clock.tv_sec * 1000000000;
   usec += clock.tv_nsec;
   this->clock= usec;

   this->iitem= iitem;
   memcpy(this->ident, ident, sizeof(this->ident));
}
}  // namespace _PUB_NAMESPACE
