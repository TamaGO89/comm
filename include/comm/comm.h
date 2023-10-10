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

#ifndef COMM_H
#define COMM_H

// COMM
#include <comm/utils.h>


namespace comm {


    using std::invalid_argument;
    using std::numeric_limits;
    using std::vector;
    using std::size_t;
    using std::string;

    /*!
    * Class that provides a portable communication interface.
    */
    class Comm {
    public:
        /*!
        * Creates a Comm object and opens the port if a port is specified,
        * otherwise it remains closed until comm::Comm::open is called.
        *
        * \param address A std::string containing the address of the comm port,
        *                which would be something like 'COM1' on Windows and '/dev/ttyS0'
        *                on Linux.
        *
        * \param eol End of line character or sequence of characters
        * 
        * \param timeout A comm::Timeout struct that defines the timeout
        * conditions for the comm port. \see comm::Timeout
        *
        * \param settings Contains settings for comm communication. \see comm::Settings
        *
        * \throw comm::InterfaceException
        * \throw comm::IOException
        * \throw std::invalid_argument
        */
        Comm ( const string &address="", const string& eol="\n", Timeout timeout=Timeout(), Settings settings=Settings() );

        /*! Destructor */
        ~Comm ();

        /*!
        * Opens the comm port as long as the port is set and the port isn't
        * already open.
        *
        * If the port is provided to the constructor then an explicit call to open
        * is not needed.
        *
        * \see Comm::Comm
        *
        * \throw std::invalid_argument
        * \throw comm::InterfaceException
        * \throw comm::IOException
        */
        void open ();
        /*! Closes the comm port. */
        void close ();
        /*! Flush the input and output buffers */
        void flush ();
        void flushInput ();
        void flushOutput ();


        /*! Gets the open status of the comm port.
        *
        * \return Returns true if the port is open, false otherwise.
        */
        bool isOpen () const;
        bool isConnected () const;

        /*! Block until there is comm data to read or read_constant
        * number of milliseconds have elapsed. The return value is true when
        * the function exits with the port in a readable state, false otherwise
        * (due to timeout or select interruption). */
        bool waitRead ();
        bool waitSend ();

        /*=====================================================================================================================
         * READ, READLINE and READLINES : Public methods to read a fixed number of characters, a line or an array of lines
         *=====================================================================================================================
         * READ : Read a fixed size of char and parse them into a char array, char vector or string, returns size or string
         *-------------------------------------------------------------------------------------------------------------------*/
        // READ (char*,size) -> size : Threadsafely read a fixed size of char in a char array
        size_t read (uint8_t *buffer, size_t size);
        // READ (vector<char>,size) -> size : Threadsafely read a fixed size of char in a char vector
        size_t read (vector<uint8_t> &buffer, size_t size);
        // READ (string,size) -> size : Threadsafely read a fixed size of char in a string
        size_t read (string &buffer, size_t size);
        // READ (size) -> string : Threadsafely read a fixed size of char in a string
        string read (size_t size);
        /*---------------------------------------------------------------------------------------------------------------------
         * READLINE : Read a fixed size of char and parse them into a char array, char vector or string, returns size or string
         *-------------------------------------------------------------------------------------------------------------------*/
        // READLINE (string,size) -> size : Read a line (until eol or size is reached) into string, return the string size
        size_t readline (string& buffer, size_t size);
        // READLINE (size) -> string : Read a line (until eol or size is reached) into a string, return the string
        string readline ( size_t size );
        /*---------------------------------------------------------------------------------------------------------------------
         * READLINES : Read multiple lines at once, emplace them in a vector of strings
         *-------------------------------------------------------------------------------------------------------------------*/
        // READLINES (size) -> vector<string> : Read a vector of strings (to eol) until a fixed size is reached, return the string
        vector<string> readlines ( size_t size );


        /*=====================================================================================================================
         * SEND : Public methods to send a frame in string, char array or char vector form
         *=====================================================================================================================
         * SEND : Send a string, char* or vector<char>. return the sent size
         *-------------------------------------------------------------------------------------------------------------------*/
        // SEND (string) -> size : Send a string, returns the number of sent char
        size_t send (const string& data);
        // SEND (vector<char>) -> size : Send a char vector, returns the number of sent char
        size_t send (const std::vector<uint8_t> &data);
        // SEND (char*,size) -> size : Send a char array, returns the number of sent char
        size_t send (const uint8_t *data, size_t size);

