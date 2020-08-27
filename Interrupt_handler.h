//
// Created by arpa on 8/26/20.
//

#include <any>
#include <vector>
#include <iostream>
#include "HTTPServer.h"

#ifndef HTTPSERVER_INTERRUPT_HANDELER_H
#define HTTPSERVER_INTERRUPT_HANDELER_H

#endif //HTTPSERVER_INTERRUPT_HANDELER_H

class Interrupt_handler {
public:
    static Interrupt_handler &getInstance() {
	    static Interrupt_handler instance;
	    return instance;
    }

    Interrupt_handler(Interrupt_handler const &) = delete;

    void operator=(Interrupt_handler const &) = delete;

    static void handle(int x) {
	    std::cerr << "Exiting cleanly" << std::endl;
	    for (auto item: getInstance().http_servers)
		    delete item;
	    exit(0);
    }

    static void watch(HTTPServer *http_server) {
	    getInstance().http_servers.push_back(http_server);
    }

private:
    Interrupt_handler() = default;
    std::vector<HTTPServer *> http_servers;
};