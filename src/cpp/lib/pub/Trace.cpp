//----------------------------------------------------------------------------
//
//       Copyright (C) 2019-2020 Frank Eskesen.
//
//       This file is free content, distributed under the GNU General
//       Public License, version 3.0.
//       (See accompanying file LICENSE.GPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/gpl-3.0.en.html)
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
//       2020/08/04
//
//----------------------------------------------------------------------------
#include <atomic>                   // For std::atomic
#include <chrono>                   // For SCDM( std::chrono )
#include <time.h>                   // For clock_gettime
#include <mutex>                    // For std::lock_guard
#include <new>                      // For std::bad_alloc
#include <string.h>                 // For memcpy
#include <unistd.h>                 // For sysconf

#include <pub/Debug.h>              // For debugging
#include <pub/Thread.h>             // For SCDM( Thread::current() )
#include <pub/utility.h>            // For pub::utility::dump

#include "pub/Trace.h"
#include "pub/detail/Trace.h"
using namespace _PUB_NAMESPACE::debugging; // For debugging
using namespace _PUB_NAMESPACE::detail::Trace; // Our option namespace

namespace _PUB_NAMESPACE {
//----------------------------------------------------------------------------
// External data areas
//----------------------------------------------------------------------------
Trace*                 Trace::trace= nullptr; // The common Trace object

//----------------------------------------------------------------------------
// The SCDM_record, only used for Special Case Debug Mode
//----------------------------------------------------------------------------
static const char*     SCDM_id= ".TAF"; // Trace Allocation Failure
static const char*     SCDM_name= "Trace.c"; // 8 characters, counting '\0'

struct SCDM_record {                // Special Case Trace::Record
char                   ident[4];    // (Same as pub::Trace::Record)
uint32_t               spins;       // The number of retries
uint64_t               clock;       // (Same as pub::Trace::Record)
Thread*                thread;      // The Thread identifier
char                   name[8];     // This module's name
}; // struct SCDM_record

//----------------------------------------------------------------------------
//
// Method-
//       Trace::Trace
//
// Purpose-
//       Constructor.
//
//----------------------------------------------------------------------------
   Trace::Trace(                    // Constructor
     uint32_t          size)        // Length of trace table
{
   unsigned zero= sizeof(Trace);    // The first record offset
   static_assert( (sizeof(Trace) & (ALIGNMENT-1)) == 0, "Trace.h ALIGNMENT");
// zero +=  (ALIGNMENT - 1);        // Not needed, static_assert verifies it
// zero &= ~(ALIGNMENT - 1);
// size -= zero;

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
// Implementation notes-
//       Storage and size alignment are done here, before construction
//
//----------------------------------------------------------------------------
Trace*                              // -> Trace instance
   Trace::make(                     // Initialize the Trace table
     void*             addr,        // Address of trace table
     size_t            size)        // Length of trace table
{
   if( CHECK ) debugf("%4d HCDM Trace.cpp CHECK active\n", __LINE__);
   if( HCDM  ) debugf("%4d HCDM Trace.cpp HCDM active\n", __LINE__);

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
#if false                           // Limited utility, unused
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
//       A nullptr resultant only occurs as a result of an application error
//       (or when using SCDM, Special Case Debug Mode.)
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
// if( CHECK ) {                    // Check size parameter?
     // Size checks are always enabled:
     //   size == 0: An all too common mistake
     //   size > (available size): The for(;;) loop never exits
     // this->zero is rounded size of header, always the header size
//   if ( size == 0 || size > (this->size - this->zero) ) // If too large
     if ( size == 0 || size > (this->size - sizeof(Trace) ) ) // If too large
       throw std::bad_alloc();      // Parameter error
// } // if( CHECK )

   uint32_t spins= 0;               // Loop retry counter (for SCDM)
   oldV= next.load();
   for(;;)
   {
     last= 0;                       // Indicate not wrapped
     newV= oldV + size;             // Arithmetic overflow is a user error
     if( CHECK ) {                  // Check for arithmetic overflow?
       // Arithmetic overflow can only occur when the size parameter plus the
       // size of the table is greater than UINT32_MAX.
       // This is simple for an application to avoid and while the checking
       // is small, it's not zero.
       if( newV < size )            // If arithmetic overflow
         return nullptr;
     }

     result= (char*)this + oldV;
     if ( newV > this->size )       // If wrap
     {
       last= oldV;
       result= (char*)this + zero;
       newV= size + zero;
     }

     if( next.compare_exchange_weak(oldV, newV) )
       break;

     if( SCDM ) spins++;          // (Compile-time option)
   }

   // Special Case Debug Mode ================================================
   if( SCDM > 0 ) {               // If enabled
     if( spins > SCDM ) {         // If excessive retries
       if( USE_DEACTIVATE )       // (Optionally)
         deactivate();            // Terminate tracing

       SCDM_record* record= (SCDM_record*)result;
       record->thread= Thread::current();
       strcpy(record->name, SCDM_name);
       ((Record*)record)->trace(SCDM_id, spins);

       result= nullptr;           // We do not return this record
     }
   }

   if( HCDM ) {
     // Clarifies record initialization in progress state
     memset(result, 0, size);         // Indicate record not initialized
     memcpy(result, ".000", 4);       // Clarify: record not initialized
     // ((Record*)result)->trace(".000", -1); // Alternate clarify option
   }

   if( last ) {                     // Handle wrap
     wrap++;                        // Update the wrap count
     this->last= last;              // Last valid location

     if( HCDM ) {
       // Clean up any last empty area
       // (Candidate for normal use. Doen't occur often, if at all.)
       // This cleanup makes the last trace entry in the trace table slightly
       // easier to look at, and doesn't occur in the normal case.
       if( last < this->size ) {      // If zombie data exists
         char* atlast= (char*)this + last;
         memset(atlast, 0, this->size - last);
         memcpy(atlast, ".END", 4);
       }
     }
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
   tracef("..wrap(%lu) zero(0x%.2x) last(0x%.8x) size(0x%.8x) next(0x%.8x)\n",
          wrap, zero, last, size, next.load());
   utility::dump(debug->get_FILE(), this, size, nullptr);
}

//----------------------------------------------------------------------------
//
// Method-
//       Trace::Record::trace
//
// Purpose-
//       Initialize the Trace::Record
//
// Implementation note-
//       Performance critical path.
//
//----------------------------------------------------------------------------
static inline void
   set_clock(                       // Set the clock
     Trace::Record*    record)      // For this Trace::Record
{
   struct timespec     clock;       // UTC time base

   clock_gettime(CLOCK_REALTIME, &clock); // Get UTC time base
   uint64_t usec= clock.tv_sec * 1000000000;
   usec += clock.tv_nsec;
   record->clock= usec;
}

void
   Trace::Record::trace(            // Initialize the Trace::Record
     const char*       ident)       // The trace type identifier
{
   set_clock(this);                 // Set the clock

   memcpy(this->ident, ident, sizeof(this->ident));
}

void
   Trace::Record::trace(            // Initialize the Trace::Record
     const char*       ident,       // The trace type identifier
     uint32_t          unit)        // The trace unit identifier
{
   set_clock(this);                 // Set the clock

   this->unit= unit;
   memcpy(this->ident, ident, sizeof(this->ident));
}
}  // namespace _PUB_NAMESPACE
