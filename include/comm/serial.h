/*!
 * \file comm/ether.h
 * \author Andrea Tamantini <andrea.tamantini@autognity.com>
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

#ifndef SERIAL_H
#define SERIAL_H

// SERIAL
#include <comm/comm.h>


namespace comm {


    using std::invalid_argument;
    using std::numeric_limits;
    using std::vector;
    using std::size_t;
    using std::string;

    /*!
    * Class that provides a portable serial port interface.
    */
    class Serial : public Comm {
    public:

        /*!
        * Creates a Serial object and opens the port if a port is specified,
        * otherwise it remains closed until comm::Serial::open is called.
        *
        * \param address A std::string containing the address of the serial port,
        *                which would be something like 'COM1' on Windows and '/dev/ttyS0'
        *                on Linux.
        *
        * \param baudrate An unsigned 32-bit integer that represents the baudrate
        *
        * \param eol End of line character or sequence of characters
        * 
        * \param timeout A comm::Timeout struct that defines the timeout
        * conditions for the serial port. \see comm::Timeout
        *
        * \param settings Contains settings for serial communication. \see comm::Settings
        *
        * \throw comm::InterfaceException
        * \throw comm::IOException
        * \throw std::invalid_argument
        */
        Serial ( const string &address="", uint32_t baudrate=9600, const string& eol="\n",
                 Timeout timeout=Timeout(), Settings settings=Settings() );
        // Destructor
        ~Serial ( ) = default;

    private:

        // Read common function
        size_t read_ (uint8_t *data, size_t size);
        // Send common function
        size_t send_ (const uint8_t *data, size_t size);
        // Open the file descriptor
        void open_ ( );
        // Ensure connection with the device
        void connect_ ( );
        // Set input output options
        void setOptions_ ( );
        // Flush input, output and both
        void flush_ ( );
        void flushInput_ ( );
        void flushOutput_ ( );

    };

} // namespace comm

#endif  // SERIAL_H
