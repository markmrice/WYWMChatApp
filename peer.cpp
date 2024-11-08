#include "peer.h"
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

// Constructor to initialize SSL socket, connection status, user name, and name colour
Peer::Peer(boost::asio::io_context& io, boost::asio::ssl::context& ssl_context, std::atomic<bool>& connected, const string& user_name, Color user_colour)
    : socket_(io, ssl_context), is_connected_(connected), name(user_name), colour_(user_colour) {}

// Returns a reference to the SSL socket
boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Peer::socket() {
    return socket_;
}

// Initiates an SSL handshake (either host or client mode) for secure communication
void Peer::start_handshake(boost::asio::ssl::stream_base::handshake_type type) {
    std::cout << "Starting handshake..." << std::endl;
	// Start the asynchronous handshake operation
    socket_.async_handshake(type, [self = shared_from_this()](boost::system::error_code ec) {
        if (!ec) {
			// Handshake successful, start reading messages
            std::cout << "Handshake successful." << std::endl;
            self->is_connected_ = true;
            self->start_read();
        }
        else {
			// Handshake failed, set connection status to false
            std::cout << "Handshake failed: " << ec.message() << std::endl;
            self->is_connected_ = false;
        }
        });
}

// Asynchronous read operation to receive messages from the peer
void Peer::start_read() {
	// Start an asynchronous read operation to read until a newline character is encountered
    async_read_until(socket_, buffer_, "\n", [self = shared_from_this()](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
			// Extract the message from the buffer
            std::istream is(&self->buffer_);
            string message;
            std::getline(is, message);

            if (!message.empty()) {
                // Extract colour and message from received string
                size_t separator = message.find('|');
                if (separator != string::npos) {
                    string colour_str = message.substr(0, separator);
                    string text_message = message.substr(separator + 1);

                    Color message_colour = Peer::string_to_colour(colour_str);

                    // Find the colon after the name to separate the username and message
                    size_t name_end = text_message.find(':');
                    if (name_end != string::npos) {
                        // Extract the user's name and the actual message
                        string user_name = text_message.substr(0, name_end + 1); // Include the colon
                        string user_message = text_message.substr(name_end + 1);

                        // Render the name in colour and message in default colour
                        ftxui::Screen name_screen(user_name.size(), 1);
                        auto rendered_name = ftxui::text(user_name) | ftxui::color(message_colour);
                        Render(name_screen, rendered_name);

                        // Print the coloured name and uncoloured message
                        std::cout << "\33[2K\r" << name_screen.ToString() << user_message << std::endl;
                    }
                    else {
                        // In case we don't find the colon, print the whole message in default format
                        std::cout << "\33[2K\r" << text_message << std::endl;
                    }
                }
                else {
					// In case we don't find the separator, print the whole message in default format
                    std::cout << "\33[2K\r" << message << std::endl;
                }
				// Display the prompt for new input
                self->display_prompt();
            }

			// Consume the read data and start reading again
            self->buffer_.consume(length);
            self->start_read();
        }
        else {
            std::cout << "Error reading message: " << ec.message() << std::endl;
        }
        });
}

// Sends a message asynchronously to the connected peer
void Peer::send_message(const string& message) {
    if (!is_connected_) {
		// If not connected, display an error message
        std::cout << "Error: Not connected to peer yet." << std::endl;
        return;
    }

	// Construct the message with the user's name and colour
    auto msg = std::make_shared<string>(Peer::colour_to_string(colour_) + "|" + name + ": " + message + "\n");
    async_write(socket_, boost::asio::buffer(*msg), [msg, this](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
            std::cout << "Error sending message: " << ec.message() << std::endl;
        }
        else {
            display_prompt();
        }
        });
}

// Clears the line and displays a prompt with the user's name for new input
void Peer::display_prompt() {
    // The width is the length of the name
    int name_width = name.size();  
    // Create a screen to render the name
	ftxui::Screen name_screen(name_width, 1); 
    // Render the name in colour
	auto prompt = ftxui::text(name) | ftxui::color(colour_); 
    // Render the name on the screen
	Render(name_screen, prompt); 
    // Clear the line and display the prompt
	std::cout << "\33[2K\r" << name_screen.ToString() << ": " << std::flush;
}

// Gracefully shuts down the connection, closing the socket if open
void Peer::shutdown() {
	// Close the socket and shutdown the connection
    boost::system::error_code ec;

	// Shutdown the socket
    if (socket_.lowest_layer().is_open()) {
		// Shutdown the socket to disable further sends and receives
        socket_.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
        if (ec) {
            std::cout << "Shutdown error: " << ec.message() << std::endl;
        }

		// Close the socket
        socket_.lowest_layer().close(ec);
        if (ec) {
            std::cout << "Close error: " << ec.message() << std::endl;
        }
        else {
            std::cout << "Connection closed successfully." << std::endl;
        }
    }
    else {
        std::cout << "Socket already closed." << std::endl;
    }
}

// Utility function to convert a colour string to FTXUI Colour
Color Peer::string_to_colour(const string& colour_str) {
    if (colour_str == "red") return Color::Red;
    if (colour_str == "green") return Color::Green;
    if (colour_str == "blue") return Color::Blue;
    if (colour_str == "yellow") return Color::Yellow;
    if (colour_str == "cyan") return Color::Cyan;
    if (colour_str == "magenta") return Color::Magenta;
    return Color::White;
}

// Utility function to convert FTXUI Colour to a string
string Peer::colour_to_string(Color colour) {
    if (colour == Color::Red) return "red";
    if (colour == Color::Green) return "green";
    if (colour == Color::Blue) return "blue";
    if (colour == Color::Yellow) return "yellow";
    if (colour == Color::Cyan) return "cyan";
    if (colour == Color::Magenta) return "magenta";
    return "white";
}
