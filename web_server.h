#pragma once

#include <string>

class WebServer {
private:
    int port;

public:
    WebServer(int port = 8080);
    void start();
    void stop();
    void generateHelpPage();
};