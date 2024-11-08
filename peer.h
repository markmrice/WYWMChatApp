#ifndef PEER_H
#define PEER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <ftxui/screen/color.hpp>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using std::string;
using namespace ftxui;

// Class representing a peer-to-peer connection with SSL encryption
class Peer : public std::enable_shared_from_this<Peer> {
public:
    // Constructor to initialize SSL socket, connection status, user name, and colour
    Peer(boost::asio::io_context& io, boost::asio::ssl::context& ssl_context, std::atomic<bool>& connected, const string& user_name, Color user_color);

    // Returns a reference to the SSL socket
    stream<tcp::socket>& socket();

    // Initiates an SSL handshake (either host or client mode) for secure communication
    void start_handshake(boost::asio::ssl::stream_base::handshake_type type);

    // Sends a message asynchronously to the connected peer
    void send_message(const string& message);

    // Clears the line and displays a prompt with the user's name for new input
    void display_prompt();

    // Gracefully shuts down the connection, closing the socket if open
    void shutdown();

    // Utility functions to convert between string and colour
    static Color string_to_colour(const string& color_str);
    static string colour_to_string(Color color);

private:
    void start_read(); // Starts asynchronous reading of messages

    stream<tcp::socket> socket_;       // SSL socket for secure communication
    boost::asio::streambuf buffer_;    // Buffer to store incoming data
    std::atomic<bool>& is_connected_;  // Tracks connection state
    string name;                       // Username of the user
    Color colour_;                      // Colour of the user for display purposes
};

#endif // PEER_H
