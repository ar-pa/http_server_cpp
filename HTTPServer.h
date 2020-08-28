//
// Created by arpa on 8/25/20.
//

#ifndef HTTPSERVER_HTTPSERVER_H
#define HTTPSERVER_HTTPSERVER_H

#include <functional>
#include <utility>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zconf.h>
#include <cstring>
#include <thread>
#include "HTTPRequest.h"
#include "HTTPResponse.h"

const int HTTP_BASE_BUFFER_SIZE = 1024;
const int HTTP_MAX_WAITING_CONNECTIONS = 1e6, MAX_THREADS = 3e4;

char buffer[HTTP_BASE_BUFFER_SIZE];

class HTTPServer {
private:
    std::function<HTTPResponse(HTTPRequest)> handler;
    std::vector<std::thread *> threads;
    std::vector<int> sockets;

    static void serve(const std::function<HTTPResponse(HTTPRequest)> &handler, int new_socket) {
	    std::string head_request;
	    while (true) {
		    int t = read(new_socket, buffer, HTTP_BASE_BUFFER_SIZE - 1);
		    if (t < 0) {
			    perror("read error");
			    return;
		    }
		    buffer[t] = 0;
		    head_request += buffer;
		    if (t == 0 || t < HTTP_BASE_BUFFER_SIZE - 1 && head_request.find("\r\n\r\n")) {
			    break;
		    }
	    }
	    HTTPRequest request(head_request);
	    if (!request.get_header("Content-Length")) {
		    request.set_header("Content-Length", "0");
	    }
	    int remaining_body =
		    stoi(request.get_header("Content-Length").value()) -
		    (head_request.size() - head_request.find("\r\n\r\n") - 4);
	    if (remaining_body > 0) {
		    char buffer[remaining_body + 1];
		    int t = read(new_socket, buffer, remaining_body);
		    if (t < 0) {
			    perror("read error x");
			    return;
		    }
		    buffer[t] = 0;
		    std::string full_request = head_request + buffer;
		    request = HTTPRequest(full_request);
	    }
	    HTTPResponse response = handler(request);
//	    HTTPResponse response(200, "");
	    send(new_socket, response.to_string().data(), response.to_string().size(), 0);
	    close(new_socket);
    }

public:
    explicit HTTPServer(std::function<HTTPResponse(HTTPRequest)> handeler) : handler(std::move(handeler)) {}

    virtual ~HTTPServer() {
	    std::cerr << "Shutting down server\n";
	    for (auto socket : sockets)
		    close(socket);
    }

    [[noreturn]] void listen(int port) {
	    int server_fd;
	    sockaddr_in address{};
	    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		    perror("socket failed");
		    exit(EXIT_FAILURE);
	    }
	    address.sin_family = AF_INET;
	    address.sin_addr.s_addr = INADDR_ANY;
	    address.sin_port = htons(port);
	    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, new int(1), sizeof(int))) {
		    perror("setsockopt");
		    exit(EXIT_FAILURE);
	    }
	    if (bind(server_fd, (sockaddr *) &address, sizeof(address)) < 0) {
		    perror("bind failed");
		    exit(EXIT_FAILURE);
	    }
	    if (::listen(server_fd, HTTP_MAX_WAITING_CONNECTIONS) < 0) {
		    perror("listen");
		    exit(EXIT_FAILURE);
	    }
	    sockets.push_back(server_fd);
	    while (true) {
		    int new_socket;
		    if ((new_socket = accept(server_fd, (sockaddr *) &address, new socklen_t(sizeof(address)))) < 0) {
			    perror("accept");
			    continue;
		    }
//		    close(new_socket);
//		    continue;
//		    sockets.push_back(new_socket);
		    if (threads.size() >= MAX_THREADS) {
			    threads[threads.size() - MAX_THREADS] -> join();
			    delete threads[threads.size() - MAX_THREADS];
		    }
		    threads.push_back(new std::thread(serve, handler, new_socket));
//		    serve(handler, new_socket);
	    }
    }
};


#endif //HTTPSERVER_HTTPSERVER_H
