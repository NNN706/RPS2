#pragma once

#include <string>

bool registerUser(const std::string& username, const std::string& password);
bool loginUser(const std::string& username, const std::string& password);
int getCurrentUserID();