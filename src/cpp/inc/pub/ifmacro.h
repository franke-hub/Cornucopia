//----------------------------------------------------------------------------
//
//       Copyright (c) 2018 Frank Eskesen.
//
//       This file is free content, distributed under the Lesser GNU
//       General Public License, version 3.0.
//       (See accompanying file LICENSE.LGPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/lgpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       ifmacro.h
//
// Purpose-
//       IFMACRO defaults and macros.
//
// Last change date-
//       2018/01/01
//
// Controls-
//       HCDM     Hard Core Debug Mode (Intensive tracing)
//       IODM     Input/Output Debug mode (I/O tracing)
//       SCDM     Soft Core Debug Mode (Moderate tracing)
//       TRACE    Memory trace activation
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IFHCDM, IFIODM IFSCDM, and IFTRACE Macros
//----------------------------------------------------------------------------
#undef IFHCDM                       // Allow multiple inclusions
#undef ELHCDM
#undef IFIODM
#undef ELIODM
#undef IFSCDM
#undef ELSCDM
#undef IFTRACE

#ifdef HCDM
  #define IFHCDM(x) {x}
  #define ELHCDM(x) {}
#else
  #define IFHCDM(x) {}
  #define ELHCDM(x) {x}
#endif

#ifdef IODM
  #define IFIODM(x) {x}
  #define ELIODM(x) {}
#else
  #define IFIODM(x) {}
  #define ELIODM(x) {x}
#endif

#ifdef SCDM
  #define IFSCDM(x) {x}
  #define ELSCDM(x) {}
#else
  #define IFSCDM(x) {}
  #define ELSCDM(x) {x}
#endif

#ifdef TRACE
  #include <pub/Trace.h>
  #define IFTRACE(x) { if( pub::Trace::trace ) {x} }
#else
  #define IFTRACE(x) {}
#endif
