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
#include <comm/comm.h>

namespace comm {

    /*! Constructor */
    Comm::Comm ( const string &address, const string& eol, Timeout timeout, Settings settings ) :
            address_(address), eol_(eol), eol_len_(eol.length()), timeout_(timeout), settings_(settings) { }
    /*! Destructor */
    Comm::~Comm () {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->close_();
    }

    /*! Opens the comm port. */
    void Comm::open () {
        //std::cout << "OPEN 1" << std::endl;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->open_();
        this->connect_();
        //std::cout << "OPEN 2" << std::endl;
    }
    /*! Closes the comm port. */
    void Comm::close () {
        //std::cout << "CLOSE 1" << std::endl;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->close_();
        //std::cout << "CLOSE 2" << std::endl;
    }

    /*! Flush the input and output buffers */
    void Comm::flush () {
        //std::cout << "FLUSH 1" << std::endl;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->flush_();
        //std::cout << "FLUSH 2" << std::endl;
    }
    void Comm::flushInput () {
        //std::cout << "FLUSH IN 1" << std::endl;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->flushInput_();
        //std::cout << "FLUSH IN 2" << std::endl;
    }
    void Comm::flushOutput () {
        //std::cout << "FLUSH OUT 1" << std::endl;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->flushOutput_();
        //std::cout << "FLUSH OUT 2" << std::endl;
    }

    /*! Gets the open status of the comm port.
    *
    * \return Returns true if the port is open, false otherwise.
    */
    bool Comm::isOpen () const { return this->is_open_; }
    bool Comm::isConnected () const { return this->is_connected_; }

    /*! Block until there is comm data to read or read_constant
    * number of milliseconds have elapsed. The return value is true when
    * the function exits with the port in a readable state, false otherwise
    * (due to timeout or select interruption). */
    bool Comm::waitRead () {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        return this->waitRead_() > 0;
    }
    bool Comm::waitSend () {
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        return this->waitSend_() > 0;
    }

    /*=====================================================================================================================
     * READ, READLINE and READLINES : Public methods to read a fixed number of characters, a line or an array of lines
     *=====================================================================================================================
     * READ : Read a fixed size of char and parse them into a char array, char vector or string, returns size or string
     *-------------------------------------------------------------------------------------------------------------------*/
    // READ (char*,size) -> size : Threadsafely read a fixed size of char in a char array
    size_t Comm::read (uint8_t *buffer, size_t size) {
        //std::cout << "READ UINT 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        //std::cout << "READ UINT 2" << std::endl;
        return this->read_ (buffer, size);
    }
    // READ (vector<char>,size) -> size : Threadsafely read a fixed size of char in a char vector
    size_t Comm::read (vector<uint8_t> &buffer, size_t size) {
        //std::cout << "READ VEC 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        uint8_t *buffer_ = new uint8_t[size];
        size_t bytes_read = 0;
        try { bytes_read = this->read_ (buffer_, size); }
        catch (const std::exception &e) { delete[] buffer_; throw; }
        buffer.insert (buffer.end (), buffer_, buffer_+bytes_read);
        delete[] buffer_;
        //std::cout << "READ VEC 2" << std::endl;
        return bytes_read;
    }
    // READ (string,size) -> size : Threadsafely read a fixed size of char in a string
    size_t Comm::read (string &buffer, size_t size) {
        //std::cout << "READ STR 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        uint8_t *buffer_ = new uint8_t[size];
        size_t bytes_read = 0;
        try { bytes_read = this->read_ (buffer_, size); }
        catch (const std::exception &e) { delete[] buffer_; throw; }
        buffer.append (reinterpret_cast<const char*>(buffer_), bytes_read);
        delete[] buffer_;
        //std::cout << "READ STR 2" << std::endl;
        return bytes_read;
    }
    // READ (size) -> string : Threadsafely read a fixed size of char in a string
    string Comm::read (size_t size) {
        std::string buffer;
        this->read (buffer, size);
        return buffer;
    }
    /*---------------------------------------------------------------------------------------------------------------------
     * READLINE : Read a fixed size of char and parse them into a char array, char vector or string, returns size or string
     *-------------------------------------------------------------------------------------------------------------------*/
    // READLINE (string,size) -> size : Read a line (until eol or size is reached) into string, return the string size
    size_t Comm::readline (string& buffer, size_t size) {
        //std::cout << "READ LINE 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        uint8_t *buffer_ = static_cast<uint8_t*> (alloca (size * sizeof (uint8_t)));
        size_t read_so_far = 0;
        while ( read_so_far < size ) {
            size_t bytes_read = this->read_ (buffer_ + read_so_far, 1);
            read_so_far += bytes_read;
            if (bytes_read == 0) break;  // Timeout occured on reading 1 byte
            if(read_so_far < this->eol_len_) continue;
            if (string (reinterpret_cast<const char*>(buffer_+read_so_far-this->eol_len_),this->eol_len_) == this->eol_)
                break; // EOL found
        }
        buffer.append(reinterpret_cast<const char*> (buffer_), read_so_far);
        //std::cout << "READ LINE 2" << std::endl;
        return read_so_far;
    }
    // READLINE (size) -> string : Read a line (until eol or size is reached) into a string, return the string
    string Comm::readline ( size_t size ) {
        string buffer;
        this->readline (buffer, size);
        return buffer;
    }
    /*---------------------------------------------------------------------------------------------------------------------
     * READLINES : Read multiple lines at once, emplace them in a vector of strings
     *-------------------------------------------------------------------------------------------------------------------*/
    // READLINES (size) -> vector<string> : Read a vector of strings (to eol) until a fixed size is reached, return the string
    vector<string> Comm::readlines ( size_t size ) {
        //std::cout << "READ LINES 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        vector<string> lines;
        uint8_t* buffer_ = static_cast<uint8_t*> (alloca (size * sizeof (uint8_t)));
        size_t read_so_far = 0;
        size_t start_of_line = 0;
        while (read_so_far < size) {
            size_t bytes_read = this->read_ (buffer_+read_so_far, 1);
            if ( bytes_read == 0 ) break; // Timeout occured on reading 1 byte
            read_so_far += bytes_read;
            if ( read_so_far < this->eol_len_ + start_of_line ) continue;
            if ( strncmp ( reinterpret_cast<const char*>(buffer_+read_so_far-this->eol_len_),
                        this->eol_.c_str(), this->eol_len_ ) ) {  // EOL found
                lines.emplace_back(reinterpret_cast<const char*>(buffer_+start_of_line), read_so_far-start_of_line);
                start_of_line = read_so_far;
            }
            if ( read_so_far == size ) break; // Reached the maximum read length
        }
        if (start_of_line != read_so_far)
            lines.emplace_back(reinterpret_cast<const char*>(buffer_+start_of_line), read_so_far-start_of_line);
        //std::cout << "READ LINES 2" << std::endl;
        return lines;
    }


