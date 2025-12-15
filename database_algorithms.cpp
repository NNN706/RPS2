#include "database_handler.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <ctime>

DatabaseHandler::DatabaseHandler() : session(nullptr) {}

DatabaseHandler::~DatabaseHandler() {
    disconnect();
}

bool DatabaseHandler::connect() {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
        if (session) {
            delete session;
        }

        session = new mysqlx::Session(
            mysqlx::SessionSettings(
                DB_HOST, DB_PORT,
                DB_USER, DB_PASS, DB_NAME
            )
        );

        std::cout << "Connected to database successfully!" << std::endl;
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        return false;
    }
}

void DatabaseHandler::disconnect() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (session) {
        delete session;
        session = nullptr;
    }
}

bool DatabaseHandler::isConnected() const {
    return session != nullptr;
}

std::string DatabaseHandler::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void DatabaseHandler::initializeDatabase() {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
        mysqlx::Schema db = session->getSchema(DB_NAME);

        // Создание таблицы пользователей
        session->sql(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "username VARCHAR(50) UNIQUE NOT NULL, "
            "email VARCHAR(100) UNIQUE NOT NULL, "
            "password_hash VARCHAR(64) NOT NULL, "
            "role ENUM('user', 'admin') DEFAULT 'user', "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        ).execute();

        // Создание таблицы истории сортировок
        session->sql(
            "CREATE TABLE IF NOT EXISTS sorting_history ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "user_id INT, "
            "input_array TEXT NOT NULL, "
            "output_array TEXT NOT NULL, "
            "algorithm VARCHAR(50) DEFAULT 'bucket_sort', "
            "execution_time_ms INT, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
            ")"
        ).execute();

        // Создание администратора по умолчанию
        std::string admin_hash = hashPassword("admin123");
        try {
            session->sql(
                "INSERT IGNORE INTO users (username, email, password_hash, role) "
                "VALUES ('admin', 'admin@sorting.com', ?, 'admin')"
            ).bind(admin_hash).execute();
        }
        catch (...) {
            // Администратор уже существует
        }

        std::cout << "Database initialized successfully!" << std::endl;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Database initialization error: " << e.what() << std::endl;
    }
}

bool DatabaseHandler::registerUser(const std::string& username, const std::string& email,
    const std::string& password, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
        // Проверка существования пользователя
        auto result = session->sql(
            "SELECT id FROM users WHERE username = ? OR email = ?"
        ).bind(username).bind(email).execute();

        if (result.count() > 0) {
            error_msg = "Username or email already exists";
            return false;
        }

        // Хеширование пароля и сохранение
        std::string password_hash = hashPassword(password);
        session->sql(
            "INSERT INTO users (username, email, password_hash) "
            "VALUES (?, ?, ?)"
        ).bind(username).bind(email).bind(password_hash).execute();

        return true;
    }
    catch (const mysqlx::Error& e) {
        error_msg = e.what();
        return false;
    }
}

bool DatabaseHandler::loginUser(const std::string& username, const std::string& password,
    struct User& user, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
        auto result = session->sql(
            "SELECT id, username, email, role, created_at "
            "FROM users WHERE username = ? AND password_hash = ?"
        ).bind(username).bind(hashPassword(password)).execute();

        if (result.count() == 0) {
            error_msg = "Invalid username or password";
            return false;
        }

        mysqlx::Row row = result.fetchOne();
        user.id = row[0];
        user.username = std::string(row[1]);
        user.email = std::string(row[2]);
        user.role = std::string(row[3]);
        user.created_at = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        return true;
    }
    catch (const mysqlx::Error& e) {
        error_msg = e.what();
        return false;
    }
}

bool DatabaseHandler::saveSortingResult(int user_id, const std::vector<int>& input,
    const std::vector<int>& output, long long exec_time) {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
        // Преобразование массивов в строку
        std::stringstream input_ss, output_ss;
        for (size_t i = 0; i < input.size(); i++) {
            input_ss << input[i];
            output_ss << output[i];
            if (i < input.size() - 1) {
                input_ss << ",";
                output_ss << ",";
            }
        }

        session->sql(
            "INSERT INTO sorting_history "
            "(user_id, input_array, output_array, execution_time_ms) "
            "VALUES (?, ?, ?, ?)"
        ).bind(user_id).bind(input_ss.str())
            .bind(output_ss.str()).bind((int)exec_time).execute();

        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Error saving sorting result: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::vector<std::string>> DatabaseHandler::getSortingHistory(int user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::vector<std::string>> history;

    try {
        auto result = session->sql(
            "SELECT input_array, output_array, execution_time_ms, created_at "
            "FROM sorting_history WHERE user_id = ? ORDER BY created_at DESC LIMIT 10"
        ).bind(user_id).execute();

        for (mysqlx::Row row : result) {
            std::vector<std::string> record;
            record.push_back(std::string(row[0])); // input_array
            record.push_back(std::string(row[1])); // output_array
            record.push_back(std::to_string(row[2])); // execution_time_ms
            record.push_back(std::string(row[3])); // created_at
            history.push_back(record);
        }
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Error fetching history: " << e.what() << std::endl;
    }

    return history;
}