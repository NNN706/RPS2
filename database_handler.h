#pragma once
#ifndef DATABASE_HANDLER_H
#define DATABASE_HANDLER_H

#include <mysqlx/xdevapi.h>
#include <mutex>
#include <string>

class DatabaseHandler {
private:
    std::mutex db_mutex;
    mysqlx::Session* session;

    const std::string DB_HOST = "localhost";
    const int DB_PORT = 3306;
    const std::string DB_USER = "sorting_user";
    const std::string DB_PASS = "sorting_pass";
    const std::string DB_NAME = "sorting_db";

public:
    DatabaseHandler();
    ~DatabaseHandler();

    bool connect();
    void disconnect();
    bool isConnected() const;

    void initializeDatabase();

    bool registerUser(const std::string& username, const std::string& email,
        const std::string& password, std::string& error_msg);
    bool loginUser(const std::string& username, const std::string& password,
        struct User& user, std::string& error_msg);

    bool saveSortingResult(int user_id, const std::vector<int>& input,
        const std::vector<int>& output, long long exec_time);

    std::vector<std::vector<std::string>> getSortingHistory(int user_id);

private:
    std::string hashPassword(const std::string& password);
};

#endif // DATABASE_HANDLER_H