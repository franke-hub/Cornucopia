//----------------------------------------------------------------------------
//
//       Copyright (c) 2019-2020 Frank Eskesen.
//
//       This file is free content, distributed under the GNU General
//       Public License, version 3.0.
//       (See accompanying file LICENSE.GPL-3.0 or the original
//       contained within https://www.gnu.org/licenses/gpl-3.0.en.html)
//
//----------------------------------------------------------------------------
//
// Title-
//       SampleBIO.cpp
//
// Purpose-
//       Sample HTTP/HTTPS Client/Server, using openssl/bio.h functions.
//
// Last change date-
//       2020/10/04
//
// Known bugs-
//       std/ssl_stressor both eventually fail on Cygwin. (OK in Linux)
//       1000: BIO_free_all error when BIO_f_buffer used
//
//----------------------------------------------------------------------------
#include <mutex>                    // For std::lock_guard, ...
#include <sstream>                  // For std::stringstream
#include <getopt.h>                 // For getopt()
#include <string.h>                 // For memset, ...
#include <openssl/bio.h>            // For openssl BIO methods
#include <openssl/err.h>            // For openssl ERR methods
#include <openssl/ssl.h>            // For openssl SSL methods
#include <sys/time.h>               // For timeval

#include <pub/utility.h>            // For to_string, visify
#include <pub/Debug.h>              // For debugging
#include <pub/Interval.h>           // For Interval
#include <pub/Semaphore.h>          // For Semaphore
#include <pub/Thread.h>             // For Thread
#include <pub/Trace.h>              // For Trace
#include <pub/Worker.h>             // For Worker, WorkerPool

#include "pub/Socket.h"             // The Socket Object
#include "BIO_debug.h"              // For debugging routines

using namespace _PUB_NAMESPACE;     // For Socket, ...
using namespace _PUB_NAMESPACE::debugging; // For debugging
using _PUB_NAMESPACE::utility::visify; // For visify

//----------------------------------------------------------------------------
// Workarounds
//
// Latest CYGWIN version is missing TLS_*_method declarations.
// This kludge gets around the problem for now, and (at this point in time)
// the TLS_*_method and TLSv1_2_*_method are essentially identical.
//----------------------------------------------------------------------------
#ifndef SSL_EXT_TLS_ONLY
#define TLS_method        TLSv1_2_method
#define TLS_client_method TLSv1_2_client_method
#define TLS_server_method TLSv1_2_server_method
#endif

//----------------------------------------------------------------------------
// Constants for parameterization
//----------------------------------------------------------------------------
#define STD_PORT 8080               // Our standard port number
#define SSL_PORT 8443               // Our SSL port number

#define USE_RUNTIME 60              // Delay before terminating
#define USE_CLIENT true             // Include Clients?
#define USE_THREAD true             // Use ClientThread (else direct call)
#define USE_SERVER true             // Include Servers?
#define USE_WORKER true             // Use WorkerThread (else direct call)

//----------------------------------------------------------------------------
// Constants for parameterization
//----------------------------------------------------------------------------
#ifndef HCDM
#undef  HCDM                        // If defined, Hard Core Debug Mode
#endif

#ifndef TRACE                       // ** NOT IMPLEMENTED **
#undef  TRACE                       // If defined, Tracing active
#endif

#include <pub/ifmacro.h>

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#define LOCK_GUARD(x) std::lock_guard<decltype(x)> lock(x)

//----------------------------------------------------------------------------
// Internal data areas
//----------------------------------------------------------------------------
static int             testfail;    // Indicates test failure
static Interval        interval;    // Stress interval timer
static Semaphore       semaphore;   // Thread complete semaphore

//----------------------------------------------------------------------------
// HTTP responses
//----------------------------------------------------------------------------
static const char*     http400=     // HTTP 400 (INVALID) response
   "HTTP/1.1 400 !INVALID!\r\n"
   "Content-type: text/html\r\n"
   "Content-length: 58\r\n"
   "\r\n"
   "<html>\r\n"                     // 8
   "<body>\r\n"                     // 8
   "<h1>400 !INVALID!</h1>\r\n"     // 24
   "</body>\r\n"                    // 8
   "</html>\r\n"                    // 8
   ;

static const char*     http404=     // HTTP 404 (NOT FOUND) response
   "HTTP/1.1 404 Not Found\r\n"
   "Content-type: text/html\r\n"
   "Content-length: 58\r\n"
   "\r\n"
   "<html>\r\n"                     // 8
   "<body>\r\n"                     // 8
   "<h1>404 NOT FOUND</h1>\r\n"     // 24
   "</body>\r\n"                    // 8
   "</html>\r\n"                    // 8
   ;

static const char*     response=    // The response data
   "HTTP/1.1 200 OK\r\n"
   "Server: RYO\r\n"
   "Content-type: text/html\r\n"
   "Content-length: 58\r\n"
   "\r\n"
   "<html>\r\n"                     // 8
   "<body>\r\n"                     // 8
   "<h1>Hello, World!</h1>\r\n"     // 24
   "</body>\r\n"                    // 8
   "</html>\r\n"                    // 8
   ;

