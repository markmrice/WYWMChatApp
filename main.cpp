#include "peer.h"
#include "config.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <limits>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::cin;
using std::endl;

// Define the port range and default port
const int PORT_MIN = 8000;
const int PORT_MAX = 9000;
const int DEFAULT_PORT = 8080;
const string IP_ADDRESS = "127.0.0.1";

// Prompts the user for a port number within the specified range, defaults to 8080 if input is empty
int get_port() {
    string port_input;
    int port;

    // Prompt the user for a port number
    cout << "Enter a port number (" << PORT_MIN << "-" << PORT_MAX << ") or press enter for default port: " << DEFAULT_PORT << "\n";
    std::getline(cin, port_input);

    // Use default port if input is empty
    if (port_input.empty()) {
        return DEFAULT_PORT;
    }

    try {
        // Convert the input to an integer
        port = std::stoi(port_input);

        // Check if the port is within the valid range
        if (port >= PORT_MIN && port <= PORT_MAX) {
            return port;  // Valid port, return it
        }
        else {
            // If the port is out of range, display an error message
            cout << "Port number must be between " << PORT_MIN << " and " << PORT_MAX << ". Please try again.\n";
            return get_port();  // Recursively prompt again
        }
    }
    catch (...) {
        // If the input is not a valid integer, display an error message
        cout << "Invalid input. Please enter a numeric port number.\n";
        return get_port();  // Recursively prompt again
    }
}

// Sets up and runs the host side of the application
void run_host(io_context& io, ssl::context& ssl_context, std::atomic<bool>& connected, const string& ip, const string& name, int port) {
    // Create a new Peer object for the host
    auto host_peer = std::make_shared<Peer>(io, ssl_context, connected, name);
    // Create an acceptor to listen for incoming connections
    tcp::acceptor acceptor(io, tcp::endpoint(ip::make_address(ip), port));

    // Display connection information
    cout << "Host IP: " << IP_ADDRESS << ", Port: " << port << endl;
    cout << "Waiting for someone to connect..." << endl;

    // Accept incoming connections
    acceptor.async_accept(host_peer->socket().lowest_layer(), [&host_peer](boost::system::error_code ec) {
        if (!ec) {
            // Connection established
            cout << "Host: Connection established." << endl;
            host_peer->start_handshake(ssl::stream_base::server);
        }
        else {
            // Error in accepting connection
            cout << "Host: Error in accepting connection: " << ec.message() << endl;
        }
        });

    // Create and start a new thread to run IO Context event loop for asynchronous operations
    // lambda function to capture the io context by reference
    std::thread io_thread([&io]() { io.run(); });

    // Wait until the connection is established
    while (!connected) {
        // Sleep for a short duration to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Prompt the user on how to exit the chat
    cout << "Enter 'exit' to quit the chat." << endl;

    // Message loop for the host
    while (true) {
        string message;
        host_peer->display_prompt();
        std::getline(cin, message);
        if (message == "exit") break;
        if (!message.empty()) host_peer->send_message(message);
    }

    // Shutdown the host peer and stop the IO context
    host_peer->shutdown();
    io.stop();
    io_thread.join(); // Wait for the IO thread to finish
}

// Sets up and runs the client side of the application
void run_client(io_context& io, ssl::context& ssl_context, std::atomic<bool>& connected, const string& host, const string& name, int port) {
    // Create a new Peer object for the client
    auto client_peer = std::make_shared<Peer>(io, ssl_context, connected, name);

    // Display connection information
    cout << "Host IP: 127.0.0.1, Port: " << port << endl;

    // Connect to the host
    client_peer->socket().lowest_layer().connect(tcp::endpoint(ip::make_address(host), port));
    // Start the SSL handshake in client mode
    client_peer->start_handshake(ssl::stream_base::client);

    // Create and start a new thread to run IO Context event loop for asynchronous operations
    std::thread io_thread([&io]() { io.run(); });

    // Wait until the connection is established
    while (!connected) {
        // Sleep for a short duration to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Prompt the user on how to exit the chat
    cout << "Enter 'exit' to quit the chat." << endl;

    // Message loop for the client
    while (true) {
        string message;
        client_peer->display_prompt();
        std::getline(cin, message);
        if (message == "exit") break;
        if (!message.empty()) client_peer->send_message(message);
    }

    // Shutdown the client peer and stop the IO context
    client_peer->shutdown();
    io.stop();
    io_thread.join();
}

// Creates and runs the appropriate peer (host or client)
void create_peer(const std::string& ip, const std::string& name, int port, bool is_host) {
    // Initialize the IO context and SSL context
    boost::asio::io_context io;
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12);
    // Atomic flag to track connection status
    std::atomic<bool> connected(false);

    // Configure the SSL context with the necessary certificate and key files
    configure_ssl_context(ssl_context);  // Call the configuration function from config.cpp

    try {
        // Run the host or client based on user input
        if (is_host) {
            // Run the host side of the application
            run_host(io, ssl_context, connected, ip, name, port);
        }
        else {
            // Run the client side of the application
            run_client(io, ssl_context, connected, ip, name, port);
        }
    }
    catch (const std::exception& e) {
        // Handle exceptions during peer creation
        std::cout << "Exception in create_peer: " << e.what() << std::endl;
    }
}

// Main function to start the program and collect user inputs
int main() {
    string ip = IP_ADDRESS;  // Default IP for both host and client
    int port;
    char mode;
    bool is_host;
    string name;

    try {
        // Collect user information
        cout << "What is your username?: ";
        cin >> name;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline from input buffer

        // Ask if the user is hosting the connection
        cout << "Are you hosting the connection? (y/n): ";
        cin >> mode;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline from input buffer

        // Set the mode based on user input
        is_host = (mode == 'y' || mode == 'Y');

        // Prompt for port only
        port = get_port();

        // Initialize and run the peer based on user input
        create_peer(ip, name, port, is_host);
    }
    catch (const std::exception& e) {
        // Handle exceptions in the main function
        cout << "Exception in main: " << e.what() << endl;
    }

    return 0;
}