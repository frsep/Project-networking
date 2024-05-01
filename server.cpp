
// write a server class that listens on a port and accepts connections
// from clients. The server should be able to handle multiple clients
// at the same time. The server should be able to receive messages from
// clients and send messages to clients. The server should be able to
// handle multiple clients at the same time.

#include <iostream>
#include <string>
#include <boost/asio.hpp>

class HttpServer {
public:
    HttpServer(boost::asio::io_context& io_context, short port)
        : io_context_(io_context),
          acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          socket_(io_context) {
        acceptConnection();
    }

private:
    void acceptConnection() {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    readRequest();
                } else {
                    std::cerr << "Error accepting connection: " << ec.message() << std::endl;
                }
            });
    }

    void readRequest() {
        boost::asio::async_read_until(socket_, request_, "\r\n\r\n",
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    std::istream request_stream(&request_);
                    std::string http_request;
                    std::getline(request_stream, http_request);

                    std::cout << "Received HTTP request: " << http_request << std::endl;

                    // Here you can process the request and generate a response
                    std::string http_response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
                    
                    // Write the response back to the client
                    boost::asio::async_write(socket_, boost::asio::buffer(http_response),
                        [this](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
                            if (!ec) {
                                // After writing response, continue reading next request
                                readRequest();
                            } else {
                                std::cerr << "Error sending response: " << ec.message() << std::endl;
                            }
                        });
                } else {
                    std::cerr << "Error reading request: " << ec.message() << std::endl;
                }
            });
    }

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf request_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        HttpServer server(io_context, 5467); // Run server on port 5467
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
