#ifndef CONFIG_H
#define CONFIG_H

#include <boost/asio/ssl.hpp>
#include <string>

// Paths to SSL certificate, private key, and CA file for secure communication
const std::string CERTIFICATE_FILE = "D:\\openssl-3.4.0\\certificate.crt";
const std::string PRIVATE_KEY_FILE = "D:\\openssl-3.4.0\\peer.key";
const std::string CA_FILE = "D:\\openssl-3.4.0\\ca.crt";

// Function to configure SSL context with necessary certificate and key files
void configure_ssl_context(boost::asio::ssl::context& ssl_context);

#endif // CONFIG_H
