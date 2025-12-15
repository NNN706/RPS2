#pragma once
#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string role;
    time_t created_at;
};

class UserManager {
private:
    DatabaseHandler* dbHandler;

public:
    UserManager(DatabaseHandler* db);

    bool registerUser(const std::string& username, const std::string& email,
        const std::string& password, std::string& error_msg);
    bool loginUser(const std::string& username, const std::string& password,
        User& user, std::string& error_msg);
    bool logoutUser(User& user);

    bool isAdmin(const User& user) const;
};

#endif // USER_MANAGER_H