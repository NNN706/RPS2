#include "user.h"
#include <iostream>

static int currentUserID = 1;

bool registerUser(const std::string& username, const std::string& password) {
    std::cout << "[Регистрация] Пользователь " << username << " зарегистрирован.\n";
    currentUserID = 1;
    return true;
}

bool loginUser(const std::string& username, const std::string& password) {
    std::cout << "[Вход] Пользователь " << username << " вошёл.\n";
    currentUserID = 1;
    return true;
}

int getCurrentUserID() {
    return currentUserID;
}