        /*=====================================================================================================================
         * GETTERS AND SETTERS : Public methods to set and get Comm parameters
         *===================================================================================================================*/
        // SET ADDRESS
        void setAddress (const std::string &address);
        // GET ADDRESS
        const std::string& getAddress () const;
        //---------------------------------------------------------------------------------------------------------------------
        // SET PORT
        void setPort ( uint16_t port );
        // GET PORT
        uint16_t getPort () const;
        //---------------------------------------------------------------------------------------------------------------------
        // SET BAUDRATE
        void setBaudrate ( uint32_t baudrate );
        // GET BAUDRATE
        uint32_t getBaudrate () const;
        //---------------------------------------------------------------------------------------------------------------------
        // SET EOL : Set end of the line char for payloads (frames) to be read
        void setEOL ( const string& eol );
        // GET EOL : Get end of the line char for payloads (frames) to be read
        const string& getEOL ( ) const;
        //---------------------------------------------------------------------------------------------------------------------
        // SET TIMEOUT : Set timeout for read and send operations and connection (passing a timeout struct)
        void setTimeout (const Timeout& timeout);
        // SET TIMEOUT : Set timeout for read and send operations and connection (specified read, send and connection timeouts)
        void setTimeout ( double read, double send, double byte, double conn );
        // GET TIMEOUT : Get timeout for read and send operations and connection (passing a timeout struct)
        const Timeout& getTimeout ( ) const;
        //---------------------------------------------------------------------------------------------------------------------
        // SET SETTINGS
        void setSettings ( const Settings& settings );
        void setSettings ( ByteSize bytesize=EIGHT,Parity parity=NOPAR,StopBits stopbits=ONE,FlowControl flowcontrol=NOFLOW );
        // GET SETTINGS
        const Settings& getSettings ( ) const;

    protected:
        // Disable copy constructors
        Comm(const Comm&);
        Comm& operator=(const Comm&);

        /*=====================================================================================================================
         * VIRTUAL : Virtual private methods to be extended
         *===================================================================================================================*/
        // Read common function ( VIRTUAL )
        virtual size_t read_ (uint8_t *data, size_t size);
        // Send common function ( VIRTUAL )
        virtual size_t send_ (const uint8_t *data, size_t size);
        // Open file descriptor
        virtual void open_ ( );
        // Close connection and file descriptor ( COMMON )
        virtual void close_ ( );
        // Connect ( SERIAL )
        virtual void connect_ ( );
        // Set Options ( SERIAL )
        virtual void setOptions_ ( );
        // Wait read/send
        int waitRead_ ( );
        int waitSend_ ( );

        // FLUSH : For the ether socket this does nothing ( SERIAL )
        virtual void flush_ ( );
        virtual void flushInput_ ( );
        virtual void flushOutput_ ( );

        /*---------------------------------------------------------------------------------------------------------------------
         * Protected instance variables
         *-------------------------------------------------------------------------------------------------------------------*/
        // address, point at the resources to communicate with, needs additional informations like baudrate or port
        string address_;
        uint32_t baudrate_;  // baudrate, used by serial communication, specify the communication speed
        uint16_t port_;  // port, used by ether communication, specify the tcp/udp port to use
        // end of line, this character indicates the last characters of a sequence
        string eol_;
        size_t eol_len_;  // end of line length, the length of the end of line sequence of characters
        // is open / is connected, indicates wether or not the resource is open and or connected
        bool is_open_ = false;
        bool is_connected_ = false;
        // timeout, contains informations about the timeout for reading and sending operation, for single byte and connection
        Timeout timeout_;
        // settings, used by serial communication, describe the low level protocol: bytesize, stopbits, parity, flowcontrol
        Settings settings_;
        // file descriptor, a pointer that describe the virtual file used to interact with the resource
        int fd_;
        // termios, used by serial communication, contains the options used by the file descriptor to connect
        struct termios termios_;
        // socket addres, used by ether communication, contains the options used by the file descriptor to connect
        struct sockaddr_in sockaddr_in_;
        // mutex, read and send mutex to allow multithreading operations
        boost::mutex mtx_read, mtx_send;
    };

} // namespace comm

#endif  // COMM_H
