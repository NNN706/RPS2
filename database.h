#pragma once

#include <string>
#include <vector>

// Структура для хранения записи о сортировке
struct SortingRecord {
    int id;
    int user_id;
    std::string original_array;
    std::string sorted_array;
    std::string timestamp;
};

// Функция для преобразования UTF-8 в UTF-16 (wstring)
std::wstring utf8_to_wstring(const std::string& str);

// Основные функции работы с БД
bool checkDatabaseConnection();
bool saveSortingHistory(int userID,
    const std::vector<int>& originalArray,
    const std::vector<int>& sortedArray);

// Функции для работы с пользователями
bool registerUserInDB(const std::string& username, const std::string& password_hash);
bool authenticateUser(const std::string& username, const std::string& password_hash);
std::vector<SortingRecord> getUserHistory(int userID);

// Валидация данных
bool validateArray(const std::vector<int>& arr);