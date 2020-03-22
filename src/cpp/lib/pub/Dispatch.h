//----------------------------------------------------------------------------
//
//       Copyright (c) 2018-2020 Frank Eskesen.
//
//       This file is free content, distributed under the Lesser GNU
//       General Public License, version 3.0.
//       (See accompanying file LICENSE.LGPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/lgpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       Dispatch.h
//
// Purpose-
//       Work dispatcher, including local definitions.
//
// Last change date-
//       2020/01/21
//
//----------------------------------------------------------------------------
#include <assert.h>                 // For assert
#include <mutex>                    // For std::lock_guard

#include <pub/Clock.h>              // DispatchTTL completion time
#include <pub/Debug.h>              // For debugging
#include <pub/Latch.h>              // Dispatch::Timers mutex
#include <pub/LinkedList.h>         // Dispatch::Task itemList
#include <pub/Named.h>              // Dispatch::Timers is a Named Thread
#include <pub/Semaphore.h>          // Dispatch::Timers event
#include <pub/Worker.h>             // Dispatch::Task base class

#include "pub/Dispatch.h"           // Include visible class definitions
using namespace _PUB_NAMESPACE::debugging; // Enable debugging functions

//----------------------------------------------------------------------------
// Constants for parameterization
//----------------------------------------------------------------------------
#ifndef HCDM
#undef  HCDM                        // If defined, Hard Core Debug Mode
#endif

#ifndef logf
#define logf traceh                 // Alias for trace w/header
#endif

//----------------------------------------------------------------------------
// Dependent macros
//----------------------------------------------------------------------------
#include <pub/ifmacro.h>

namespace _PUB_NAMESPACE::Dispatch {
//----------------------------------------------------------------------------
//
// Class-
//       DispatchTTL
//
// Purpose-
//       Dispatch Timer Thread Link: Keep track of delay request.
//
//----------------------------------------------------------------------------
class DispatchTTL : public Link_List<DispatchTTL>::Link {
//----------------------------------------------------------------------------
// DispatchTTL::Attributes
//----------------------------------------------------------------------------
public:
Clock const            time;        // The proposed completion time
Item* const            item;        // The work Item

//----------------------------------------------------------------------------
// DispatchTTL::Constructors
//----------------------------------------------------------------------------
public:
   ~DispatchTTL( void ) {}          // Destructor
   DispatchTTL(                     // Constructor
     Clock&            time,        // Completion time
     Item*             item)        // WorkUnit object
:  Link_List<DispatchTTL>::Link(), time(time), item(item) {}
}; // class DispatchTTL

//----------------------------------------------------------------------------
//
// Class-
//       Timers
//
// Purpose-
//       Handle time delay requests.
//
//----------------------------------------------------------------------------
class Timers : public Thread, public Named {
//----------------------------------------------------------------------------
// Timers::Attributes
//----------------------------------------------------------------------------
protected:
Semaphore              event;       // Synchronization event object
Link_List<DispatchTTL> list;        // Ordered list of pending events
Latch                  mutex;       // Synchronization mutex
bool                   operational; // TRUE iff operational

//----------------------------------------------------------------------------
// Timers::Constructors
//----------------------------------------------------------------------------
public:
virtual
   ~Timers( void ) {}               // Destructor
   Timers( void )                   // Constructor
:  Thread(), Named("DispatchTime")
,  event(), list(), mutex(), operational(true)
{  start(); }

//----------------------------------------------------------------------------
// Timers::Methods
//----------------------------------------------------------------------------
public:
void
   cancel(                          // Cancel
     void*             token)       // This timer event
{
   std::lock_guard<decltype(mutex)> lock(mutex);

   DispatchTTL* link= list.getHead();
   while( link ) {
     if( (void*)link == token ) {
       list.remove(link, link);
       link->item->post(Item::CC_PURGE);
       delete link;
       break;
     }
   }
}

void*                               // Cancellation token
   delay(                           // Delay for
     double            seconds,     // This many seconds, then
     Item*             item)        // Complete this work Item
{
   if( seconds < 0.015625 ) {       // If interval too short
     item->post();
     return nullptr;
   }

   Clock time(Clock::now() + seconds);
   DispatchTTL* link= new DispatchTTL(time, item);

   std::lock_guard<decltype(mutex)> lock(mutex);

   if( !operational ) {
     item->post(Item::CC_PURGE);
     delete link;
     return nullptr;
   }

   DispatchTTL* after= nullptr;
   DispatchTTL* work= list.getHead();
   while( work ) {
     if( link->time < work->time )
       break;

     after= work;
     work= work->getNext();
   }

   list.insert(after, link, link);
   if( after == nullptr )           // If new head of list
     event.post();                  // Use the new timeout

   return link;
}

virtual void                        // Operate the Thread
   run()
{
   IFHCDM( traceh("Dispatch::Timers running..."); )

   while( operational ) {
     double delay= 60.0;            // Minimum wait delay (seconds)

     {{{{
       std::lock_guard<decltype(mutex)> lock(mutex);

       // Drive all expired timers
       DispatchTTL* link= list.getHead();
       while( link ) {
         delay= link->time.get() - Clock::now();
         if( delay > 0.015625 ) {
           if( delay > 60.0 )
             delay= 60.0;

           break;
         }

         list.remove(link, link);
         link->item->post();
         delete link;

         link= list.getHead();
       }
     }}}}

     event.wait(delay);
   }

   // Non-operational. Purge all timers before we go.
   std::lock_guard<decltype(mutex)> lock(mutex);

   DispatchTTL* link= list.getHead();
   while( link ) {
     list.remove(link, link);
     link->item->post(Item::CC_PURGE);
     delete link;

     link= list.getHead();
   }

   IFHCDM( traceh("Dispatch::Timers ...terminated"); )
}

virtual void
   stop( void )                     // Terminate the Thread
{
   std::lock_guard<decltype(mutex)> lock(mutex);

   operational= false;
   event.post();
}
}; // class Timers
}  // namespace _PUB_NAMESPACE::Dispatch
