#include "user_manager.h"
#include "database_handler.h"

UserManager::UserManager(DatabaseHandler* db) : dbHandler(db) {}

bool UserManager::registerUser(const std::string& username, const std::string& email,
    const std::string& password, std::string& error_msg) {
    return dbHandler->registerUser(username, email, password, error_msg);
}

bool UserManager::loginUser(const std::string& username, const std::string& password,
    User& user, std::string& error_msg) {
    return dbHandler->loginUser(username, password, user, error_msg);
}

bool UserManager::logoutUser(User& user) {
    // —брос данных пользовател€
    user.id = -1;
    user.username = "";
    user.email = "";
    user.role = "";
    user.created_at = 0;
    return true;
}

bool UserManager::isAdmin(const User& user) const {
    return user.role == "admin";
}