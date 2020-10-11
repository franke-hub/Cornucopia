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
//       Xcb/Active.cpp
//
// Purpose-
//       Implement Active.h
//
// Last change date-
//       2020/10/11
//
//----------------------------------------------------------------------------
#include <string.h>                 // For memcpy, memmove, strlen

#include <pub/Debug.h>              // For pub::Debug object
#include <pub/Must.h>               // For pub::Must methods
#include <pub/UTF8.h>               // For pub::UTF8 methods and objects

#include "Xcb/Global.h"             // For opt_hcdm, opt_verbose
#include "Xcb/Active.h"             // Implementation class

using namespace pub::debugging;     // For debugging subroutines
using namespace pub;                // For Must::

//----------------------------------------------------------------------------
// Constants for parameterization
//----------------------------------------------------------------------------
enum { BUFFER_SIZE= 2048 };         // Default buffer/expansion size (2**N)

namespace xcb {
//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::~Active
//
// Purpose-
//       Destructor.
//
//----------------------------------------------------------------------------
   Active::~Active( void )          // Destructor
{
   if( opt_hcdm )
     debugh("Active(%p)::~Active\n", this);

   Must::free(buffer);
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::Active
//
// Purpose-
//       Constructor.
//
//----------------------------------------------------------------------------
   Active::Active( void )           // Constructor
{
   if( opt_hcdm )
     debugh("Active(%p)::Active\n", this);

   source= (const unsigned char*)""; // Initial text
   buffer_size= BUFFER_SIZE;        // Default buffer size
   buffer_used= 0;
   buffer= (unsigned char*)Must::malloc(buffer_size);
   fsm= FSM_RESET;
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::debug
//
// Purpose-
//       Debugging display.
//
//----------------------------------------------------------------------------
void
   Active::debug(                   // Debugging display
     const char*       info) const  // Associated text
{
   debugf("Active(%p)::debug(%s) fsm(%d)\n", this, info ? info : "", fsm);
   debugf("..source(%s)\n", source);
   if( fsm != FSM_RESET ) {
     debugf("..buffer_size(%zd)\n", buffer_size);
     debugf("..buffer_used(%zd)\n", buffer_used);
     buffer[buffer_used]= '\0';     // (Buffer is mutable)
     debugf("..buffer(%s)\n", buffer);
   }
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::expand
//
// Purpose-
//       Expand the buffer to the appropriate character size. (Does not fetch)
//
//----------------------------------------------------------------------------
void
   Active::expand(                  // Expand the buffer
     Length            length)      // To this length (+1)
{
   if( opt_hcdm )
     debugh("Active(%p)::expand(%zd) [%zd,%zd]\n", this, length, buffer_used, buffer_size);

   if( length >= buffer_size ) {    // If expansion required
     size_t replace_size= length + BUFFER_SIZE + BUFFER_SIZE;
     replace_size &= ~(BUFFER_SIZE - 1);
     unsigned char* replace= (unsigned char*)Must::malloc(replace_size);
     if( fsm != FSM_RESET )
       memcpy(replace, buffer, buffer_used);
     Must::free(buffer);
     buffer= replace;
     buffer_size= replace_size;
   }
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::append_text
//
// Purpose-
//       Concatenate text substring.
//
//----------------------------------------------------------------------------
void
   Active::append_text(             // Concatenate text
     const char*       join,        // The join substring
     Length            size)        // The substring Length
{
   if( size == 0 )                  // If nothing to insert
     return;                        // (Line unchanged)

   fetch();
   fetch(buffer_used + size + 1);   // Insure room for concatenation
   memcpy(buffer + buffer_used, join, size); // Concatenate
   buffer_used += size;
   fsm= FSM_CHANGED;
}

void
   Active::append_text(             // Concatenate text
     const char*       join)        // The join string
{  append_text(join, strlen(join)); }

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::fetch
//
// Purpose-
//       Fetch the line and/or expand the buffer to the appropriate size.
//
//----------------------------------------------------------------------------
void
   Active::fetch(                   // Fetch the text
     Length            column)      // With blank fill to this length
{
   if( fsm == FSM_RESET )
     buffer_used= strlen((const char*)source);

   size_t buffer_need= buffer_used + 1;
   if( column >= buffer_need )
     buffer_need= column + 1;
   if( buffer_need >= buffer_size ) // If expansion required
     expand(buffer_need);

   if( fsm == FSM_RESET ) {         // If not fetched yet
     fsm= FSM_FETCHED;              // Fetch it now
     memcpy(buffer, source, buffer_used);
   }

   if( buffer_used < column ) {     // If expansion required
//   fsm= FSM_CHANGED;              // (Blank fill DOES NOT imply change)
     memset(buffer + buffer_used, ' ', column - buffer_used);
     buffer_used= column;
   }

   if( opt_hcdm )
     debugh("Active(%p)::fetch(%zd) [%zd/%zd]\n", this, column, buffer_used, buffer_size);
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::get_buffer
//
// Purpose-
//       (Unconditionally) access the buffer, leaving trailing blanks.
//
//----------------------------------------------------------------------------
const char*                         // The current buffer
   Active::get_buffer(              // Get '\0' delimited buffer
     Column            column) const // Starting at this Column
{
   if( fsm == FSM_RESET )
     return (const char*)source + pub::UTF8::index(source, column);

   buffer[buffer_used]= '\0';       // Set string delimiter (buffer mutable)
   return (const char*)buffer + pub::UTF8::index(buffer, column);
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::get_changed
//
// Purpose-
//       Access the buffer if changed. If unchanged, return nullptr.
//
//----------------------------------------------------------------------------
const char*                         // The changed text, nullptr if unchanged
   Active::get_changed( void ) const // Get changed text string
{
   if( fsm != FSM_CHANGED )         // If unchanged
     return nullptr;

   Length used_buffer= this->buffer_used; // (For const)
   while( used_buffer > 0 ) {       // Remove trailing blanks
     if( buffer[used_buffer - 1] != ' ' )
       break;

     used_buffer--;
   }

   buffer[used_buffer]= '\0';       // Set string delimiter (buffer mutable)
   return (const char*)buffer;
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::get_cols
//
// Purpose-
//       Return the buffer length (removing trailing blanks, if present)
//
//----------------------------------------------------------------------------
Active::Ccount                      // The current buffer Column count
   Active::get_cols( void )         // Get current buffer Column count
{
   fetch();                         // (Initialize buffer_used)
   while( buffer_used > 0 && buffer[buffer_used - 1] == ' ' )
     buffer_used--;
   buffer[buffer_used]= '\0';

   pub::UTF8::Decoder decoder(buffer);
   Ccount ccount= 0;                // Number of columns
   for(;;) {
     if( decoder.decode() <= 0 )
       return ccount;

     ccount++;
   }
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::get_used
//
// Purpose-
//       Return the buffer length (including trailing blanks, if present)
//
//----------------------------------------------------------------------------
Active::Length                      // The current buffer used length
   Active::get_used( void )         // Get current buffer used length
{  fetch(); return buffer_used; }

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::index
//
// Purpose-
//       Address character at column index, fetching if required.
//
//----------------------------------------------------------------------------
Active::Offset                      // The character Offset
   Active::index(                   // Get character Offset for
     Column            column)      // This Column
{
   fetch(column + 1);               // Load the buffer
   Offset offset= 0;
   while( column > 0 ) {
     if( buffer_used <= offset ) {  // If blank fill required
       fetch(buffer_used + column);
       offset= buffer_used - 1;
       break;
     }
     offset += pub::UTF8::index(buffer + offset, 1); // Next character length
     --column;
   }

   return offset;
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::insert_char
//
// Purpose-
//       Insert a (UTF32) character.
//
//----------------------------------------------------------------------------
void
   Active::insert_char(             // Insert character
     Column            column,      // The current column
     int               code)        // The insert character
{
   if( code <= 0 ) return;          // Ignore invalid codes

   char insert_buff[8];             // The insert character encoder buffer
   pub::UTF8::Encoder encoder(insert_buff, sizeof(insert_buff));
   encoder.encode(code);
   insert_buff[encoder.get_used()]= '\0';
   replace_text(column, 0, insert_buff);
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::insert_text
//
// Purpose-
//       Insert a text string.
//
//----------------------------------------------------------------------------
void
   Active::insert_text(             // Insert text
     Column            column,      // The insert column
     const char*       text)        // The insert text
{  replace_text(column, 0, text); }

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::remove_char
//
// Purpose-
//       Remove a character.
//
//----------------------------------------------------------------------------
void
   Active::remove_char(             // Remove the character
     Column            column)      // At this column
{  replace_text(column, 1, ""); }   // (Replace it with nothing)

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::replace_char
//
// Purpose-
//       Replace a character.
//
//----------------------------------------------------------------------------
void
   Active::replace_char(            // Replace the character
     Column            column,      // At this column
     int               code)        // With this character
{
   if( code <= 0 ) return;          // Ignore invalid codes

   char insert_buff[8];             // The replace character encoder buffer
   pub::UTF8::Encoder encoder(insert_buff, sizeof(insert_buff));
   encoder.encode(code);
   insert_buff[encoder.get_used()]= '\0';
   replace_text(column, 1, insert_buff);
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::replace_text
//
// Purpose-
//       Replace (or insert) a text string.
//
//----------------------------------------------------------------------------
void
   Active::replace_text(            // Replace (or insert) text
     Column            column,      // The replacement Column
     Ccount            ccount,      // The replacement (delete) Ccount
     const char*       text)        // The replacement (insert) text
{
   Offset offset= index(column);    // Initialize with blank fill
   Length insert= strlen(text);     // The inserted text length
   buffer[buffer_used]= '\0';
   Length remain_size= 0;           // Trailing text Length remaining in buffer
   Length remove= pub::UTF8::index(buffer + offset, ccount);
   if( buffer_used - offset > remove ) // If text will remain
     remain_size= buffer_used - offset - remove;
   expand(offset + insert + remain_size); // If necessary, expand the buffer
   if( remove && remain_size ) {    // If removing part of text
     memmove( buffer + offset + insert
            , buffer + offset + remove
            , remain_size);        // Preserve remaining text
     fsm= FSM_CHANGED;
   }

   if( insert ) {                   // If inserting text
     memmove(buffer + offset, text, insert); // Insert the text
     fsm= FSM_CHANGED;
   }

   buffer_used= offset + insert + remain_size;
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::reset
//
// Purpose-
//       Reset the Active source string.
//
//----------------------------------------------------------------------------
void
   Active::reset(                   // Set source
     const char*       text)        // To this (immutable) text
{
   source= (const unsigned char*)text;
   fsm= FSM_RESET;
}

//----------------------------------------------------------------------------
//
// Method-
//       xcb::Active::undo
//
// Purpose-
//       Undo any changes.
//
//----------------------------------------------------------------------------
int                                 // Return code: 0 if state changed
   Active::undo( void )             // Undo any changes
{
   int rc= 1;                       // Default, no change
   if( fsm != FSM_RESET ) {         // If something to undo
     fsm= FSM_RESET;
     rc= 0;
   }

   return rc;
}
}  // namespace xcb
