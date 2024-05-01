struct server{
    char *browser_port;
    char *name;
};
void server_listen(struct server *server){
    // Create an io_context
    boost::asio::io_context io_context;
    // Create a server object
    HttpServer server(io_context, 5467);
    // Run the io_context to start the server
    io_context.run();
}