#include "peer.h"
#include "config.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>
#include <limits>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace boost::asio;
using namespace ftxui;
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

    try {
        cout << "Enter a port number (" << PORT_MIN << "-" << PORT_MAX << ") or press enter for default port: " << DEFAULT_PORT << "\n";
        std::getline(cin, port_input);

        if (port_input.empty()) {
            return DEFAULT_PORT;
        }

        port = std::stoi(port_input);
        if (port >= PORT_MIN && port <= PORT_MAX) {
            return port;
        }
        else {
            cout << "Port number must be between " << PORT_MIN << " and " << PORT_MAX << ". Please try again.\n";
            return get_port();
        }
    }
    catch (const std::exception& e) {
        cout << "Invalid input. Please enter a numeric port number.\n";
        return get_port();
    }
}

// Prompts the user to choose a colour for their name
Color get_user_colour() {
    try {
        cout << "Choose a colour for your username:\n";
        cout << "1: Red\n";
        cout << "2: Green\n";
        cout << "3: Blue\n";
        cout << "4: Yellow\n";
        cout << "5: Cyan\n";
        cout << "6: Magenta\n";
        cout << "Enter a number (1-6): ";

        int choice;
        cin >> choice;
		// Clear the input buffer
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
        case 1: return Color::Red;
        case 2: return Color::Green;
        case 3: return Color::Blue;
        case 4: return Color::Yellow;
        case 5: return Color::Cyan;
        case 6: return Color::Magenta;
        default: return Color::White;
        }
    }
    catch (const std::exception& e) {
		// Recursively call the function until a valid input is received
        cout << "Please enter a number between 1 and 6\n";
		get_user_colour();
    }
}

// Sets up and runs the host side of the application
void run_host(io_context& io, ssl::context& ssl_context, std::atomic<bool>& connected, const string& ip, const string& name, Color user_colour, int port) {
    try {
		// Create a shared pointer to the Peer object
        auto host_peer = std::make_shared<Peer>(io, ssl_context, connected, name, user_colour);
		// Create an acceptor object to listen for incoming connections
        tcp::acceptor acceptor(io, tcp::endpoint(ip::make_address(ip), port));

		// Display host information
        cout << "Host IP: " << IP_ADDRESS << ", Port: " << port << endl;
        cout << "Waiting for someone to connect..." << endl;

		// Asynchronously accept incoming connections
        acceptor.async_accept(host_peer->socket().lowest_layer(), [&host_peer](boost::system::error_code ec) {
            if (!ec) {
                cout << "Host: Connection established." << endl;
				// Start the SSL handshake in server mode
                host_peer->start_handshake(ssl::stream_base::server);
            }
            else {
                cout << "Host: Error in accepting connection: " << ec.message() << endl;
            }
            });

		// Start the IO context in a separate thread
        std::thread io_thread([&io]() {
            try {
				// Run the IO context
                io.run();
            }
            catch (const std::exception& e) {
                cout << "Exception in IO thread: " << e.what() << endl;
            }
            });

        while (!connected) {
			// Wait for the connection to be established
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

		// Display exit chat instructions
        cout << "Enter 'exit' to quit the chat." << endl;

		// Continuously read user input and send messages
        while (true) {
            try {
                string message;
                // Display the prompt and read user input
                host_peer->display_prompt();
                // Read the message from the user
                std::getline(cin, message);
                if (message == "exit") break;
                // Send the message if it is not empty
                if (!message.empty()) host_peer->send_message(message);
            }
            catch (const std::exception& e) {
                cout << "Error sending message: " << e.what() << endl;
            }
        }

		// Shutdown the connection and stop the IO context
        host_peer->shutdown();
        io.stop();
		// Wait for the IO thread to finish
        io_thread.join();
    }
    catch (const std::exception& e) {
        cout << "Error running host: " << e.what() << endl;
    }
}

// Sets up and runs the client side of the application
void run_client(io_context& io, ssl::context& ssl_context, std::atomic<bool>& connected, const string& host, const string& name, Color user_color, int port) {
    try {
		// Create a shared pointer to the Peer object
        auto client_peer = std::make_shared<Peer>(io, ssl_context, connected, name, user_color);

        cout << "Host IP: 127.0.0.1, Port: " << port << endl;
		// Attempt to connect to the host
        client_peer->socket().lowest_layer().connect(tcp::endpoint(ip::make_address(host), port));
		// Start the SSL handshake in client mode
        client_peer->start_handshake(ssl::stream_base::client);

		// Start the IO context in a separate thread
        std::thread io_thread([&io]() {
            try {
				// Run the IO context
                io.run();
            }
            catch (const std::exception& e) {
                cout << "Exception in IO thread: " << e.what() << endl;
            }
            });

        while (!connected) {
			// Wait for the connection to be established
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

		// Display exit chat instructions
        cout << "Enter 'exit' to quit the chat." << endl;

        while (true) {
            try {
                string message;
				// Display the prompt and read user input
                client_peer->display_prompt();
				// Read the message from the user
                std::getline(cin, message);
				// Exit the chat if the user types 'exit'
                if (message == "exit") break;
                // Send the message if it is not empty
                if (!message.empty()) client_peer->send_message(message);
            }
            catch (const std::exception& e) {
                cout << "Error sending message: " << e.what() << endl;
            }
        }

		// Shutdown the connection and stop the IO context
        client_peer->shutdown();
        io.stop();
		// Wait for the IO thread to finish
        io_thread.join();
    }
    catch (const std::exception& e) {
        cout << "Error running client: " << e.what() << endl;
    }
}

// Creates and runs the appropriate peer (host or client)
void create_peer(const std::string& ip, const std::string& name, Color user_colour, int port, bool is_host) {
	// Create the IO context and SSL context
    boost::asio::io_context io;
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12);
	// Atomic flag to track connection status
    std::atomic<bool> connected(false);

    try {
		// Configure the SSL context with the necessary certificate and key files
        configure_ssl_context(ssl_context);

        if (is_host) {
			// Run the host side of the application
            run_host(io, ssl_context, connected, ip, name, user_colour, port);
        }
        else {
			// Run the client side of the application
            run_client(io, ssl_context, connected, ip, name, user_colour, port);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception in create_peer: " << e.what() << std::endl;
    }
}

// Main function to start the program and collect user inputs
int main() {
    string ip = IP_ADDRESS;
    int port;
    char mode;
    bool is_host;
    string name;

    try {
        cout << "What is your username?: ";
        cin >> name;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        Color user_colour = get_user_colour();

        cout << "Are you hosting the connection? (y/n): ";
        cin >> mode;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check if the user is hosting the connection
        is_host = (mode == 'y' || mode == 'Y');
		// Get the port number from the user
        port = get_port();

		// Create the appropriate peer based on user input
        create_peer(ip, name, user_colour, port, is_host);
    }
    catch (const std::exception& e) {
        cout << "Exception in main: " << e.what() << endl;
    }

    return 0;
}
