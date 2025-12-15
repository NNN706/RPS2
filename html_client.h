#pragma once
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

void sendToWeb(
    const std::string& inputArray,
    const std::string& outputArray,
    int executionTime
);

#endif