//----------------------------------------------------------------------------
// Options
//----------------------------------------------------------------------------
static const char*     opt_debug= nullptr; // --debug{=filename}
static int             opt_help= false; // --help or error
static int             opt_index;   // Option index
static int             opt_runtime= USE_RUNTIME; // Server run time
static int             opt_client=  USE_CLIENT; // Use client?
static int             opt_server=  USE_SERVER; // Use server?
static int             opt_stress=  false;      // Use stress test?
static int             opt_thread=  USE_THREAD; // Use client thread?
static int             opt_worker=  USE_WORKER; // Use worker?
static int             opt_verbose= 0; // --verbose{=verbosity}
static int             bug_1000= false; // BIO_free_all fails with BIO_f_buffer

static struct option   OPTS[]=      // Options
{  {"help",     no_argument,       &opt_help,    true}
,  {"debug",    optional_argument, nullptr,      0}
,  {"runtime",  required_argument, nullptr,      0}
,  {"verbose",  optional_argument, nullptr,      0}

,  {"bug_1000",    no_argument,    &bug_1000,    true} // Bug# 1000
,  {"client",      no_argument,    &opt_client,  true} // use_client
,  {"server",      no_argument,    &opt_server,  true} // use_server
,  {"stress",      no_argument,    &opt_stress,  true} // Stress test?
,  {"thread",      no_argument,    &opt_thread,  true} // client thread
,  {"worker",      no_argument,    &opt_worker,  true} // server thread
,  {"no-client",   no_argument,    &opt_client,  false} // use_client
,  {"no-server",   no_argument,    &opt_server,  false} // use_server
,  {"no-thread",   no_argument,    &opt_thread,  false} // client thread
,  {"no-worker",   no_argument,    &opt_worker,  false} // server thread
,  {0, 0, 0, 0}                     // (End of option list)
};

enum OPT_INDEX
{  OPT_HELP
,  OPT_DEBUG
,  OPT_RUNTIME
,  OPT_VERBOSE
};

