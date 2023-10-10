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

#ifndef ETHER_H
#define ETHER_H

// COMM
#include <comm/comm.h>


namespace comm {


    using std::invalid_argument;
    using std::numeric_limits;
    using std::vector;
    using std::size_t;
    using std::string;

    /*!
    * Class that provides a portable network socket.
    */
    class Ether : public Comm {
    public:

        /*!
        * Creates a Ether object and opens the connection if address and port are specified,
        * otherwise it remains closed until comm::Ether::open is called.
        *
        * \param address A std::string containing the IPv4 address of the ether connection
        *
        * \param port An unsigned 16-bit integer that represents the port
        *
        * \param eol A char containing the end of line character for packets payloads (frames)
        *
        * \param timeout A comm::Timeout struct that defines the timeout
        * conditions for the ether connection. \see comm::Timeout
        *
        * \throw comm::ConnectionNotOpenedException
        * \throw comm::IOException
        * \throw std::invalid_argument
        */
        Ether ( const string& address=string(), uint16_t port=0, const string& eol="\r", Timeout timeout=Timeout() );
        // Destructor
        ~Ether ( ) = default;

    private:

        // Read common function
        size_t read_ (uint8_t *data, size_t size);
        // Send common function
        size_t send_ (const uint8_t *data, size_t size);
        // Open the file descriptor
        void open_ ();
        // Establish a connection to the server
        void connect_ ();
        // Set socket
        void setOptions_();

    };

} // namespace comm

#endif
