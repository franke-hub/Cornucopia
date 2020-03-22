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
//       LinkedList.h
//
// Purpose-
//       Describe the LinkedList objects.
//
// Last change date-
//       2020/01/27
//
// Implementation notes-
//       For all LinkedList classes, the isCoherent and isOnList methods run
//       in linear time. Method isCoherent examines the entire LinkedList and
//       method isOnList examines the LinkedList until the element is found.
//
//       By convention, if it exists in the implementation, the head Link
//       is the oldest Link, the Link that will be removed by REMQ. Likewise
//       the tail Link, if it exists in the implementation, is the newest.
//       It is the final Link on the LinkedList available to REMQ.
//
//       If a LinkedList contains more than an implementation defined maximum
//       Link element count, method isCoherent reports FALSE. (Currently 1G)
//
// LinkedList types-
//       AU_List<T>:    Atomic Update Linked List, the only thread-safe List.
//       DHDL_List<T>:  Doubly Headed Doubly Linked List.
//       DHSL_List<T>:  Doubly Headed Singly Linked List.
//       SHSL_List<T>:  Single Headed Singly Linked List.
//       Sort_List<T>:  privately derived from DHDL_List<T>, implementing all
//                      methods. A sort method is added, and a user-defined
//                      compare method must be provided.
//       Link_List<T>:  Equivalent to DHDL_List<T>.
//
//       In each case, the associated Link class is defined within the
//       LinkedList class. All Link classes must be derived from that
//       LinkedList::Link class.
//       An example follows:
//
// Example declaration and usage-
//       class My_Link : public Link_List<My_List>::Link {
//       public:
//         My_Link : Link_List<My_List>::Link() { ... }
//         // Remainder of implementation of My_Link
//       }; // class My_Link, the elements to be put on a class My_List
//
//       Link_List<My_Link> my_list1;  // A List containing My_Link elements
//       Link_List<My_Link> my_list2;  // Another List contining My_Link Links
//
//       My_Link* link1= new My_Link(); // Create a new My_Link
//       my_list1.fifo(link1);         // Insert it (fifo) onto my_list1
//       My_link* link2= my_list1.remq(); // Then remove it, emptying my_list1
//       assert( link1 == link2 );     // The link added is the link removed
//       my_list2.fifo(link2);         // Insert it (lifo) onto my_list2
//       // my_list1 is empty, my_list2 contains (only) link2
//
//----------------------------------------------------------------------------
#ifndef _PUB_LINKEDLIST_H_INCLUDED
#define _PUB_LINKEDLIST_H_INCLUDED

#include <atomic>                   // For std::atomic

#include "config.h"                 // For _PUB_NAMESPACE