//----------------------------------------------------------------------------
//
// Subroutine-
//       CTX_error
//
// Purpose-
//       Handle CTX creation error
//
//----------------------------------------------------------------------------
static void
   CTX_error(                       // Handle CTX creation error
     const char*       fmt)         // Format string (with one %s)
{
   char buffer[256];                // Working buffer
   long E= ERR_get_error();         // Get last error code
   ERR_error_string(E, buffer);
   std::string S= pub::utility::to_string(fmt, buffer);
   throw SocketException(S);
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       ctx_password_cb
//
// Purpose-
//       Our pem_password_cb
//
//----------------------------------------------------------------------------
static int                          // Actual password length
   ctx_password_cb(                 // Our pem password callback
    char*              buff,        // Return buffer address (for password)
    int                size,        // Return buffer length  (for password)
    int                rwflag,      // FALSE:decryption, TRUE:encryption
    void*              userdata)    // User data
{
   if( false )                      // Note: only usage of userdata
     debugf("%4d HCDM(%p,%d,%d,%p)\n", __LINE__, buff, size, rwflag, userdata);

   if( rwflag ) {                   // If encryption
     debugf("%4d HCDM SHOULD NOT OCCUR\n", __LINE__);
     return -1;                     // (NOT SUPPORTED)
   }

   const char* result= "xxyyz";     // Our (not so secret) password
   int L= strlen(result);           // Resultant length
   if( L > size ) L= size;          // (Cannot exceed maximum)

   memcpy(buff, result, L);         // Set the resultant
   return L;
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       ctx_password_cb_init
//
// Purpose-
//       Initialize pem_password_cb
//
//----------------------------------------------------------------------------
static void
   ctx_password_cb_init(            // Initialize pem password callback
     SSL_CTX*          context)     // For this context
{
   SSL_CTX_set_default_passwd_cb(context, ctx_password_cb);
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       handle_ERR
//
// Purpose-
//       Display all pending OPENSSL errors
//
//----------------------------------------------------------------------------
static void
   handle_ERR( void )               // Display all pending BIO errors
{
   char buffer[256];                // Error buffer string

   long ec= ERR_get_error();        // The first error
   while( ec ) {                    // Display all errors
     ERR_error_string(ec, buffer);  // Format this error
     errorf("%s\n", buffer);

     ec= ERR_get_error();
   }
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       initialize_SSL
//
// Purpose-
//       Initialize SSL
//
//----------------------------------------------------------------------------
static void
   initialize_SSL( void )           // Initialize SSL
{
static std::mutex      mutex;       // Latch protecting initialized
static int             initialized= false; // TRUE when initialized

   std::lock_guard<decltype(mutex)> lock(mutex);

   if( !initialized ) {
     SSL_library_init();
     SSL_load_error_strings();
     ERR_load_BIO_strings();
     OpenSSL_add_all_algorithms();

     initialized= true;
   }
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       new_client_CTX
//
// Purpose-
//       Create a client SSL_CTX
//
//----------------------------------------------------------------------------
static SSL_CTX*
   new_client_CTX( void )           // Create a client SSL_CTX
{
   const SSL_METHOD* method= TLS_client_method();
   SSL_CTX* context= SSL_CTX_new(method);
   if( context == nullptr )
     CTX_error("SSL_CTX_new: %s");

   SSL_CTX_set_mode(context, SSL_MODE_AUTO_RETRY);
   ctx_password_cb_init(context);

   return context;
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       new_server_CTX
//
// Purpose-
//       Create a server SSL_CTX
//
//----------------------------------------------------------------------------
static SSL_CTX*
   new_server_CTX(                  // Create a client SSL_CTX
     const char*       pub_file,    // The public cert file name
     const char*       key_file)    // The private key file name
{
   const SSL_METHOD* method= TLS_server_method();
   SSL_CTX* context= SSL_CTX_new(method);
   if( context == nullptr )
     CTX_error("SSL_CTX_new: %s");
   ctx_password_cb_init(context);

   if( SSL_CTX_use_certificate_file(context, pub_file, SSL_FILETYPE_PEM) <= 0 ) {
     debugf("new_serverCTX(%s,%s) invalid public file\n", pub_file, key_file);
     CTX_error("use_certificate file: %s");
   }

   if( SSL_CTX_use_PrivateKey_file(context, key_file, SSL_FILETYPE_PEM) <= 0 ) {
     debugf("new_serverCTX(%s,%s) invalid key file\n", pub_file, key_file);
     CTX_error("use_PrivateKey file: %s");
   }

   if( !SSL_CTX_check_private_key(context) )
   {
     debugf("new_server_CTX(%s,%s) key mismatch\n", pub_file, key_file);
     CTX_error("Public/private key mismatch: %s");
   }

   SSL_CTX_set_mode(context, SSL_MODE_AUTO_RETRY);

   return context;
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       reconnect
//
// Purpose-
//       Attempt to create a connection, ignoring any result.
//
//----------------------------------------------------------------------------
static void
   reconnect(                       // Attempt a dummy connection
     int               port)        // To this port
{
   try {
     Socket socket;

     socket.open(AF_INET, SOCK_STREAM, 0);
     sockaddr_in sa;
     memset(&sa, 0, sizeof(sa));
     sa.sin_family= AF_INET;
     sa.sin_port= htons(port);
     sa.sin_addr.s_addr= htonl(0x7f000001);
     socket.connect((sockaddr*)&sa, sizeof(sa));
     Thread::sleep(0.125);
   } catch(...) {
   }
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       torf
//
// Purpose-
//       Return "true" or "false"
//
//----------------------------------------------------------------------------
static const char*                  // "true" or "false"
   torf(                            // True or False?
     int               cc)          // For this condition
{
   if( cc )
     return "true";
   return "false";
}

//----------------------------------------------------------------------------
//
// Class-
//       WorkerThread
//
// Purpose-
//       The Worker Thread
//
//----------------------------------------------------------------------------
class WorkerThread : public Worker { // The worker Thread
//----------------------------------------------------------------------------
// WorkerThread::Attributes
//----------------------------------------------------------------------------
public:
BIO*                   bio;         // The connection BIO
char                   buffer[32768]; // Input buffer;

//----------------------------------------------------------------------------
// WorkerThread::Constructors/Destructor
//----------------------------------------------------------------------------
public:
virtual
   ~WorkerThread( void )            // Destructor
{  IFHCDM( debugh("WorkerThread(%p)::~WorkerThread()...\n", this); )

   if( bio ) {
     if( bug_1000 ) {
       debugh("%4d HCDM\n", __LINE__);
       debug_chain(bio, "~WorkerThread");
     }
     try {
       BIO_free_all(bio);
     } catch(...) {
       debugf("Exception...\n");
     }
     if( bug_1000 ) debugh("%4d HCDM\n", __LINE__);
   }

   IFHCDM(debugh("WorkerThread(%p)::...~WorkerThread()\n", this); )
}

   WorkerThread(                    // Constructor
     BIO*              bio)         // Connection BIO
:  bio(bio)
{  IFHCDM(debugh("WorkerThread(%p)::WorkerThread(%p)\n", this, bio); ) }

//----------------------------------------------------------------------------
// WorkerThread::Methods
//----------------------------------------------------------------------------
public:
std::string                         // The next token, "" if at end
   get_token(                       // Get next token
     const char*&       text)
{
   while( *text == ' ' )
     text++;

   if( *text == '\0' )
     return "";

   if( *text == '\r' && *(text+1) == '\n' ) {
     text += 2;
     return "\r\n";
   }

   const char* origin= text;
   while( *text != ' ' && *text != '\t' && *text != '\r' )
     text++;

   std::string result(origin, text - origin);
   return result;
}

virtual void
   work( void )                     // Process work
{
   if( opt_verbose )
     debugh("WorkerThread::work()\n");

   try {
     worker();
   } catch(pub::Exception& X) {
     debugging::debugh("Worker: %s\n", X.to_string().c_str());
   } catch(std::exception& X) {
     debugging::debugh("Worker: what(%s)\n", X.what());
   } catch(...) {
     debugging::debugh("Worker: ...\n");
   }

   delete this;                     // When done, delete this Worker
}

virtual void
   worker( void )                   // Process work
{
   // The default timeout
   if( opt_stress == false ) {
     struct timeval tv= { 3, 0 };   // 3.0 second timeout
     ::setsockopt(BIO_get_fd(bio, nullptr), SOL_SOCKET, SO_RCVTIMEO,
                  &tv, sizeof(tv));
   }

   for(int count= 0;;count++) {
     buffer[0]= '\0';
     int L= BIO_read(bio, buffer, sizeof(buffer));
     if( L > 0 )
       buffer[L]= '\0';

     if( opt_verbose > 1 )
       debugh("Worker: Read %d '%s'\n", L, visify(buffer).c_str());
     if( L <= 0 ) {
       if( bug_1000 ) {
         /** Problem: BIO_free_all, Chrome, BIO_f_buffer interaction?
         *** Problem: This bypass DOES NOT WORK
         ***   When this happens, freeing the BIO becomes iffy.
         ***   BIO_free_all can abort without any error message.
         ***   (Platform specific. Occurs in Linux but not Cygwin.)
         ***   There are no Google references to such a problem, so it's
         ***   probably due to some error in this code.
         ***   Until the root cause for the problem is found,
         ***   it's bypassed by skipping the BIO_free_all.
         **/
         if( L == 0 && count == 0 ) {
           debugh("Worker: %4d HCDM\n", __LINE__); // Bug workaround
           bio= nullptr;
         }
       }
       break;
     }

     const char* C= buffer;
     std::string method= get_token(C);
     if( method != "GET" ) {
       L= BIO_write(bio, http400, strlen(http400));
       if( opt_verbose > 1 )
         debugh("Worker: Sent %d '%s'\n", L, visify(http400).c_str());
     } else {

     std::string what= get_token(C);
     if( what != "/" && what != "/index.html"
         && what != "/std" && what != "/ssl" ) {
       L= BIO_write(bio, http404, strlen(http404));
       if( opt_verbose > 1 )
         debugh("Worker: Sent %d '%s'\n", L, visify(http404).c_str());
     } else {

     std::string http= get_token(C);
     if( http != "HTTP/1.1" ) {
       L= BIO_write(bio, http400, strlen(http400));
       if( opt_verbose > 1 )
         debugh("Worker: Sent %d '%s'\n", L, visify(http400).c_str());
     } else {

     L= BIO_write(bio, response, strlen(response));
     if( opt_verbose > 1 )
       debugh("Worker: Sent %d '%s'\n", L, visify(response).c_str());
     } } }

     BIO_flush(bio);
   }
}
}; // class WorkerThread

//----------------------------------------------------------------------------
//
// Class-
//       STD_ClientThread
//
// Purpose-
//       The Client Thread
//
//----------------------------------------------------------------------------
class STD_ClientThread : public Thread { // The client Thread
//----------------------------------------------------------------------------
// STD_ClientThread::Attributes
//----------------------------------------------------------------------------
public:
BIO*                   bio;         // Our client BIO
char                   buffer[8192]; // Response data

//----------------------------------------------------------------------------
// STD_ClientThread::Constructors
//----------------------------------------------------------------------------
public:
   ~STD_ClientThread( void )        // Destructor
{  if( bio ) BIO_free_all(bio); }

   STD_ClientThread( void )         // Constructor
:  Thread(), bio(nullptr)
{
   // Connect to server
   sprintf(buffer, "localhost:%d", STD_PORT);
   bio= BIO_new_connect(buffer);
   if( bio == nullptr )
     throw Exception("STD_Client: cannot create BIO");
}

//----------------------------------------------------------------------------
// STD_ClientThread::Methods
//----------------------------------------------------------------------------
public:
virtual void
   run( void )                      // Operate this Thread
{
   if( opt_verbose > 1 )
     debugh("STD_ClientThread::run()\n");

try {
   if( BIO_do_connect(bio) <= 0 )
     throw Exception("STD_Client: connect failure");

   // Write/read
static const char*     request=     // The request data
   "GET /std HTTP/1.1\r\n"
   ;

   int L= BIO_write(bio, request, strlen(request));
   if( opt_verbose > 1 )
     debugh("STD Client: Wrote %d '%s'\n", L, visify(request).c_str());

   L= BIO_read(bio, buffer, sizeof(buffer)-1);
   if( L > 0 ) {
     buffer[L]= '\0';
     if( opt_verbose > 1 )
       debugh("STD Client: Read %d '%s'\n", L, visify(buffer).c_str());
   }

} catch(...) {
   traceh("STD_Client failure\n");
   testfail= true;
}
   // Close the connection
   BIO_free_all(bio);
   bio= nullptr;

   // Post the semaphore
   if( opt_stress )
     semaphore.post();
}
}; // class STD_ClientThread

//----------------------------------------------------------------------------
//
// Class-
//       SSL_ClientThread
//
// Purpose-
//       The Client Thread
//
//----------------------------------------------------------------------------
class SSL_ClientThread : public Thread { // The client Thread
//----------------------------------------------------------------------------
// SSL_ClientThread::Attributes
//----------------------------------------------------------------------------
public:
BIO*                   bio;         // Our client BIO
char                   buffer[8192]; // Response data
SSL_CTX*               context;     // SSL_CTX

//----------------------------------------------------------------------------
// SSL_ClientThread::Constructors
//----------------------------------------------------------------------------
public:
   ~SSL_ClientThread( void )        // Destructor
{  if( bio ) BIO_free_all(bio); }

   SSL_ClientThread(                // Constructor
     SSL_CTX*          context)     // SSL_CTX
:  Thread(), bio(nullptr), context(context)
{  bio= BIO_new_ssl_connect(context); }

//----------------------------------------------------------------------------
// SSL_ClientThread::Methods
//----------------------------------------------------------------------------
public:
virtual void
   run( void )                      // Operate this Thread
{
   if( opt_verbose > 1 )
     debugh("SSL_ClientThread::run()\n");

   // Connect to server
   SSL*                ssl;         // Working SSL

try {
   BIO_get_ssl(bio, &ssl);
   if( ssl == nullptr ) {
     handle_ERR();
     throw Exception("SSL_Client: Cannot locate SSL");
   }

   sprintf(buffer, "localhost:%d", SSL_PORT);
   if( BIO_set_conn_hostname(bio, buffer) <= 0 )
     throw Exception("SSL_Client: cannot set hostname");

   if( BIO_do_connect(bio) <= 0 )
     throw Exception("SSL_Client: connect failure");

   // Write/Read
static const char*     request=     // The request data
   "GET /ssl HTTP/1.1\r\n"
   ;

   int L= BIO_write(bio, request, strlen(request));
   if( opt_verbose > 1 )
     debugh("SSL Client: Wrote %d '%s'\n", L, visify(request).c_str());

   L= BIO_read(bio, buffer, sizeof(buffer)-1);
   if( L > 0 ) {
     buffer[L]= '\0';
     if( opt_verbose > 1 )
       debugh("SSL Client: Read %d '%s'\n", L, visify(buffer).c_str());
   }

} catch(...) {
   traceh("STD_Client failure\n");
   testfail= true;
}
   // Close the connection
   BIO_free_all(bio);
   bio= nullptr;

   // Post the semaphore
   if( opt_stress )
     semaphore.post();
}
}; // class SSL_ClientThread

//----------------------------------------------------------------------------
//
// Class-
//       STD_ServerThread
//
// Purpose-
//       The Server Thread
//
//----------------------------------------------------------------------------
class STD_ServerThread : public Thread { // The server Thread
//----------------------------------------------------------------------------
// STD_ServerThread::Attributes
//----------------------------------------------------------------------------
public:
std::mutex             mutex;       // Object lock
Semaphore              sem;         // Startup Semaphore
BIO*                   bio;         // Our listener BIO

bool                   operational; // TRUE while operational
int                    port;        // Listener port

//----------------------------------------------------------------------------
// STD_ServerThread::Constructors
//----------------------------------------------------------------------------
public:
   ~STD_ServerThread( void )        // Destructor
{  if( bio ) BIO_free_all(bio); }

   STD_ServerThread(                // Constructor
     int               port)        // Listener port
:  Thread()
,  sem(), bio(nullptr), operational(true), port(port)
{  }

//----------------------------------------------------------------------------
// STD_ServerThread::Methods
//----------------------------------------------------------------------------
public:
virtual void
   run( void )                      // Operate this Thread
{
   debugh("STD_ServerThread::run() port(%d)\n", port );

   // socket.bind equivalent
   char buffer[16];
   sprintf(buffer, "%d", port);

   bio= BIO_new_accept(buffer);
   if( bio == nullptr )
     throw Exception("STD_Server: cannot create BIO");

   if( BIO_set_bind_mode(bio, BIO_BIND_REUSEADDR) <= 0 ) {
     handle_ERR();
     throw Exception("STD_Server: bind mode failure");
   }

   if( BIO_do_accept(bio) <= 0 ) {
     handle_ERR();
     throw Exception("STD_Server: bind failure");
   }

   sem.post();                      // Indicate started / bound

   try {
     while( operational ) {
       int rc= BIO_do_accept(bio);

       std::lock_guard<decltype(mutex)> lock(mutex);
       if( ! operational )
         break;

       if( rc <= 0 ) {
         LOCK_GUARD(*Debug::get());
         errorf("Error accepting STD connection\n");
         handle_ERR();
       } else {
         BIO* client= BIO_pop(bio);
         WorkerThread* worker= new WorkerThread(client);
         if( opt_worker )
           WorkerPool::work(worker);
         else
           worker->work();
       }
     }
   } catch(pub::Exception& X) {
     debugh("Exception: %s\n", X.to_string().c_str());
   } catch(std::exception& X) {
     debugh("std::Exception what(%s)\n", X.what());
   }
}

void
   stop( void )                     // Terminate this Thread
{
   {{{{
     std::lock_guard<decltype(mutex)> lock(mutex);

     operational= false;            // Indicate terminated
   }}}}

   //-------------------------------------------------------------------------
   // We may need to attempt a dummy connection complete the listen.
   // (We ignore any and all errors that might occur doing this.)
   reconnect(STD_PORT);
}
}; // class std_serverThread

//----------------------------------------------------------------------------
//
// Class-
//       SSL_ServerThread
//
// Purpose-
//       The Server Thread
//
//----------------------------------------------------------------------------
class SSL_ServerThread : public Thread { // The server Thread
//----------------------------------------------------------------------------
// SSL_ServerThread::Attributes
//----------------------------------------------------------------------------
public:
std::mutex             mutex;       // Object lock
SSL_CTX*               context;     // SSL_CTX
Semaphore              sem;         // Startup Semaphore

bool                   operational; // TRUE while operational
int                    port;        // Listener port

//----------------------------------------------------------------------------
// SSL_ServerThread::Constructors
//----------------------------------------------------------------------------
public:
   ~SSL_ServerThread( void )        // Destructor
{  }

   SSL_ServerThread(                // Constructor
     SSL_CTX*          context,     // SSL_CTX
     int               port)        // Listener port
:  Thread(), context(context)
,  sem(), operational(true), port(port)
{  }

//----------------------------------------------------------------------------
// SSL_ServerThread::Methods
//----------------------------------------------------------------------------
public:
virtual void
   run( void )                      // Operate this Thread
{
   debugh("SSL_ServerThread::run() port(%d)\n", port );

   // Set up SSL BIO
   BIO* ssl_bio= BIO_new_ssl(context, 0);
   if( ssl_bio == nullptr )
     throw Exception("SSL_Server: cannot create BIO");

   SSL* ssl; BIO_get_ssl(ssl_bio, &ssl);
   if( ssl == nullptr )
     throw Exception("SSL_Server: cannot locate SSL");

   // Add buffer BIO
   if( bug_1000 ) {
     BIO* buf_bio= BIO_new(BIO_f_buffer());
//// ssl_bio= BIO_push(ssl_bio, buf_bio);
     ssl_bio= BIO_push(buf_bio, ssl_bio);
   }

   // Create / add accept BIO
   char buffer[16];
   sprintf(buffer, "%d", port);
   BIO* acc_bio= BIO_new_accept(buffer);
   if( acc_bio == nullptr )
     throw Exception("SSL_Server: cannot create accept BIO");

   if( BIO_set_bind_mode(acc_bio, BIO_BIND_REUSEADDR) <= 0 ) {
     handle_ERR();
     throw Exception("STD_Server: bind mode failure");
   }

   // socket.bind equivalent
   BIO_set_accept_bios(acc_bio, ssl_bio);
   if( BIO_do_accept(acc_bio) <= 0 ) {
     handle_ERR();
     throw Exception("SSL_Server: bind failure");
   }

   if( bug_1000 ) {
     debug_chain(ssl_bio, "SSL_Server: ssl_bio");
     debug_chain(acc_bio, "SSL_Server: acc_bio");
   }

   sem.post();                      // Indicate started / bound

   try {
     while( operational ) {
       int rc= BIO_do_accept(acc_bio);

       std::lock_guard<decltype(mutex)> lock(mutex);
       if( ! operational )
         break;

       if( rc <= 0 ) {
         LOCK_GUARD(*Debug::get());
         errorf("Error accepting SSL connection:\n");
         handle_ERR();
       } else {
         if( bug_1000 )
           debug_chain(acc_bio,  "SSL_Server: before");
         BIO* client= BIO_pop(acc_bio);
         if( bug_1000 ) {
           debug_chain(acc_bio,  "SSL_Server: after ");
           debug_chain(client, "SSL_Server: client");
         }
         if( BIO_do_handshake(client) <= 0 ) {
           LOCK_GUARD(*Debug::get());
           errorf("SSL Handshake error:\n");
           handle_ERR();
           BIO_free_all(client);
         } else {
           WorkerThread* worker= new WorkerThread(client);
           if( opt_worker )
             WorkerPool::work(worker);
           else
             worker->work();
         }
       }
     }
   } catch(pub::Exception& X) {
     debugh("Exception: %s\n", X.to_string().c_str());
   } catch(std::exception& X) {
     debugh("std::Exception what(%s)\n", X.what());
   }

// BIO_free_all(acc_bio);
   BIO_free_all(ssl_bio);
}

void
   stop( void )                     // Terminate this Thread
{
   {{{{
     std::lock_guard<decltype(mutex)> lock(mutex);

     operational= false;              // Indicate terminated
   }}}}

   //-------------------------------------------------------------------------
   // We may need to attempt a dummy connection complete the listen.
   // (We ignore any and all errors that might occur doing this.)
   reconnect(SSL_PORT);
}
}; // class SSL_ServerThread

//----------------------------------------------------------------------------
//
// Subroutine-
//       dirty
//
// Purpose-
//       Quick and dirty test.
//
//----------------------------------------------------------------------------
static inline void
   dirty( void )                    // Quick and dirty test
{
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       ssl_stressor
//
// Purpose-
//       Run SSL stress test
//
//----------------------------------------------------------------------------
static void
   ssl_stressor(                    // Run SSL stress test
     SSL_CTX*          context)     // Our client context
{
static const int THREAD_COUNT= 64;  // Maximum thread count
   testfail= false;                 // No failure yet

   SSL_ClientThread* thread[THREAD_COUNT];
   for(int i= 0; i<THREAD_COUNT; i++) {
     thread[i]= new SSL_ClientThread(context);
     thread[i]->start();
   }

   long op_count= 0;                // Number of completed threads
   interval.start();
   while( interval.stop() < double(opt_runtime) && testfail == false ) {
     semaphore.wait();
     for(int i= 0; i<THREAD_COUNT; i++) {
       if( thread[i]->bio == nullptr ) {
         op_count++;
         thread[i]->join();
         delete thread[i];

         thread[i]= new SSL_ClientThread(context);
         thread[i]->start();
       }
     }
   }

   // Complete all threads
   double runtime= interval.stop();
   for(;;) {
     int running= 0;
     for(int i= 0; i<THREAD_COUNT; i++) {
       if( thread[i] ) {
         running++;
         if( thread[i]->bio == nullptr ) {
           thread[i]->join();
           delete thread[i];
           thread[i]= nullptr;
         }
       }
     }

     if( running == 0 )
       break;

     semaphore.wait(0.5);
   }

   // Statistics
   debugf("SSL Client: %s\n", testfail ? "FAILED" : "Complete");
   debugf("%8ld Operations\n", op_count);
   debugf("%12.3f Seconds\n", runtime);
   debugf("%12.3f Operations/second\n", double(op_count) / runtime);
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       std_stressor
//
// Purpose-
//       Run standard stress test
//
//----------------------------------------------------------------------------
static void
   std_stressor( void )             // Run standard stress test
{
static const int THREAD_COUNT= 16;  // Maximum thread count
   testfail= false;                 // No failure yet

   STD_ClientThread* thread[THREAD_COUNT];
   for(int i= 0; i<THREAD_COUNT; i++) {
     thread[i]= new STD_ClientThread();
     thread[i]->start();
   }

   long op_count= 0;                // Number of completed threads
   interval.start();
   while( interval.stop() < double(opt_runtime) && testfail == false ) {
     semaphore.wait();
     for(int i= 0; i<THREAD_COUNT; i++) {
       if( thread[i]->bio == nullptr ) {
         op_count++;
         thread[i]->join();
         delete thread[i];

         thread[i]= new STD_ClientThread();
         thread[i]->start();
       }
     }
   }

   // Complete all threads
   double runtime= interval.stop();
   for(;;) {
     int running= 0;
     for(int i= 0; i<THREAD_COUNT; i++) {
       if( thread[i] ) {
         running++;
         if( thread[i]->bio == nullptr ) {
           thread[i]->join();
           delete thread[i];
           thread[i]= nullptr;
         }
       }
     }

     if( running == 0 )
       break;

     semaphore.wait(0.5);
   }

   // Statistics
   debugf("STD Client: %s\n", testfail ? "FAILED" : "Complete");
   debugf("%8ld Operations\n", op_count);
   debugf("%12.3f Seconds\n", runtime);
   debugf("%12.3f Operations/second\n", double(op_count) / runtime);
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       info
//
// Purpose-
//       Parameter description.
//
//----------------------------------------------------------------------------
static void
   info( void)                      // Parameter description
{
   fprintf(stderr, "SampleBIO [options]\n"
                   "Options:\n"
                   "  --{no-}bug_1000\n"
                   "  --{no-}client\n"
                   "  --{no-}server\n"
                   "  --{no-}thread\n"
                   "  --{no-}worker\n"
                   "  --debug{=file_name}\n"
                   "  --runtime=value\n"
                   "  --verbose{=value}\n"
          );

   exit(EXIT_FAILURE);
}

//----------------------------------------------------------------------------
//
// Subroutine-
//       parm
//
// Purpose-
//       Parameter analysis.
//
//----------------------------------------------------------------------------
static void
   parm(                            // Parameter analysis
     int               argc,        // Argument count
     char*             argv[])      // Argument array
{
   int                 C;

   //-------------------------------------------------------------------------
   // Argument analysis
   //-------------------------------------------------------------------------
   opterr= 0;                       // Do not write error messages

   while( (C= getopt_long(argc, (char**)argv, ":", OPTS, &opt_index)) != -1 )
     switch( C )
     {
       case 0:
         switch( opt_index )
         {
           case OPT_DEBUG:
             opt_debug= optarg;
             if( opt_verbose == 0 )
               opt_verbose= 1;
             break;

           case OPT_RUNTIME:
             opt_runtime= atoi(optarg);
             break;

           case OPT_VERBOSE:
             opt_verbose= 2;         // Default "extra" verbosity
             if( optarg )
               opt_verbose= atoi(optarg);
             break;

           default:
             break;
         }
         break;

       case ':':
         opt_help= true;
         if( optopt == 0 )
           fprintf(stderr, "Option requires an argument '%s'.\n",
                           argv[optind-1]);
         else
           fprintf(stderr, "Option requires an argument '-%c'.\n", optopt);
         break;

       case '?':
         opt_help= true;
         if( optopt == 0 )
           fprintf(stderr, "Unknown option '%s'.\n", argv[optind-1]);
         else if( isprint(optopt) )
           fprintf(stderr, "Unknown option '-%c'.\n", optopt);
         else
           fprintf(stderr, "Unknown option character '0x%x'.\n", optopt);
         break;

       default:
         fprintf(stderr, "%4d SNO ('%c',0x%x).\n", __LINE__, C, C);
         exit( EXIT_FAILURE );
     }

   if( opt_help )
     info();
}

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
   main(                            // Mainline code
     int               argc,        // Argument count
     char*             argv[])      // Argument array
{
   unsigned int        errorCount= 0;

   // Parse parameters
   parm(argc, argv);

   // Set debugging object
   Debug debug(opt_debug);
   debug.set(&debug);
   debug.set_head(Debug::HEAD_THREAD);
   IFHCDM( debug.set_mode(Debug::MODE_INTENSIVE); )
   ELHCDM( if(opt_runtime > 0) debug.set_mode(Debug::MODE_INTENSIVE); )
   debug.debugh("SampleBIO started...\n");

   debugf("\n");
   debugf("Settings:\n");
   debugf("%5s: bug_1000\n", torf(bug_1000));
   debugf("%5d: runtime\n",  opt_runtime);
   debugf("%5s: stress\n",   torf(opt_stress));
   debugf("%5s: client\n",   torf(opt_client));
   debugf("%5s: thread\n",   torf(opt_thread));
   debugf("%5s: server\n",   torf(opt_server));
   debugf("%5s: worker\n",   torf(opt_worker));
   debugf("%5d: verbose\n",  opt_verbose);
   debugf("\n");

   try {
     initialize_SSL();              // Initialize SSL

     SSL_CTX* client_CTX= new_client_CTX();
     SSL_CTX* server_CTX= new_server_CTX("public.crt", "private.key");

     STD_ClientThread std_client;   // Our STD_ClientThread
     SSL_ClientThread ssl_client(client_CTX); // Our SSL_ClientThread
     STD_ServerThread std_server(STD_PORT); // Our STD_ServerThread
     SSL_ServerThread ssl_server(server_CTX, SSL_PORT); // Our SSL_ServerThread

     if( opt_server ) {
       std_server.start();
       ssl_server.start();

       std_server.sem.wait();       // Wait until started
       ssl_server.sem.wait();       // Wait until started
     }

     if( opt_stress ) {             // Run stress test?
       std_stressor();              // Run STD stress test
       ssl_stressor(client_CTX);    // Run SSL stress test
     } else {
       if( opt_client ) {
         if( opt_thread ) {
           std_client.start();      // Start our STD_ClientThread
           ssl_client.start();      // Start our SSL_ClientThread

           std_client.join();       // (Wait for completion)
           ssl_client.join();       // (Wait for completion)
         }
       } else {
         std_client.run();          // (Run in main thread)
         ssl_client.run();          // (Run in main thread)
       }

       if( opt_server && opt_runtime > 0 ) {
         Thread::sleep(opt_runtime);
       }
     }

     if( opt_server ) {
       std_server.stop();
       ssl_server.stop();
       std_server.join();
       ssl_server.join();
     }

     debugf("%4d HCDM\n", __LINE__);
     Thread::sleep(0.5);            // Completion delay
     WorkerPool::debug();

     SSL_CTX_free(client_CTX);
     SSL_CTX_free(server_CTX);
   } catch(pub::Exception& X) {
     errorCount++;
     debugf("pub::Exception: %s\n", X.to_string().c_str());
   } catch(std::exception& X) {
     errorCount++;
     debugf("std::Exception what(%s)\n", X.what());
   } catch(...) {
     debugf("catch(...)\n");
   }

   debug.debugf("...SampleBIO complete(%d)\n", errorCount);
   Debug::set(nullptr);

   return ( errorCount != 0 );
}
