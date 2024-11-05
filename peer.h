#ifndef PEER_H
#define PEER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <atomic>
#include <memory>
#include <string>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using std::string;

// Class representing a peer-to-peer connection with SSL encryption
// enables_shared_from_this is used to safely pass a shared pointer of the object to the lambda functions
class Peer : public std::enable_shared_from_this<Peer> {
public:
	// Constructor to initialize SSL socket, connection status, and user name
    Peer(boost::asio::io_context& io, boost::asio::ssl::context& ssl_context, std::atomic<bool>& connected, const string& user_name);

	// Returns a reference to the SSL socket
    stream<tcp::socket>& socket();
	// Initiates an SSL handshake (either host or client mode) for secure communication
    void start_handshake(boost::asio::ssl::stream_base::handshake_type type);
    void send_message(const string& message); // Asynchronous read operation to receive messages from the peer
    void display_prompt(); // Sends a message asynchronously to the connected peer
    void shutdown(); // Graceful shutdown method

private:
    void start_read(); // Starts asynchronous reading of messages

    stream<tcp::socket> socket_;       // SSL socket for secure communication
    boost::asio::streambuf buffer_;    // Buffer to store incoming data
    std::atomic<bool>& is_connected_;  // Tracks connection state
    string name;                       // Username of the user
};

#endif // PEER_H
