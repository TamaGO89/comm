// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK -----------------------------------------------------------------------------------------------------

/*!
 *  Copyright (C) 2021, Autognity Srl, Via del Popolo 6, Orvieto (TR)
 *  \file ether.cpp
 *  \author Andrea Tamantini <andrea.tamantini@autognity.com>
 *  \date 2021-05-18
 */

// -- END LICENSE BLOCK -------------------------------------------------------------------------------------------------------


/*=============================================================================================================================
 * HEADER
 *===========================================================================================================================*/
#include <comm/utils.h>

namespace comm {

    timeval to_timeval ( double data ) {
        int64_t sec64 = static_cast<int64_t>(std::floor(data));
        if (sec64 < 0 || sec64 > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("Time is out of dual 32-bit range");
        struct timeval time{
            static_cast<uint32_t>(sec64),
            static_cast<uint32_t>(std::round((data - sec64) * 1e6))
        };
        // avoid rounding errors
        time.tv_sec += (time.tv_usec / 1000000ul);
        time.tv_usec %= 1000000ul;
        return time;
    }

    double get_bytetime ( uint32_t baudrate, const Settings& settings ) {
        return ( 1.0 + settings.bytesize + settings.parity + (settings.stopbits==HALFONE?1.5:settings.stopbits) ) / baudrate;
    }

    speed_t get_baudrate (uint32_t baudrate) {
        switch (baudrate) {
# ifdef B0
            case 0: return B0;
# endif
# ifdef B50
            case 50: return B50;
# endif
# ifdef B75
            case 75: return B75;
# endif
# ifdef B110
            case 110: return B110;
# endif
# ifdef B134
            case 134: return B134;
# endif
# ifdef B150
            case 150: return B150;
# endif
# ifdef B200
            case 200: return B200;
# endif
# ifdef B300
            case 300: return B300;
# endif
# ifdef B600
            case 600: return B600;
# endif
# ifdef B1200
            case 1200: return B1200;
# endif
# ifdef B1800
            case 1800: return B1800;
# endif
# ifdef B2400
            case 2400: return B2400;
# endif
# ifdef B4800
            case 4800: return B4800;
# endif
# ifdef B7200
            case 7200: return B7200;
# endif
# ifdef B9600
            case 9600: return B9600;
# endif
# ifdef B14400
            case 14400: return B14400;
# endif
# ifdef B19200
            case 19200: return B19200;
# endif
# ifdef B28800
            case 28800: return B28800;
# endif
# ifdef B57600
            case 57600: return B57600;
# endif
# ifdef B76800
            case 76800: return B76800;
# endif
# ifdef B38400
            case 38400: return B38400;
# endif
# ifdef B115200
            case 115200: return B115200;
# endif
# ifdef B128000
            case 128000: return B128000;
# endif
# ifdef B153600
            case 153600: return B153600;
# endif
# ifdef B230400
            case 230400: return B230400;
# endif
# ifdef B256000
            case 256000: return B256000;
# endif
# ifdef B460800
            case 460800: return B460800;
# endif
# ifdef B500000
            case 500000: return B500000;
# endif
# ifdef B576000
            case 576000: return B576000;
# endif
# ifdef B921600
            case 921600: return B921600;
# endif
# ifdef B1000000
            case 1000000: return B1000000;
# endif
# ifdef B1152000
            case 1152000: return B1152000;
# endif
# ifdef B1500000
            case 1500000: return B1500000;
# endif
# ifdef B2000000
            case 2000000: return B2000000;
# endif
# ifdef B2500000
            case 2500000: return B2500000;
# endif
# ifdef B3000000
            case 3000000: return B3000000;
# endif
# ifdef B3500000
            case 3500000: return B3500000;
# endif
# ifdef B4000000
            case 4000000: return B4000000;
# endif
            default: return 0;
        }
    }

    void init_options ( termios * option ) {
        // INITIALIZE OPTIONS
        option->c_cflag |= (tcflag_t)  (CLOCAL | CREAD);
        option->c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
        option->c_oflag &= (tcflag_t) ~(OPOST);
        option->c_iflag &= (tcflag_t) ~(INLCR | IGNCR | ICRNL | IGNBRK);
#ifdef IUCLC
        option->c_iflag &= (tcflag_t) ~IUCLC;
#endif
#ifdef PARMRK
        option->c_iflag &= (tcflag_t) ~PARMRK;
#endif
    }

    void set_options ( termios * option, const Settings& settings ) {
        // SET CHAR LENGTH
        option->c_cflag &= (tcflag_t) ~CSIZE;
        switch ( settings.bytesize ) {
            case EIGHT: option->c_cflag |= CS8; break;
            case SEVEN: option->c_cflag |= CS7; break;
            case SIX: option->c_cflag |= CS6; break;
            case FIVE: option->c_cflag |= CS5; break;
            default: throw new std::invalid_argument ( "set_options : bytesize" );
        }
        // SET STOP BITS
        switch ( settings.stopbits ) {
            case ONE: option->c_cflag &= (tcflag_t) ~(CSTOPB); break;
            case HALFONE: option->c_cflag |=  (CSTOPB); break;
            case TWO: option->c_cflag |=  (CSTOPB); break;
            default: throw new std::invalid_argument ( "set_options : stopbits" );
        }
        // SET PARITY
        option->c_iflag &= (tcflag_t) ~(INPCK | ISTRIP);
        switch ( settings.parity ) {
            case NOPAR: option->c_cflag &= (tcflag_t) ~(PARENB | PARODD); break;
            case EVEN:
                option->c_cflag &= (tcflag_t) ~(PARODD); option->c_cflag |=  (PARENB); break;
            case ODD: option->c_cflag |=  (PARENB | PARODD); break;
#ifdef CMSPAR
            case MARK: option->c_cflag |=  (PARENB | CMSPAR | PARODD); break;
            case SPACE:
                option->c_cflag |=  (PARENB | CMSPAR); option->c_cflag &= (tcflag_t) ~(PARODD); break;
#endif
            default: throw new std::invalid_argument ( "set_options : parity" );
        }
        // SET FLOW CONTROL
        bool xonxoff=false, rtscts=false;
        switch ( settings.flowcontrol ) {
            case NOFLOW: xonxoff = false; rtscts = false; break;
            case SOFTWARE: xonxoff = true; rtscts = false; break;
            case HARDWARE: xonxoff = false; rtscts = true; break;
            default: throw new std::invalid_argument ( "set_options : flowcontrol" );
        }
        // XONXOFF
#ifdef IXANY
        if ( xonxoff ) option->c_iflag |=  (IXON | IXOFF);
        else option->c_iflag &= (tcflag_t) ~(IXON | IXOFF | IXANY);
#else
        if ( xonxoff ) option->c_iflag |=  (IXON | IXOFF);
        else option->c_iflag &= (tcflag_t) ~(IXON | IXOFF);
#endif
        // RTSCTS
#ifdef CRTSCTS
        if ( rtscts ) option->c_cflag |=  (CRTSCTS);
        else option->c_cflag &= (unsigned long) ~(CRTSCTS);
#else
        if ( rtscts ) option->c_cflag |=  (CNEW_RTSCTS);
        else option->c_cflag &= (unsigned long) ~(CNEW_RTSCTS);
#endif
        // http://www.unixwiz.net/techtips/termios-vmin-vtime.html
        // this basically sets the read call up to be a polling read,
        // but we are using select to ensure there is data available
        // to read before each call, so we should never needlessly poll
        option->c_cc[VMIN] = 0;
        option->c_cc[VTIME] = 0;
    }

    // Get file descriptor options
    int get_options ( int fd ) {
        int argument = fcntl(fd, F_GETFL, NULL);
        if ( argument < 0 )
            throw IOException ( "get file access mode", errno );
        return argument;
    }

    // Set file descriptor options
    void set_options ( int fd, int cmd ) {
        if ( fcntl(fd, F_SETFL, cmd ) < 0 )
            throw IOException ( "set", errno );
    }

    // Set socket address and port
    void set_address ( const string * address, uint32_t port, sockaddr_in * sockaddr ) {
        if ( address->empty() )
            throw invalid_argument ( "Empty ip address is invalid" );
        if ( port == 0 )
            throw invalid_argument ( "unspecified port is invalid" );
        // parse connection data into sock_addr_in struct
        sockaddr->sin_port = htons(port);
        if ( inet_pton(sockaddr->sin_family, address->c_str(), & ( sockaddr->sin_addr ) ) < 1 )
            throw InterfaceException ( "unable to parse ip address", errno );
    }

}