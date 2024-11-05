#include "peer.h"
#include <iostream>

// Constructor to initialize SSL socket, connection status, and user name
Peer::Peer(boost::asio::io_context& io, boost::asio::ssl::context& ssl_context, std::atomic<bool>& connected, const string& user_name)
    // Initialize the SSL socket with the provided I/O context and SSL context
    : socket_(io, ssl_context), is_connected_(connected), name(user_name) {}

// Returns a reference to the SSL socket
boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Peer::socket() {
    return socket_;
}

// Initiates an SSL handshake (either host or client mode) for secure communication
void Peer::start_handshake(boost::asio::ssl::stream_base::handshake_type type) {
	// Display a message indicating the start of the handshake
    std::cout << "Starting handshake..." << std::endl;
	// Asynchronously initiate the SSL handshake
	// shared_from_this() is used to safely pass a shared pointer of the object to the lambda function
    socket_.async_handshake(type, [self = shared_from_this()](boost::system::error_code ec) {
        if (!ec) {
			// Handshake successful, update connection status and start reading messages
            std::cout << "Handshake successful." << std::endl;
            self->is_connected_ = true;
            self->start_read();
        }
        else {
			// Handshake failed, update connection status
            std::cout << "Handshake failed: " << ec.message() << std::endl;
            self->is_connected_ = false;
        }
        });
}

// Asynchronous read operation to receive messages from the peer
void Peer::start_read() {
	// Asynchronously read data until a newline character is encountered
	// shared_from_this() is used to safely pass a shared pointer of the object to the lambda function
    async_read_until(socket_, buffer_, "\n", [self = shared_from_this()](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
			// Extract the message from the buffer and print it to the console
            std::istream is(&self->buffer_);
            string message;
            std::getline(is, message);

			// Check if the message is not empty
            if (!message.empty()) {
                std::cout << "\33[2K\r" << message << std::endl;  // Clear line and print received message
                self->display_prompt();                           // Display prompt after printing message
            }

            self->buffer_.consume(length); // Clear the buffer and read again
            self->start_read();            // Recursively call to keep reading messages
        }
        else {
			// If an error occurs, print the error message
            std::cout << "Error reading message: " << ec.message() << std::endl;
        }
        });
}

// Sends a message asynchronously to the connected peer
void Peer::send_message(const string& message) {
    if (!is_connected_) {
		// If not connected to a peer, display an error message
        std::cout << "Error: Not connected to peer yet." << std::endl;
        return;
    }

	// Asynchronously write the message to the socket
	// make_shared is used to create a shared pointer to the message string
    auto msg = std::make_shared<string>(name + ": " + message + "\n"); // Format message with username
	// Write the message to the socket
    async_write(socket_, boost::asio::buffer(*msg), [msg, this](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
			// If an error occurs, print the error message
            std::cout << "Error sending message: " << ec.message() << std::endl;
        }
        else {
			// Message sent successfully, display the prompt for new input
            display_prompt();
        }
        });
}

// Clears the line and displays a prompt with the user's name for new input
void Peer::display_prompt() {
    std::cout << "\33[2K\r" << name << ": " << std::flush;
}

// Gracefully shuts down the connection, closing the socket if open
void Peer::shutdown() {
	// error_code object for handling errors
    boost::system::error_code ec;

	// Check if the socket is open
    if (socket_.lowest_layer().is_open()) {
        // Attempt to shut down both send and receive on the socket
        socket_.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
        if (ec) {
			// If an error occurs, print the error message
            std::cout << "Shutdown error: " << ec.message() << std::endl;
        }

        // Close the socket to release associated resources
        socket_.lowest_layer().close(ec);
        if (ec) {
			// If an error occurs, print the error message
            std::cout << "Close error: " << ec.message() << std::endl;
        }
        else {
			// Print a message indicating successful closure
            std::cout << "Connection closed successfully." << std::endl;
        }
    }
    else {
		// Print a message indicating that the socket is already closed
        std::cout << "Socket already closed." << std::endl;
    }
}
