#include "config.h"

// Configures the SSL context with the necessary certificate, key, and CA files
void configure_ssl_context(boost::asio::ssl::context& ssl_context) {
    // Set SSL options to disable outdated protocols not considered secure,
	// default_workarounds option: Applies various bug workarounds
    ssl_context.set_options(boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3);

    // Load the host certificate chain
    ssl_context.use_certificate_chain_file(CERTIFICATE_FILE);
    // Load the private key for this peer
    ssl_context.use_private_key_file(PRIVATE_KEY_FILE, boost::asio::ssl::context::pem);
    // Load the certificate authority file to verify peers
    ssl_context.load_verify_file(CA_FILE);
    // Set SSL verification mode to require peer verification
    ssl_context.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
}
