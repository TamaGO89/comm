// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK -----------------------------------------------------------------------------------------------------

/*!
 *  Copyright (C) 2021, Autognity Srl, Via del Popolo 6, Orvieto (TR)
 *  \file serial.cc
 *  \author Andrea Tamantini <andrea.tamantini@autognity.com>
 *  \date 2021-05-18
 */

// -- END LICENSE BLOCK -------------------------------------------------------------------------------------------------------


/*=============================================================================================================================
 * HEADER
 *===========================================================================================================================*/
#include <comm/serial.h>

namespace comm {

    Serial::Serial ( const string &address, uint32_t baudrate, const string& eol, Timeout timeout, Settings settings ) :
            Comm(address,eol,timeout,settings) {
        this->baudrate_ = baudrate;
        this->open();
    }

    // Read common function
    size_t Serial::read_ (uint8_t *data, size_t size) {
        std::cout << "READ_ 1" << std::endl;
        // If the port is not open or not connected, throw
        if ( ! ( this->is_open_ && this->is_connected_ ) ) throw new ConnectionException ("Serial::read : not connected");
        // Pre-fill buffer with available bytes
        ssize_t bytes_read_now = ::read(fd_, data, size);
        size_t bytes_read = bytes_read_now > 0 ? bytes_read_now : 0;
        // Prepare timeout value : now + read + byte*size
        TimeCheck timeout ( this->timeout_.read, this->timeout_.byte, size );
        // Read until the desired size is read, there's nothing left to read or timeout expires
        while (bytes_read < size) {
            // If the timeout expired or i read no data in the last cycle break the reading loop
            if ( timeout.expired() || bytes_read_now == 0 ) {
                std::cout << "READ_ EXP" << std::endl;
                break;
            }
            // Wait for the device to be readable, otherwise check again on the next loop
            if ( this->waitRead_() < 1 ) continue;
            // Read new available bytes
            bytes_read_now = ::read (fd_, data + bytes_read, size - bytes_read);
            // retry if interrupted
            if ( bytes_read_now == -1 && errno == EINTR) continue;
            // At least 1 byte should always be read
            if ( bytes_read_now < 1 )
                throw new InterfaceException (
                        "Serial::read : device reports readiness to read but returned no data, disconnected?", errno);
            // Update bytes_read
            bytes_read += bytes_read_now;
        }
        std::cout << "READ_ 2" << std::endl;
        return bytes_read;
    }

    // Send common function
    size_t Serial::send_ (const uint8_t *data, size_t size) {
        std::cout << "SEND_ 1" << std::endl;
        // If the connection is not open and connected, throw
        if ( ! ( this->is_open_ && this->is_connected_ ) ) throw new ConnectionException ("Serial::send : not connected");
        // Prepare return variables
        fd_set writefds;
        size_t bytes_sent = 0, bytes_sent_now = 0;
        // Prepare timeout value : now + send + byte*size
        TimeCheck timeout ( this->timeout_.send, this->timeout_.byte, size );
        while (bytes_sent < size) {
            // If the timeout expired break the reading loop
            if ( timeout.expired() ) {
                std::cout << "SEND_ EXP" << std::endl;
                break;
            }
            // Wait for the device to be ready to receive, otherwise check again on the next loop
            if ( this->waitSend_() < 1 ) continue;
            // This will write some
            bytes_sent_now = ::write (fd_, data + bytes_sent, size - bytes_sent);
            // retry if interrupted
            if ( bytes_sent_now == -1 && errno == EINTR) continue;
            // at least 1 byte should always be sent
            if (bytes_sent_now < 1)
                throw new InterfaceException(
                        "Serial::send : device reports readiness to receive but returned data, disconnected?", errno);
            // Update bytes_sent
            bytes_sent += bytes_sent_now;
        }
        std::cout << "SEND_ 2" << std::endl;
        return bytes_sent;
    }

    void Serial::open_ ( ) {
        if ( ( this->is_open_ || this->address_.empty() ) )
            return;
        if ( ( this->fd_ = ::open(this->address_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK) ) < 0 ) {
            switch ( errno ) {
            case EINTR: // Recurse because this is a recoverable error.
                this->open_ ( ); return;
            case ENFILE: case EMFILE: // Other error numbers
                throw new IOException ( "Serial::open : Too many file handles open", errno );
            default:
                throw new IOException ( "Serial::open : general IO exception", errno );
            }
        }
        // IS OPEN
        this->is_open_ = true;
    }

    void Serial::connect_ ( ) {
        if ( this->is_connected_ )
            return;
        this->setOptions_();
        // IS CONNECTED
        this->is_connected_ = true;
    }

    void Serial::setOptions_ ( ) {
        if ( ::tcgetattr ( this->fd_, &this->termios_ ) == -1 )
            throw new IOException ( "Serial::setOptions : tcgetattr", errno );
        // INIT OPTION
        init_options ( &this->termios_ );
        // SET BAUDRATE
        speed_t baudrate_code = get_baudrate ( this->baudrate_ );
        ::cfsetispeed( &this->termios_, baudrate_code );
        ::cfsetospeed( &this->termios_, baudrate_code );
        // SET OPTION
        set_options ( &this->termios_, this->settings_ );
        // ACTIVATE OPTIONS
        ::tcsetattr ( this->fd_, TCSANOW, &this->termios_ );
        // SET BYTE TIMEOUT
        this->timeout_.byte = to_timeval( get_bytetime( this->baudrate_, this->settings_ ) );
    }

    // FLUSH : For the ether socket this does nothing
    void Serial::flush_ ( ) { if ( this->is_open_ && this->is_connected_ ) ::tcdrain ( this->fd_ ); }
    void Serial::flushInput_ ( ) { if ( this->is_open_ && this->is_connected_ ) ::tcflush ( this->fd_, TCIFLUSH ); }
    void Serial::flushOutput_ ( ) { if ( this->is_open_ && this->is_connected_ ) ::tcflush ( this->fd_, TCOFLUSH ); }

}