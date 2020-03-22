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
//       DispatchTask.h
//
// Purpose-
//       Standard Dispatch Task object.
//
// Last change date-
//       2020/01/13
//
//----------------------------------------------------------------------------
#ifndef _PUB_DISPATCHTASK_H_INCLUDED
#define _PUB_DISPATCHTASK_H_INCLUDED

#include "Dispatch.h"               // The Dispatcher
#include "DispatchItem.h"           // Dispatch work Item
#include "Worker.h"                 // Worker Thread Pool

namespace _PUB_NAMESPACE::Dispatch {
//----------------------------------------------------------------------------
//
// Class-
//       Dispatch::Task
//
// Purpose-
//       The Dispatch Task.
//
// Notes-
//       Multiple threads may simultaneously add work items to the work Item
//       list. They are processed in FIFO order by the worker, one Item at a
//       time, in single-threaded mode, by the work() method.
//
//       A Worker either completes the work Item by invoking its post() method
//       or enqueues it onto another Task.
//
//----------------------------------------------------------------------------
class Task : public Worker {        // Dispatch Task
//----------------------------------------------------------------------------
// Task::Attributes
//----------------------------------------------------------------------------
protected:
AU_List<Item>          itemList;    // The Work item list

//----------------------------------------------------------------------------
// Task::Constructors
//----------------------------------------------------------------------------
public:
virtual
   ~Task( void ) {}                 // Destructor
   Task( void )                     // Constructor
:  Worker(), itemList() {}

   Task(const Task&) = delete;      // Disallowed copy constructor
   Task& operator=(const Task&) = delete; // Disallowed assignment operator

//----------------------------------------------------------------------------
// Task::Methods
//----------------------------------------------------------------------------
public:
inline int                          // TRUE iff Task has no enqueued work
   isIdle( void ) const             // Is this Task idle?
{
   return (itemList.getTail() == nullptr);
}

inline int                          // TRUE iff Task has enqueued work
   isBusy( void ) const             // Is this Task busy?
{
   return (itemList.getTail() != nullptr);
}

virtual void
   debug( void ) const;             // Debugging display

void
   enqueue(                         // Enqueue
     Item*             item)        // This work Item
{
   Item* tail= itemList.fifo(item); // Insert work Item
   if( tail == nullptr )            // If the list was empty
     WorkerPool::work(this);        // Schedule this Task
}

void
   reset( void )                    // Reset the itemList
{  itemList.reset(); }

virtual void                        // The Worker interface
   work( void );                    // Drain work from Task

protected:
virtual void                        // (IMPLEMENT this method)
   work(                            // Process
     Item*             item);       // This work Item
/* item->post(); */
}; // class Task
}  // namespace _PUB_NAMESPACE::Dispatch
#endif // _PUB_DISPATCHTASK_H_INCLUDED
