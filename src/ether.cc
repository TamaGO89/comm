// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK -----------------------------------------------------------------------------------------------------

/*!
 *  Copyright (CC) 2023, Andrea Tamantini (Tamago)
 *  \file ether.cpp
 *  \author Andrea Tamantini <tamandre89@gmail.com>
 *  \date 2021-05-18
 */

// -- END LICENSE BLOCK -------------------------------------------------------------------------------------------------------


/*=============================================================================================================================
 * HEADER
 *===========================================================================================================================*/
#include <comm/ether.h>

namespace comm {

    Ether::Ether ( const string& address, uint16_t port, const string& eol, Timeout timeout ) : Comm(address,eol,timeout) {
        this->port_ = port;
        this->sockaddr_in_.sin_family = AF_INET;
        this->open();
    }

    // Read common function
    size_t Ether::read_ (uint8_t *data, size_t size) {
        // If the connection is not open and connected, throw
        if ( ! ( this->is_open_ && this->is_connected_ ) ) throw new ConnectionException ("Ether::read : not connected");
        // Pre-fill buffer with available bytes
        ssize_t bytes_read_now = ::recv ( this->fd_, data, size, MSG_DONTWAIT );
        size_t bytes_read = bytes_read_now > 0 ? bytes_read_now : 0;
        // Prepare timeout value : now + read + byte*size
        TimeCheck timeout ( this->timeout_.read, this->timeout_.byte, size );
        // Read until the desired size is read, there's nothing left to read or timeout expires
        while ( bytes_read < size ) {
            // If the timeout expired or i read no data in the last cycle break the reading loop
            if ( timeout.expired() || bytes_read_now == 0 ) break;
            // Wait for the device to be readable, otherwise check again on the next loop
            if ( this->waitRead_() < 1 ) continue;
            // Read new available bytes
            bytes_read_now = ::recv (this->fd_, data + bytes_read, size - bytes_read, MSG_DONTWAIT );
            // retry if interrupted
            if ( bytes_read_now == -1 && errno == EINTR) continue;
            // At least 1 byte should always be read
            if ( bytes_read_now < 1 )
                throw new InterfaceException (
                        "Ether::read : device reports readiness to read but returned no data, disconnected?", errno);
            bytes_read += bytes_read_now;
        }
        return bytes_read;
    }

    // Send common function
    size_t Ether::send_ (const uint8_t *data, size_t size) {
        // If the connection is not open and connected, throw
        if ( ! ( this->is_open_ && this->is_connected_ ) ) throw new ConnectionException ("Ether::send : not connected");
        // Prepare return variables
        ssize_t bytes_sent_now = ::send ( this->fd_, data, size, 0 );
        size_t bytes_sent = bytes_sent_now > 0 ? bytes_sent_now : 0;
        // Prepare timeout value : now + send + byte*size
        TimeCheck timeout ( this->timeout_.send, this->timeout_.byte, size );
        // Send until the desired size is sent or timeout expires
        while ( bytes_sent < size ) {
            // If the timeout expired break the reading loop
            if ( timeout.expired() ) break;
            // Wait for the device to be ready to receive, otherwise check again on the next loop
            if ( this->waitSend_() < 1 ) continue;
            // Send more byets
            bytes_sent_now = ::send (fd_, data + bytes_sent, size - bytes_sent, MSG_MORE );
            // retry if interrupted
            if ( bytes_sent_now == -1 && errno == EINTR) continue;
            // at least 1 byte should always be sent
            if (bytes_sent_now < 1)
                throw new InterfaceException (
                        "Ether::send : device reports readiness to receive but returned no data, disconnected?", errno);
            bytes_sent += bytes_sent_now;
        }
        return bytes_sent;
    }

    // Open the file descriptor
    void Ether::open_ () {
        if ( this->is_open_ ) 
            return;
        if ( ( this->fd_ = socket(this->sockaddr_in_.sin_family, SOCK_STREAM, IPPROTO_TCP) ) < 0 ) {
            switch ( errno ) {
            case EINTR: // Recurse because this is a recoverable error.
                this->open_ ( ); return;
            case ENFILE: case EMFILE: // Other error numbers
                throw new IOException ( "Ether::open : Too many file handles open", errno );
            default:
                throw new IOException ( "Ether::open : general IO exception", errno );
            }
        }
        // IS OPEN
        this->is_open_ = true;
    }
    // Establish a connection to the server
    void Ether::connect_ () {
        if ( this->is_connected_ || this->address_.empty() || this->port_ == 0 )
            return;
        // Set socket address
        set_address ( &(this->address_), this->port_, &(this->sockaddr_in_) );
        // UNLOCK
        set_options ( this->fd_, get_options(this->fd_) | O_NONBLOCK );
        // Trying to connect
        if ( ::connect( this->fd_, (struct sockaddr *) & ( this->sockaddr_in_ ), sizeof ( this->sockaddr_in_ ) ) < 0 )
            switch ( errno ) {
                case EINPROGRESS: {
                    fd_set fd_set_;
                    FD_ZERO ( &fd_set_ );
                    FD_SET ( this->fd_, &fd_set_ );
                    int result = select ( this->fd_+1, NULL, &fd_set_, NULL, & ( this->timeout_.conn ) );
                    if ( result < 1 )
                        throw new InterfaceException ( "Ether::connect : connection error", errno );
                } break;
                default:
                    throw new InterfaceException ( "Ether::connect : generic error", errno );
            }
        // LOCK
        set_options ( this->fd_, get_options(this->fd_) & ~O_NONBLOCK );
        // Set socket timeout
        this->setOptions_ ( );
        // IS CONNECTED
        this->is_connected_ = true;
    }
    // Set socket
    void Ether::setOptions_() {
        if ( setsockopt ( this->fd_, SOL_SOCKET, SO_RCVTIMEO,
                        (const char *) & ( this->timeout_.read ),
                        sizeof ( this->timeout_.read ) ) != 0 )
            throw new InterfaceException ( "Ether::setOptions : set read timeout", errno );
        if ( setsockopt ( this->fd_, SOL_SOCKET, SO_SNDTIMEO,
                        (const char *) & ( this->timeout_.send ),
                        sizeof ( this->timeout_.send ) ) != 0 )
            throw new InterfaceException ( "Ether::setOptions : set send timeout", errno );
        socklen_t socket_length = sizeof(int);
        int result;
        if ( getsockopt ( this->fd_, SOL_SOCKET, SO_ERROR, (void*)(&result), &socket_length ) < 0 )
            throw new InterfaceException ( "Ether::setOptions : select socket", errno );
        if ( result )
            throw new InterfaceException ( "Ether::setOptions : select socket", result );
    }

}