namespace _PUB_NAMESPACE {
//----------------------------------------------------------------------------
// Forward references
//----------------------------------------------------------------------------
template<class T> class AU_List;    // Atomic update List
template<class T> class DHDL_List;  // DHDL List
template<class T> class DHSL_List;  // DHSL List
template<class T> class Link_List;  // List, equivalent to DHDL_List
template<class T> class SHSL_List;  // SHSL List
template<class T> class Sort_List;  // Sort List

//----------------------------------------------------------------------------
//
// Class-
//       AU_List
//
// Purpose-
//       The Atomic Update List is a thread-safe FIFO LinkedList.
//
// Implementation notes-
//       The FIFO and RESET methods generally run in constant time.
//       The REMQ methods runs in (list size dependent) linear time.
//
//       The Atomic Update List is optimized for sequential FIFO insertion
//       onto a single list by multiple threads. Insertion, via the FIFO
//       method, is thread safe and may be used concurrently with any other
//       method. Multiple threads may use the FIFO method concurrently.
//
//       The REMQ method itself is not thread safe. The REMQ method must
//       only be invoked from a single thread, or otherwise serialized.
//       (This also applies to the isCoherent and isOnList methods.)
//
//       Since the element chain must be traversed in order to find the oldest
//       element, this mechanism is best suited for cases where the List is
//       kept to a small number of elements.
//
//       The AU_List is a Singly Headed Singly Linked List maintained in
//       reverse Link sequence: It begins with the newest (tail) Link and
//       continues via its prev Link. The final Link's prev Link is nullptr.
//
//       Methods LIFO, INSERT, and REMOVE (available on other List types)
//       are not provided: No known thread-safe implementation exists.
//
//----------------------------------------------------------------------------
template<> class AU_List<void> {    // AU_List base
public:
class Link {                        // AU_List<void>::Link
protected:
Link*                  prev;        // -> Prior (next older) Link

public:
inline Link*                        // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return prev; }

inline void
   setPrev(                         // Set prior Link
     Link*             link)        // -> Prior Link
{  prev= link; }
}; // class AU_List<void>::Link

//----------------------------------------------------------------------------
// AU_List::Attributes
//----------------------------------------------------------------------------
protected:
std::atomic<Link*>     tail;        // -> Tail (newest) Link

//----------------------------------------------------------------------------
// AU_List::Constructors
//----------------------------------------------------------------------------
public:
   ~AU_List( void );                // Destructor
   AU_List( void );                 // Constructor

   AU_List(const AU_List&) = delete; // Disallowed copy constructor
AU_List& operator=(const AU_List&) = delete; // Disallowed assignment operator

//----------------------------------------------------------------------------
// AU_List::Accessors
//----------------------------------------------------------------------------
public:
//----------------------------------------------------------------------------
//
// Method-
//       AU_List::getTail
//
// Purpose-
//       Get the tail (newest) Link.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//
//----------------------------------------------------------------------------
Link*                               // -> The tail (newest) Link
   getTail( void ) const            // Get tail (newest) Link
{  return tail.load(); }

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::fifo
//
// Purpose-
//       Insert a link onto the list with FIFO ordering.
//
// Implementation notes-
//       This operation IS thread safe.
//
//----------------------------------------------------------------------------
Link*                               // -> Prior tail
   fifo(                            // Insert (FIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::isCoherent
//
// Purpose-
//       Coherency check.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//
//----------------------------------------------------------------------------
int                                 // TRUE if the object is coherent
   isCoherent( void ) const;        // Coherency check

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::isOnList
//
// Purpose-
//       Test whether Link is present in this List.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//
//----------------------------------------------------------------------------
int                                 // TRUE if link is contained
   isOnList(                        // Is link contained?
     Link*             link) const; // -> Link

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::remove
//
// Purpose-
//       Remove a  link from the list.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//
//----------------------------------------------------------------------------
Link*                               // -> Removed Link
   remove(                          // Remove link
     Link*             link);       // The link to remove

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::remq
//
// Purpose-
//       Remove the oldest link from the list.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//
//----------------------------------------------------------------------------
Link*                               // -> Removed (oldest) Link
   remq( void );                    // Remove oldest link

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::reset
//
// Purpose-
//       Remove ALL Links from the List.
//
// Implementation notes-
//       Only the consumer thread can safely use this method.
//       The links are ordered from newest to oldest.
//
//----------------------------------------------------------------------------
Link*                               // -> The set of removed Links
   reset( void );                   // Reset (empty) the List

//----------------------------------------------------------------------------
//
// Method-
//       AU_List::swap
//
// Purpose-
//       Remove ALL Links from the List, replacing the List
//
// Implementation notes-
//       Consumers use this method to replace the current List with a dummy
//       Link, and then process all the current List elements. When this
//       processing completes, the process is repeated using the same dummy
//       Link. If no new elements were added during processing, swap returns
//       nullptr. If new elements were added, swap returns the new List with
//       the dummy element as the oldest element.
//
//       If (atomically) the current tail == nullptr or the replacement tail,
//       the List remains or is emptied and nullptr is returned.
//
//       Only the consumer thread can safely use this method.
//       The returned Links are ordered from newest to oldest.
//       Use swap in conjunction with an AU_FIFO to efficiently reorder the
//       removed links (for FIFO ordering). Sample code:
//         AU_List<Item>       itemList;    // The Work item list
//         :
//         Item                only;        // Our fake work Item
//         Item*               fake= &only;
//
//         Item* list= itemList.swap(fake); // Replace List with fake element
//         AU_FIFO<Item> fifo(list);        // Initialize the FIFO
//         for(;;) {                        // Drain the itemList
//           Item* item= fifo.remq();       // Get oldest link
//           if( item == nullptr ) {        // If none remain
//             list= itemList.swap(fake);   // Get new list
//             if( list == nullptr )        // If none left
//               return;                    // (or break)
//
//             fifo.reset(list);            // Re-initialize the fifo
//             item= fifo.remq();           // Remove oldest link (the fake item)
//             assert( item == fake );      // Verify what we think we know
//             item->setPrev(nullptr);      // The fake item ends the AU_List
//
//             item= fifo.remq();           // Remove oldest link
//             assert( item != nullptr );   // Which can not be a nullptr
//           }
//
//           // Process the removed Item (*item)
//         }
//
//----------------------------------------------------------------------------
Link*                               // -> The set of removed Links
   swap(                            // Swap (empty) the List
     Link*             tail);       // Replacing it with this link
}; // class AU_List<void>

//----------------------------------------------------------------------------
//
// Class-
//       AU_List<T>
//
// Purpose-
//       Typed AU_List object, where T is of class AU_List<T>::Link.
//
//----------------------------------------------------------------------------
template<class T>
class AU_List : public AU_List<void> { // AU_List<T>
public:
class Link : public AU_List<void>::Link { // AU_List<T>::Link
public:
inline T*                           // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return static_cast<T*>(prev); }
}; // class AU_List<T>::Link

public:
T*                                  // -> Tail (newest) Link
   getTail( void ) const            // Get tail link
{  return static_cast<T*>(AU_List<void>::getTail()); }

T*                                  // -> Prior tail
   fifo(                            // Insert (fifo order)
     T*                link)        // -> Link to insert
{  return static_cast<T*>(AU_List<void>::fifo(link)); }

T*                                  // Removed link (if on list)
   remove(                          // Remove Link
     T*                link)        // The link to remove
{  return static_cast<T*>(AU_List<void>::remove(link)); }

T*                                  // Removed Link
   remq( void )                     // Remove oldest Link
{  return static_cast<T*>(AU_List<void>::remq()); }

T*                                  // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return static_cast<T*>(AU_List<void>::reset()); }

T*                                  // (See above)
   swap(                            // Replace List
     T*                list)        // With this one
{  return static_cast<T*>(AU_List<void>::swap(list)); }
}; // class AU_List<T>

//----------------------------------------------------------------------------
//
// Class-
//       AU_FIFO
//
// Purpose-
//       A helper class used to improve AU_List::remq performance.
//       (See sample code in AU_List<void>::swap() above.)
//
//----------------------------------------------------------------------------
template<class T>                   // T is an AU_List::Link
class AU_FIFO {                     // AU_FIFO helper class
protected:
T*                     head;        // The current head

public:
//----------------------------------------------------------------------------
// AU_FIFO::Constructor
//----------------------------------------------------------------------------
   AU_FIFO(                         // Constructor
     T*                tail)        // The list
:  head(nullptr)
{  reset(tail); }                   // Initial conversion

//----------------------------------------------------------------------------
// AU_FIFO::remq: Obtain next element
//----------------------------------------------------------------------------
T*                                  // The oldest Link
   remq( void )                     // Get oldest Link
{
   T* result= head;

   if( result != nullptr )
     head= result->getPrev();

   return result;
}

//----------------------------------------------------------------------------
// AU_FIFO::reset: (Re-)initialize the list
//----------------------------------------------------------------------------
void
   reset(                           // Re-initialize the list
     T*                tail)        // The list
{
   while( tail ) {
     T* prev= tail->getPrev();
     tail->setPrev(head);
     head= tail;
     tail= prev;
   }
}
}; // class AU_FIFO

//----------------------------------------------------------------------------
//
// Class-
//       DHDL_List
//
// Purpose-
//       The Doubly Headed, Doubly Linked List is a general purpose List.
//
// Implementation notes-
//       The DHDL_List is not thread safe. Method usage must be serialized.
//       The FIFO, LIFO, INSERT, and REMOVE methods run in constant time.
//
//----------------------------------------------------------------------------
template<> class DHDL_List<void> {  // DHDL_List base
public:
class Link {                        // DHDL_List<void>::Link
protected:
Link*                  next;        // -> Forward Link
Link*                  prev;        // -> Reverse Link

public:
inline Link*                        // -> Next Link
   getNext( void ) const            // Get next Link
{  return next; }

inline Link*                        // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return prev; }

inline void
   setNext(                         // Set next Link
     Link*             link)        // -> Next Link
{  next= link; }

inline void
   setPrev(                         // Set prior Link
     Link*             link)        // -> Prior Link
{  prev= link; }
}; // class DHDL_List<void>::Link

//----------------------------------------------------------------------------
// DHDL_List::Attributes
//----------------------------------------------------------------------------
protected:
Link*                  head;        // -> Head Link
Link*                  tail;        // -> Tail Link

//----------------------------------------------------------------------------
// DHDL_List::Constructors
//----------------------------------------------------------------------------
public:
   ~DHDL_List( void );              // Default destructor
   DHDL_List( void );               // Default constructor

private:                            // Bitwise copy prohibited
   DHDL_List(const DHDL_List&);
DHDL_List&
   operator=(const DHDL_List&);

//----------------------------------------------------------------------------
// DHDL_List::Accessors
//----------------------------------------------------------------------------
public:
inline Link*                        // -> Oldest Link on List
   getHead( void ) const            // Get head link
{  return head; }

inline Link*                        // -> Newest Link on List
   getTail( void ) const            // Get tail link
{  return tail; }

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::fifo
//
// Purpose-
//       Insert a link onto the list with FIFO ordering.
//
//----------------------------------------------------------------------------
void
   fifo(                            // Insert (FIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::insert
//
// Purpose-
//       Insert a chain of elements onto the list at the specified position.
//
//----------------------------------------------------------------------------
void
   insert(                          // Insert at position,
     Link*             link,        // -> Link to insert after
     Link*             head,        // -> First Link to insert
     Link*             tail);       // -> Final Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::isCoherent
//
// Purpose-
//       List coherency check.
//
//----------------------------------------------------------------------------
int                                 // TRUE if the object is coherent
   isCoherent( void ) const;        // Coherency check

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::isOnList
//
// Purpose-
//       Test whether Link is present in this List.
//
//----------------------------------------------------------------------------
int                                 // TRUE if link is contained
   isOnList(                        // Is Link contained?
     Link*             link) const; // -> Link

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::lifo
//
// Purpose-
//       Insert a link onto the list with LIFO ordering.
//
//----------------------------------------------------------------------------
void
   lifo(                            // Insert (LIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::remove
//
// Purpose-
//       Remove a chain of elements from the list.
//
//----------------------------------------------------------------------------
void
   remove(                          // Remove from list
     Link*             head,        // -> First Link to remove
     Link*             tail);       // -> Final Link to remove

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::remq
//
// Purpose-
//       Remove the oldest link from the list.
//
//----------------------------------------------------------------------------
Link*                               // -> Removed Link
   remq( void );                    // Remove oldest Link

//----------------------------------------------------------------------------
//
// Method-
//       DHDL_List::reset
//
// Purpose-
//       Remove ALL Links from the List.
//
//----------------------------------------------------------------------------
Link*                               // The set of removed Links
   reset( void );                   // Reset (empty) the list
}; // class DHDL_List

//----------------------------------------------------------------------------
//
// Class-
//       DHDL_List<T>
//
// Purpose-
//       Typed DHDL_List object, where T is of class DHDL_List<T>::Link.
//
//----------------------------------------------------------------------------
template<class T>
class DHDL_List : public DHDL_List<void> { // DHDL_List<T>
public:
class Link : public DHDL_List<void>::Link { // DHDL_List<T>::Link
public:
inline T*                           // -> Next Link
   getNext( void ) const            // Get next Link
{  return static_cast<T*>(next); }

inline T*                           // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return static_cast<T*>(prev); }
}; // class DHDL_List<void>::Link

public:
T*                                  // -> Oldest T* on List
   getHead( void ) const            // Get head link
{  return static_cast<T*>(head); }

T*                                  // -> Newest T* on List
   getTail( void ) const            // Get tail link
{  return static_cast<T*>(tail); }

T*                                  // Removed T*
   remq( void )                     // Remove oldest link
{  return static_cast<T*>(DHDL_List<void>::remq()); }

T*                                  // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return static_cast<T*>(DHDL_List<void>::reset()); }
}; // class DHDL_List<T>

//----------------------------------------------------------------------------
//
// Class-
//       DHSL_List
//
// Purpose-
//       The Doubly Headed, Singly Linked List.
//
// Implementation notes-
//       The DHSL_List is not thread safe. Method usage must be serialized.
//
//       The FIFO, LIFO, REMQ, and RESET methods run in constant time.
//       The INSERT and REMOVE methods run in linear time.
//
//----------------------------------------------------------------------------
template<> class DHSL_List<void> {  // DHSL_List base
public:
class Link {                        // DHSL_List<void>::Link
protected:
Link*                  next;        // -> Forward Link

public:
inline Link*                        // -> Next Link
   getNext( void ) const            // Get next Link
{  return next; }

inline void
   setNext(                         // Set next Link
     Link*             link)        // -> Next Link
{  next= link; }
}; // class DHSL_List<void>::Link

//----------------------------------------------------------------------------
// DHSL_List::Attributes
//----------------------------------------------------------------------------
protected:
Link*                  head;        // -> Head Link
Link*                  tail;        // -> Tail Link

//----------------------------------------------------------------------------
// DHSL_List::Constructors
//----------------------------------------------------------------------------
public:
   ~DHSL_List( void );              // Default destructor
   DHSL_List( void );               // Default constructor

private:                            // Bitwise copy prohibited
   DHSL_List(const DHSL_List&);
DHSL_List&
   operator=(const DHSL_List&);

//----------------------------------------------------------------------------
// DHSL_List::Accessors
//----------------------------------------------------------------------------
public:
inline Link*                        // -> Newest Link on List
   getHead( void ) const            // Get head Link
{  return head; }

inline Link*                        // -> Oldest Link on List
   getTail( void ) const            // Get tail Link
{  return tail; }

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::fifo
//
// Purpose-
//       Insert a Link onto the List with FIFO ordering.
//
//----------------------------------------------------------------------------
void
   fifo(                            // Insert (FIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::insert
//
// Purpose-
//       Insert a chain of elements onto the list at the specified position.
//
//----------------------------------------------------------------------------
void
   insert(                          // Insert at position,
     Link*             link,        // -> Link to insert after
     Link*             head,        // -> First Link to insert
     Link*             tail);       // -> Final Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::isCoherent
//
// Purpose-
//       List coherency check.
//
//----------------------------------------------------------------------------
int                                 // TRUE if the object is coherent
   isCoherent( void ) const;        // Coherency check

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::isOnList
//
// Purpose-
//       Test whether Link is present in this List.
//
//----------------------------------------------------------------------------
int                                 // TRUE if Link is contained
   isOnList(                        // Is Link contained?
     Link*             link) const; // -> Link

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::lifo
//
// Purpose-
//       Insert a Link onto the List with LIFO ordering.
//
//----------------------------------------------------------------------------
void
   lifo(                            // Insert (LIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::remove
//
// Purpose-
//       Remove a chain of elements from the List.
//       This is an expensive operation for a DHSL_List.
//
//----------------------------------------------------------------------------
void
   remove(                          // Remove from DHSL_List
     Link*             head,        // -> First Link to remove
     Link*             tail);       // -> Final Link to remove

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::remq
//
// Purpose-
//       Remove the oldest Link from the list.
//
//----------------------------------------------------------------------------
Link*                               // -> Removed Link
   remq( void );                    // Remove oldest Link

//----------------------------------------------------------------------------
//
// Method-
//       DHSL_List::reset
//
// Purpose-
//       Remove ALL Links from the List.
//
//----------------------------------------------------------------------------
Link*                               // The set of removed Links
   reset( void );                   // Reset (empty) the List
}; // class DHSL_List<void>

//----------------------------------------------------------------------------
//
// Class-
//       DHSL_List<T>
//
// Purpose-
//       Typed DHSL_List object, where T is of class DHSL_List<T>::Link.
//
//----------------------------------------------------------------------------
template<class T>
class DHSL_List : public DHSL_List<void> {// DHSL_List<T>
public:
class Link : public DHSL_List<void>::Link { // DHSL_List<T>::Link
public:
inline T*                           // -> Next Link
   getNext( void ) const            // Get next Link
{  return static_cast<T*>(next); }
}; // class DHSL_List<T>::Link

public:
T*                                  // -> Newest T* on List
   getHead( void ) const            // Get head Link
{  return static_cast<T*>(head); }

T*                                  // -> Oldest T* on List
   getTail( void ) const            // Get tail Link
{  return static_cast<T*>(tail); }

T*                                  // Removed T*
   remq( void )                     // Remove oldest Link
{  return static_cast<T*>(DHSL_List<void>::remq()); }

T*                                  // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return static_cast<T*>(DHSL_List<void>::reset()); }
}; // class DHSL_List<T>

//----------------------------------------------------------------------------
//
// Class-
//       Link_List<T>
//
// Purpose-
//       Typed LinkedList object, where T is of class Link_List<T>::Link.
//
// Implementation notes-
//       This is a DHDL_List. See the associated notes.
//
//----------------------------------------------------------------------------
template<class T>
class Link_List : public DHDL_List<T> { // Link_List<T>, equivalent to DHDL_List<T>
}; // class Link_List<T>

//----------------------------------------------------------------------------
//
// Class-
//       SHSL_List
//
// Purpose-
//       The Singly Headed, Singly Linked List.
//
// Implemenation notes-
//       The SHDL_List is not thread safe. Method usage must be serialized.
//       The SHSL_List is optimized for LIFO operation. If you think of
//       this List as a Stack, LIFO == PUSH and REMQ == PULL.
//
//       The LIFO and REMQ methods run in constant time.
//       The FIFO, INSERT, and REMOVE methods run in linear time.
//
//----------------------------------------------------------------------------
template<> class SHSL_List<void> {  // SHSL_List base
public:
class Link {                        // SHSL_List<void>::Link
protected:
Link*                  next;        // -> Forward Link

public:
inline Link*                        // -> Next Link
   getNext( void ) const            // Get next Link
{  return next; }

inline void
   setNext(                         // Set next Link
     Link*             link)        // -> Next Link
{  next= link; }
}; // class SHSL_List<void>::Link

//----------------------------------------------------------------------------
// SHSL_List::Attributes
//----------------------------------------------------------------------------
protected:
Link*                  head;        // -> Head (Oldest) Link

//----------------------------------------------------------------------------
// SHSL_List::Constructors
//----------------------------------------------------------------------------
public:
   ~SHSL_List( void );              // Default destructor
   SHSL_List( void );               // Default constructor

private:                            // Bitwise copy prohibited
   SHSL_List(const SHSL_List&);
SHSL_List&
   operator=(const SHSL_List&);

//----------------------------------------------------------------------------
// SHSL_List::Accessors
//----------------------------------------------------------------------------
public:
inline Link*                        // -> Newest Link on List
   getHead( void ) const            // Get head Link
{  return head; }

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::isCoherent
//
// Purpose-
//       List coherency check.
//
//----------------------------------------------------------------------------
int                                 // TRUE if the object is coherent
   isCoherent( void ) const;        // Coherency check

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::isOnList
//
// Purpose-
//       Test whether Link is present in this List.
//
//----------------------------------------------------------------------------
int                                 // TRUE if Link is contained
   isOnList(                        // Is Link contained?
     Link*             link) const; // -> Link

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::fifo
//
// Purpose-
//       Insert a Link onto the List with FIFO ordering.
//
// Implementation notes-
//       This examines all existing Link elements and takes linear time.
//
//----------------------------------------------------------------------------
void
   fifo(                            // Insert (FIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::insert
//
// Purpose-
//       Insert a chain of elements onto the list at the specified position.
//
// Implementation notes-
//       This examines existing Link elements, taking linear time.
//
//----------------------------------------------------------------------------
void
   insert(                          // Insert at position,
     Link*             posLink,     // -> Link to insert after
     Link*             head,        // -> First Link to insert
     Link*             tail);       // -> Final Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::lifo
//
// Purpose-
//       Insert a Link onto the List with LIFO ordering.
//
//----------------------------------------------------------------------------
void
   lifo(                            // Insert (LIFO order)
     Link*             link);       // -> Link to insert

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::remove
//
// Purpose-
//       Remove a chain of elements from the List.
//
// Implementation notes-
//       This examines existing Link elements, taking linear time.
//
//----------------------------------------------------------------------------
void
   remove(                          // Remove from List
     Link*             head,        // -> First Link to remove
     Link*             tail);       // -> Final Link to remove

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::remq
//
// Purpose-
//       Remove the newest Link from the list.
//
//----------------------------------------------------------------------------

Link*                               // -> Removed Link
   remq( void );                    // Remove newest Link

//----------------------------------------------------------------------------
//
// Method-
//       SHSL_List::reset
//
// Purpose-
//       Remove ALL Links from the List.
//
//----------------------------------------------------------------------------
Link*                               // The set of removed Links
   reset( void );                   // Reset (empty) the List
}; // class SHSL_List<void>

//----------------------------------------------------------------------------
//
// Class-
//       SHSL_List<T>
//
// Purpose-
//       Typed SHSL_List object, where T is of class SHSL_List<T>::Link.
//
//----------------------------------------------------------------------------
template<class T>
class SHSL_List : public SHSL_List<void> { // SHSL_List<T>
public:
class Link : public SHSL_List<void>::Link { // SHSL_List<T>::Link
public:
inline T*                           // -> Next Link
   getNext( void ) const            // Get next Link
{  return static_cast<T*>(next); }
}; // class SHSL_List<T>::Link

public:
T*                                  // -> Oldest T* on List
   getHead( void ) const            // Get head link
{  return static_cast<T*>(head); }

T*                                  // Removed T*
   remq( void )                     // Remove oldest link
{  return static_cast<T*>(SHSL_List<void>::remq()); }

T*                                  // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return static_cast<T*>(SHSL_List<void>::reset()); }
}; // class DHSL_List<T>

//----------------------------------------------------------------------------
//
// Class-
//       Sort_List
//
// Purpose-
//       The Sort_List is a sortable DHDL_List.
//
// Implementation notes-
//       See DHDL_List for general implementation notes.
//
//       A Sort_List is in sorted order (from lowest to highest) only after
//       the sort method is invoked. If Links are added to the List, the
//       List remains potentially out of sort order until the sort method
//       is invoked (again.)
//
//       The DHDL_List base is private, making the Sort_List appear to be
//       a separate class. This prevents DHDL_List::Link objects from being
//       added to a Sort_List at compile time, which is useful.
//
//----------------------------------------------------------------------------
template<> class Sort_List<void> : private DHDL_List<void> { // Sort_List base
typedef DHDL_List<void>::Link DHDL_Link; // Internal shorthand
public:
class Link : private DHDL_Link {    // Sort_List<void>::Link
public:
virtual int                         // Result (<0, =0, >0)
   compare(                         // Compare to
     const Sort_List<void>::Link*
                       that) const = 0; // This Link

inline Link*                        // -> Next Link
   getNext( void ) const            // Get next Link
{  return static_cast<Link*>(next); }

inline Link*                        // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return static_cast<Link*>(prev); }

inline void
   setNext(                         // Set next Link
     Link*             link)        // -> Next Link
{  next= link; }

inline void
   setPrev(                         // Set prior Link
     Link*             link)        // -> Prior Link
{  prev= link; }
}; // class Sort_List<void>::Link

//----------------------------------------------------------------------------
// Sort_List::Constructors
//----------------------------------------------------------------------------
public:
inline
   ~Sort_List( void ) {}            // Default destructor
inline
   Sort_List( void )                // Default constructor
:  DHDL_List<void>() {}             // (Construct DHDL_List base)

//----------------------------------------------------------------------------
// Sort_List::Accessors
//----------------------------------------------------------------------------
public:
Link*                               // -> Oldest Link on List
   getHead( void ) const            // Get head link
{  return (Link*)head; }

Link*                               // -> Newest Link on List
   getTail( void ) const            // Get tail link
{  return (Link*)tail; }

//----------------------------------------------------------------------------
// Sort_List::Methods
//----------------------------------------------------------------------------
inline void
   fifo(                            // Insert (FIFO order)
     Link*             link)        // -> Link to insert
{  DHDL_List<void>::fifo((DHDL_Link*)link); }

inline void
   insert(                          // Insert at position,
     Link*             link,        // -> Link to insert after
     Link*             head,        // -> First Link to insert
     Link*             tail)        // -> Final Link to insert
{  DHDL_List<void>::insert((DHDL_Link*)link, (DHDL_Link*)head,
                           (DHDL_Link*)tail);
}

int                                 // TRUE if the object is coherent
   isCoherent( void ) const         // Coherency check
{  return DHDL_List<void>::isCoherent(); }

int                                 // TRUE if link is contained
   isOnList(                        // Is Link contained?
     Link*             link) const  // -> Link
{  return DHDL_List<void>::isOnList((DHDL_Link*)link); }

inline void
   lifo(                            // Insert (LIFO order)
     Link*             link)        // -> Link to insert
{  DHDL_List<void>::lifo((DHDL_Link*)link); }

inline void
   remove(                          // Remove from list
     Link*             head,        // -> First Link to remove
     Link*             tail)        // -> Final Link to remove
{  DHDL_List<void>::remove((DHDL_Link*)head, (DHDL_Link*)tail); }

inline Link*                        // -> Removed Link
   remq( void )                     // Remove oldest/lowest valued Link
{  return (Link*)DHDL_List<void>::remq(); }

inline Link*                        // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return (Link*)DHDL_List<void>::reset(); }

//----------------------------------------------------------------------------
//
// Method-
//       Sort_List::sort
//
// Purpose-
//       Sort the list. After this operation, the list is sorted.
//       Sort runs in polynomial time, currently implemented as a bubble sort.
//
//----------------------------------------------------------------------------
public:
void
   sort( void );                    // Sort the list
}; // class Sort_List

//----------------------------------------------------------------------------
//
// Class-
//       Sort_List<T>
//
// Purpose-
//       Typed Sort_List object, where T is of class Sort_List<T>::Link.
//
//----------------------------------------------------------------------------
template<class T>
class Sort_List : public Sort_List<void> { // Sort_List<T>
public:
class Link : public Sort_List<void>::Link { // Sort_List<T>::Link
public:
//----------------------------------------------------------------------------
// This method MUST BE supplied. There is no default implementation.
//
// Code format:
//     template<>
//     Sort_List<T>::Link::compare(const Sort_List<void>::Link* that) const
//     {
//       // In this implementation,
//       // use: static_cast<const T*>(this)->  (to refer to this object)
//       // and: static_cast<const T*>(that)->  (to refer to that object)
//
//       // Note: even though you can static_cast<const T*>(this), this method
//       // is not declared within or even considered part of class T itself.
//       // (It's actually a member of class T's base class, so you can
//       // override this method in class T. You'll still need to declare an
//       // implementation here.)
//       return 0; // Will suffice for the default in that case
//     }
//
// To override the base class implementation, in class T code:
//     virtual int compare(const Sort_List<void>::Link* that) const
//     { implementation; }
//----------------------------------------------------------------------------
virtual int                         // Result (<0, =0, >0)
   compare(                         // Compare to
     const Sort_List<void>::Link*
                       that) const; // This (class T) Link

inline T*                           // -> Next Link
   getNext( void ) const            // Get next Link
{  return static_cast<T*>(Sort_List<void>::Link::getNext()); } // (No direct access to next)

inline T*                           // -> Prior Link
   getPrev( void ) const            // Get prior Link
{  return static_cast<T*>(Sort_List<void>::Link::getPrev()); } // (No direct access to prev)
}; // class Sort_List<void>::Link

public:
T*                                  // -> Oldest T* on List
   getHead( void ) const            // Get head link
{  return static_cast<T*>(Sort_List<void>::getHead()); } // (No direct access to head)

T*                                  // -> Newest T* on List
   getTail( void ) const            // Get tail link
{  return static_cast<T*>(Sort_List<void>::getTail()); } // (No direct access to tail)

T*                                  // Removed T*
   remq( void )                     // Remove oldest/lowest valued link
{  return static_cast<T*>(Sort_List<void>::remq()); }

T*                                  // -> The set of removed Links
   reset( void )                    // Reset (empty) the List
{  return static_cast<T*>(Sort_List<void>::reset()); }
}; // class Sort_List<T>
}  // namespace _PUB_NAMESPACE
#endif // _PUB_LINKEDLIST_H_INCLUDED