    /*=====================================================================================================================
     * SEND : Public methods to send a frame in string, char array or char vector form
     *=====================================================================================================================
     * SEND : Send a string, char* or vector<char>. return the sent size
     *-------------------------------------------------------------------------------------------------------------------*/
    // SEND (string) -> size : Send a string, returns the number of sent char
    size_t Comm::send (const string& data) {
        //std::cout << "SEND STR 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_send);
        //std::cout << "SEND STR 2" << std::endl;
        return this->send_ (reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }
    // SEND (vector<char>) -> size : Send a char vector, returns the number of sent char
    size_t Comm::send (const std::vector<uint8_t> &data) {
        //std::cout << "SEND VEC 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_send);
        //std::cout << "SEND VEC 2" << std::endl;
        return this->send_ (&data[0], data.size());
    }
    // SEND (char*,size) -> size : Send a char array, returns the number of sent char
    size_t Comm::send (const uint8_t *data, size_t size) {
        //std::cout << "SEND UINT 1" << std::endl;
        boost::lock_guard<boost::mutex> lock(this->mtx_send);
        //std::cout << "SEND UINT 2" << std::endl;
        return this->send_ (data, size);
    }

    /*=====================================================================================================================
     * GETTERS AND SETTERS : Public methods to set and get Comm parameters
     *===================================================================================================================*/
    // SET ADDRESS
    void Comm::setAddress (const std::string &address) {
        if ( this->address_.compare(address) == 0 ) return;
        this->address_ = address;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        if ( this->is_connected_ ) { this->close_(); this->open_(); }
    }
    // GET ADDRESS
    const string& Comm::getAddress () const { return this->address_; }
    //---------------------------------------------------------------------------------------------------------------------
    // SET PORT
    void Comm::setPort ( uint16_t port ) {
        if ( this->port_ == port ) return;
        this->port_ = port;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        if ( this->is_connected_ ) { this->close_(); this->open_(); }
    }
    // GET PORT
    uint16_t Comm::getPort () const { return this->port_; }
    //---------------------------------------------------------------------------------------------------------------------
    // SET BAUDRATE
    void Comm::setBaudrate ( uint32_t baudrate ) {
        if ( this->baudrate_ == baudrate ) return;
        this->baudrate_ = baudrate;
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        if ( this->is_connected_ ) { this->close_(); this->open_(); }
    }
    // GET BAUDRATE
    uint32_t Comm::getBaudrate () const { return this->baudrate_; }
    //---------------------------------------------------------------------------------------------------------------------
    // SET EOL : Set end of the line char for payloads (frames) to be read
    void Comm::setEOL ( const string& eol ) {
        boost::lock_guard<boost::mutex> lock(this->mtx_read);
        this->eol_ = eol; this->eol_len_ = eol.length(); }
    // GET EOL : Get end of the line char for payloads (frames) to be read
    const string& Comm::getEOL ( ) const { return this->eol_; }
    //---------------------------------------------------------------------------------------------------------------------
    // SET TIMEOUT : Set timeout for read and send operations and connection (passing a timeout struct)
    void Comm::setTimeout (const Timeout& timeout) {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->timeout_ = timeout;
        this->setOptions_ ( );
    }
    // SET TIMEOUT : Set timeout for read and send operations and connection (specified read, send and connection timeouts)
    void Comm::setTimeout ( double read, double send, double byte, double conn ) {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->timeout_ = Timeout ( read, send, byte, conn );
        this->setOptions_ ( );
    }
    // GET TIMEOUT : Get timeout for read and send operations and connection (passing a timeout struct)
    const Timeout& Comm::getTimeout ( ) const { return this->timeout_; }
    //---------------------------------------------------------------------------------------------------------------------
    // SET SETTINGS
    void Comm::setSettings ( const Settings& settings ) {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->settings_ = settings;
        this->setOptions_();
    }
    void Comm::setSettings ( ByteSize bytesize, Parity parity, StopBits stopbits, FlowControl flowcontrol ) {
        boost::lock_guard<boost::mutex> lock_read(this->mtx_read);
        boost::lock_guard<boost::mutex> lock_send(this->mtx_send);
        this->settings_ = Settings(bytesize, parity, stopbits, flowcontrol);
        this->setOptions_();
    }
    // GET SETTINGS
    const Settings& Comm::getSettings ( ) const { return this->settings_; }

    /*=====================================================================================================================
     * VIRTUAL : Virtual private methods to be extended
     *===================================================================================================================*/
    // Read common function ( VIRTUAL )
    size_t Comm::read_ (uint8_t *data, size_t size) { return -1; }
    // Send common function ( VIRTUAL )
    size_t Comm::send_ (const uint8_t *data, size_t size) { return -1; }
    // Open file descriptor
    void Comm::open_ ( ) { throw new IOException ( "Comm::open : to be extended" ); }
    // Close connection and file descriptor ( COMMON )
    void Comm::close_ ( ) {
        if ( this->is_open_ == true) {
            if ( this->fd_ != -1) {
                int result = ::close (fd_);
                if ( result != 0 )
                    throw new IOException ( "Comm::close", errno );
                this->fd_ = -1;
            }
            this->is_connected_ = false;
            this->is_open_ = false;
        }
    }
    // Connect ( SERIAL )
    void Comm::connect_ ( ) { throw new IOException ( "Comm::connect : to be extended" ); }
    // Set Options ( SERIAL )
    void Comm::setOptions_ ( ) { throw new InterfaceException ( "Comm::setOptions : to be extended" ); }


    /*! Block until there is comm data to read or read_constant
    * number of milliseconds have elapsed. The return value is true when
    * the function exits with the port in a readable state, false otherwise
    * (due to timeout or select interruption). */
    int Comm::waitRead_ () {
        // Setup a select call to block for comm data or a timeout
        fd_set fd_set_;
        FD_ZERO ( &fd_set_ );
        FD_SET ( this->fd_, &fd_set_ );
        int result = select (fd_ + 1, &fd_set_, NULL, NULL, &(this->timeout_.conn));
        //std::cout << "WAIT READ 1 : " << result << std::endl;
        if (result < 0) {
            //std::cout << "WAIT READ 2 : " << errno << std::endl;
            // Select was interrupted
            if (errno == EINTR) return 0;
            // Otherwise there was some error
            throw new IOException ( "waitRead", errno );
        }
        // This shouldn't happen, if r > 0 our fd has to be in the list!
        if (result > 0 && ! FD_ISSET ( fd_, &fd_set_ ) )
            throw new IOException (
                    "Comm::waitRead : select reports ready to read, but our fd isn't in the list, this shouldn't happen!");
        // Data available to read.
        //std::cout << "WAIT READ 3" << std::endl;
        return result;
    }
    int Comm::waitSend_ () {
        //std::cout << "WAIT SEND 1" << std::endl;
        // Setup a select call to block for comm data or a timeout
        fd_set fd_set_;
        FD_ZERO ( &fd_set_ );
        FD_SET ( this->fd_, &fd_set_ );
        int result = select (fd_ + 1, NULL, &fd_set_, NULL, &(this->timeout_.conn));
        if (result < 0) {
            // Select was interrupted
            if (errno == EINTR) return 0;
            // Otherwise there was some error
            throw new IOException ( "waitSend", errno );
        }
        // This shouldn't happen, if r > 0 our fd has to be in the list!
        if (result > 0 && ! FD_ISSET ( fd_, &fd_set_ ) )
            throw new IOException (
                    "Comm::waitSend : select reports ready to send, but our fd isn't in the list, this shouldn't happen!");
        // Data available to read.
        //std::cout << "WAIT SEND 2" << std::endl;
        return result;
    }

    // FLUSH : For the ether socket this does nothing ( SERIAL )
    void Comm::flush_ ( ) { }
    void Comm::flushInput_ ( ) { }
    void Comm::flushOutput_ ( ) { }

}