/*!
 * \file comm/ether.h
 * \author Andrea Tamantini <tamandre89@gmail.com>
 * \version 0.1
 *
 * \section LICENSE
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * \section DESCRIPTION
 *
 * This provides a cross platform interface for interacting with Ether Ports.
 */

#ifndef SERIAL_UTILS_H
#define SERIAL_UTILS_H

// STD
#include <limits>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <alloca.h>
// SYS
#include <sysexits.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
// BOOST
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
// THREAD
#include <pthread.h>
// IO
#include <fcntl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
// ETH
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>


namespace comm {

    using std::invalid_argument;
    using std::numeric_limits;
    using std::vector;
    using std::size_t;
    using std::string;

    /*=========================================================================================================================
     * 
     */
    // Enumeration defines the possible bytesizes for the serial port.
    typedef enum { FIVE = 5, SIX = 6, SEVEN = 7, EIGHT = 8 } ByteSize;
    // Enumeration defines the possible parity types for the serial port.
    typedef enum { NOPAR = 0, ODD = 1, EVEN = 2, MARK = 3, SPACE = 4 } Parity;
    // Enumeration defines the possible stopbit types for the serial port.
    typedef enum { ONE = 1, TWO = 2, HALFONE = 3 } StopBits;
    // Enumeration defines the possible flowcontrol types for the serial port.
    typedef enum { NOFLOW = 0, SOFTWARE = 1, HARDWARE = 2 } FlowControl;

    timeval to_timeval ( double data );

    template<typename ... Args> string format( const char* format, Args ... args ) {
        int size_int = snprintf( nullptr, 0, format, args ... );  // Extra space for '\0'
        if( size_int <= 0 ) throw std::runtime_error( "format exception" );
        size_t size = size_int + 1;
        std::unique_ptr<char[]> buffer( new char[ size ] );
        snprintf( buffer.get(), size, format, args ... );
        return string( buffer.get(), buffer.get() + size - 1 );  // minus the '\0'
    };

    struct Timeout {

        struct timeval read, send, byte, conn;

        explicit Timeout ( double read=0, double send=0, double byte=0, double conn=0 ) :
                read(to_timeval(read)), send(to_timeval(send)), byte(to_timeval(byte)), conn(to_timeval(conn)) { }
        static Timeout simpleTimeout(double timeout) { return Timeout(timeout, timeout, timeout); }
    };

    struct TimeCheck {

        struct timespec now, timeout;

        explicit TimeCheck ( timeval timeout, timeval byte, size_t size ) { clock_gettime( CLOCK_MONOTONIC, & ( this->now ) );
            this->timeout.tv_sec = timeout.tv_sec + byte.tv_sec * size * 2 + this->now.tv_sec;
            this->timeout.tv_nsec = ( timeout.tv_usec + byte.tv_usec * size * 2 ) * 1000 + this->now.tv_nsec; }
        bool expired ( ) { clock_gettime( CLOCK_MONOTONIC, & ( this->now ) );
            return ( this->timeout.tv_sec < this->now.tv_sec || this->timeout.tv_nsec < this->now.tv_nsec ); }
    };

    /*!
     * Contains a description for serial communication
     *
     * \param bytesize Size of each byte in the serial transmission of data, default is EIGHT,
     * possible values are: FIVE, SIX, SEVEN, EIGHT
     *
     * \param parity Method of parity, default is NONE,
     * possible values are: NONE, ODD, EVEN
     *
     * \param stopbits Number of stop bits used, default is ONE,
     * possible values are: ONE, HALF_ONE, TWO
     *
     * \param flowcontrol Type of flowcontrol used, default is NONE,
     * possible values are: NONE, SOFTWARE, HARDWARE
     */
    struct Settings {

        ByteSize bytesize;
        Parity parity;
        StopBits stopbits;
        FlowControl flowcontrol;

        explicit Settings ( ByteSize bytesize=EIGHT, Parity parity=NOPAR,
                            StopBits stopbits=ONE, FlowControl flowcontrol=NOFLOW ) :
                bytesize(bytesize), parity(parity), stopbits(stopbits), flowcontrol(flowcontrol) { 
        }
    };

    /*=========================================================================================================================
     * CUSTOM EXCEPTIONS : Interface, IO and Connection, custom exceptions for Communication interface
     *=======================================================================================================================*/
    class InterfaceException : public std::exception {
        // Disable copy constructors
        InterfaceException& operator=(const InterfaceException&);
        std::string e_what_;
    public:
        InterfaceException ( const string& description_ ) { this->e_what_ = description_; }
        InterfaceException ( const string& description_, int errno_ ) {
            this->e_what_ = format ( "%d : %s : %s", errno_, description_, strerror(errno_) );
        }
        InterfaceException (const InterfaceException& other) : e_what_(other.e_what_) {}
        virtual ~InterfaceException() throw() {}
        virtual const char* what () const throw () {
            return e_what_.c_str();
        }
    };

    class IOException : public std::exception {
        // Disable copy constructors
        IOException& operator=(const IOException&);
        std::string e_what_;
    public:
        IOException ( const string& description_ ) { this->e_what_ = description_; }
        IOException ( const string& description_, int errno_ ) {
            this->e_what_ = format ( "%d : %s : %s", errno_, description_, strerror(errno_) );
        }
        IOException (const IOException& other) : e_what_(other.e_what_) {}
        virtual ~IOException() throw() {}
        virtual const char* what () const throw () {
            return e_what_.c_str();
        }
    };

    class ConnectionException : public std::exception {
        // Disable copy constructors
        ConnectionException& operator=(const ConnectionException&);
        std::string e_what_;
    public:
        ConnectionException ( const string& description_ ) { this->e_what_ = description_; }
        ConnectionException ( const string& description_, int errno_ ) {
            this->e_what_ = format ( "%d : %s : %s", errno_, description_, strerror(errno_) );
        }
        ConnectionException (const ConnectionException& other) : e_what_(other.e_what_) {}
        virtual ~ConnectionException() throw() {}
        virtual const char* what () const throw () {
            return e_what_.c_str();
        }
    };

    double get_bytetime ( uint32_t baudrate, const Settings& settings );

    speed_t get_baudrate (uint32_t baudrate);

    void init_options ( termios * option );

    void set_options ( termios * option, const Settings& settings );

    // Get file descriptor options
    int get_options ( int fd );

    // Set file descriptor options
    void set_options ( int fd, int cmd );

    // Set socket address and port
    void set_address ( const string * address, uint32_t port, sockaddr_in * sockaddr );

} // namespace comm

#endif  // SERIAL_UTILS_H
