#include "database.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>

#pragma comment(lib, "odbc32.lib")

// ================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =================

// Функция для преобразования UTF-8 в UTF-16 (wstring)
std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (size_needed <= 0) return L"";

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

// Функция для преобразования UTF-16 в UTF-8
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";

    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// Функция для очистки ODBC ресурсов
void cleanupODBC(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt = SQL_NULL_HSTMT) {
    if (hstmt != SQL_NULL_HSTMT) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
    if (hdbc != SQL_NULL_HDBC) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }
    if (henv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}

// ================= ОСНОВНЫЕ ФУНКЦИИ БД =================

bool checkDatabaseConnection() {
    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLRETURN retcode;

    // 1. Allocate environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка выделения окружения ODBC" << std::endl;
        return false;
    }

    // 2. Set the ODBC version environment attribute
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка установки версии ODBC" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // 3. Allocate connection handle
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка выделения соединения ODBC" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // 4. Set login timeout to 5 seconds
    SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

    // 5. Connect to SQL Server
    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;"
        L"Encrypt=no;";

    SQLWCHAR outstr[1024];
    SQLSMALLINT outstrlen;

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS,
        outstr, sizeof(outstr) / sizeof(outstr[0]),
        &outstrlen, SQL_DRIVER_NOPROMPT);

    bool connected = (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);

    if (!connected) {
        SQLWCHAR sqlstate[6];
        SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER native_error;
        SQLSMALLINT length;

        SQLGetDiagRecW(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &native_error,
            message, SQL_MAX_MESSAGE_LENGTH, &length);

        // Преобразование wide char в char для вывода
        char buffer[SQL_MAX_MESSAGE_LENGTH];
        WideCharToMultiByte(CP_UTF8, 0, message, -1, buffer, sizeof(buffer), NULL, NULL);

        std::cerr << "Ошибка подключения к SQL Server: " << buffer << std::endl;
    }

    // Cleanup
    cleanupODBC(henv, hdbc);

    return connected;
}

bool saveSortingHistory(int userID,
    const std::vector<int>& originalArray,
    const std::vector<int>& sortedArray) {

    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    bool success = false;

    // 1. Allocate environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка выделения окружения ODBC" << std::endl;
        return false;
    }

    // 2. Set the ODBC version
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка установки версии ODBC" << std::endl;
        cleanupODBC(henv, hdbc);
        return false;
    }

    // 3. Allocate connection handle
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка выделения соединения ODBC" << std::endl;
        cleanupODBC(henv, hdbc);
        return false;
    }

    // 4. Connect to SQL Server
    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;"
        L"Encrypt=no;";

    SQLWCHAR outstr[1024];
    SQLSMALLINT outstrlen;

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS,
        outstr, sizeof(outstr) / sizeof(outstr[0]),
        &outstrlen, SQL_DRIVER_NOPROMPT);

    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        SQLWCHAR sqlstate[6];
        SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER native_error;
        SQLSMALLINT length;

        SQLGetDiagRecW(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &native_error,
            message, SQL_MAX_MESSAGE_LENGTH, &length);

        char buffer[SQL_MAX_MESSAGE_LENGTH];
        WideCharToMultiByte(CP_UTF8, 0, message, -1, buffer, sizeof(buffer), NULL, NULL);

        std::cerr << "Ошибка подключения: " << buffer << std::endl;

        cleanupODBC(henv, hdbc);
        return false;
    }

    // 5. Allocate statement handle
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Ошибка выделения statement ODBC" << std::endl;
        cleanupODBC(henv, hdbc);
        return false;
    }

    try {
        // Преобразуем массивы в строки
        std::ostringstream origStream, sortedStream;
        for (size_t i = 0; i < originalArray.size(); ++i) {
            if (i > 0) origStream << ",";
            origStream << originalArray[i];
        }
        for (size_t i = 0; i < sortedArray.size(); ++i) {
            if (i > 0) sortedStream << ",";
            sortedStream << sortedArray[i];
        }

        std::string originalStr = origStream.str();
        std::string sortedStr = sortedStream.str();

        // Преобразуем в UTF-16 для SQL Server
        std::wstring wOriginalStr = utf8_to_wstring(originalStr);
        std::wstring wSortedStr = utf8_to_wstring(sortedStr);

        // Подготовка SQL запроса
        std::wstring sql = L"INSERT INTO SortingHistory (user_id, original_array, sorted_array) VALUES (?, ?, ?)";

        retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            throw std::runtime_error("Ошибка подготовки SQL запроса");
        }

        // Привязка параметров
        SQLINTEGER userIDParam = userID;
        retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &userIDParam, 0, NULL);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            throw std::runtime_error("Ошибка привязки параметра user_id");
        }

        // Привязка строковых параметров как Unicode (SQL_C_WCHAR)
        retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wOriginalStr.length() * 2, 0, (SQLPOINTER)wOriginalStr.c_str(),
            wOriginalStr.length() * 2, NULL);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            throw std::runtime_error("Ошибка привязки параметра original_array");
        }

        retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wSortedStr.length() * 2, 0, (SQLPOINTER)wSortedStr.c_str(),
            wSortedStr.length() * 2, NULL);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            throw std::runtime_error("Ошибка привязки параметра sorted_array");
        }

        // Выполнение запроса
        retcode = SQLExecute(hstmt);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            SQLWCHAR sqlstate[6];
            SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH];
            SQLINTEGER native_error;
            SQLSMALLINT length;

            SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &native_error,
                message, SQL_MAX_MESSAGE_LENGTH, &length);

            char buffer[SQL_MAX_MESSAGE_LENGTH];
            WideCharToMultiByte(CP_UTF8, 0, message, -1, buffer, sizeof(buffer), NULL, NULL);

            throw std::runtime_error(std::string("Ошибка выполнения SQL: ") + buffer);
        }

        std::cout << "Данные успешно сохранены в SQL Server" << std::endl;
        success = true;

    }
    catch (const std::exception& e) {
        std::cerr << "Исключение при сохранении в SQL: " << e.what() << std::endl;
        success = false;
    }

    // Cleanup
    cleanupODBC(henv, hdbc, hstmt);

    return success;
}

// ================= ФУНКЦИИ ДЛЯ РАБОТЫ С ПОЛЬЗОВАТЕЛЯМИ =================

bool registerUserInDB(const std::string& username, const std::string& password_hash) {
    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    bool success = false;

    // Инициализация ODBC
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS) return false;

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    // Подключение к БД
    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;";

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    // Выделение statement
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    try {
        std::wstring sql = L"INSERT INTO Users (username, password_hash) VALUES (?, ?)";
        retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (retcode != SQL_SUCCESS) throw std::runtime_error("Ошибка подготовки SQL");

        std::wstring wUsername = utf8_to_wstring(username);
        std::wstring wPasswordHash = utf8_to_wstring(password_hash);

        SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wUsername.length() * 2, 0, (SQLPOINTER)wUsername.c_str(),
            wUsername.length() * 2, NULL);

        SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wPasswordHash.length() * 2, 0, (SQLPOINTER)wPasswordHash.c_str(),
            wPasswordHash.length() * 2, NULL);

        retcode = SQLExecute(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            success = true;
        }

    }
    catch (...) {
        success = false;
    }

    cleanupODBC(henv, hdbc, hstmt);
    return success;
}

bool authenticateUser(const std::string& username, const std::string& password_hash) {
    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    bool authenticated = false;

    // Инициализация ODBC (аналогично registerUserInDB)
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS) return false;

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;";

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return false;
    }

    try {
        std::wstring sql = L"SELECT id FROM Users WHERE username = ? AND password_hash = ?";
        retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (retcode != SQL_SUCCESS) throw std::runtime_error("Ошибка подготовки SQL");

        std::wstring wUsername = utf8_to_wstring(username);
        std::wstring wPasswordHash = utf8_to_wstring(password_hash);

        SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wUsername.length() * 2, 0, (SQLPOINTER)wUsername.c_str(),
            wUsername.length() * 2, NULL);

        SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
            wPasswordHash.length() * 2, 0, (SQLPOINTER)wPasswordHash.c_str(),
            wPasswordHash.length() * 2, NULL);

        retcode = SQLExecute(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            // Если есть хотя бы одна строка - пользователь найден
            retcode = SQLFetch(hstmt);
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                authenticated = true;
            }
        }

    }
    catch (...) {
        authenticated = false;
    }

    cleanupODBC(henv, hdbc, hstmt);
    return authenticated;
}

std::vector<SortingRecord> getUserHistory(int userID) {
    std::vector<SortingRecord> history;

    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    // Инициализация ODBC
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS) return history;

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return history;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return history;
    }

    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;";

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return history;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS) {
        cleanupODBC(henv, hdbc);
        return history;
    }

    try {
        std::wstring sql = L"SELECT id, user_id, original_array, sorted_array, CONVERT(varchar, timestamp, 120) FROM SortingHistory WHERE user_id = ? ORDER BY timestamp DESC";
        retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (retcode != SQL_SUCCESS) throw std::runtime_error("Ошибка подготовки SQL");

        SQLINTEGER paramUserID = userID;
        SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &paramUserID, 0, NULL);

        retcode = SQLExecute(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                SortingRecord record;
                SQLINTEGER id, fetchedUserID;
                SQLWCHAR originalArray[1024], sortedArray[1024], timestamp[50];
                SQLLEN id_ind, user_id_ind, orig_ind, sorted_ind, time_ind;

                SQLGetData(hstmt, 1, SQL_C_SLONG, &id, 0, &id_ind);
                SQLGetData(hstmt, 2, SQL_C_SLONG, &fetchedUserID, 0, &user_id_ind);
                SQLGetData(hstmt, 3, SQL_C_WCHAR, originalArray, sizeof(originalArray), &orig_ind);
                SQLGetData(hstmt, 4, SQL_C_WCHAR, sortedArray, sizeof(sortedArray), &sorted_ind);
                SQLGetData(hstmt, 5, SQL_C_WCHAR, timestamp, sizeof(timestamp), &time_ind);

                if (id_ind != SQL_NULL_DATA) record.id = id;
                if (user_id_ind != SQL_NULL_DATA) record.user_id = fetchedUserID;
                if (orig_ind != SQL_NULL_DATA) record.original_array = wstring_to_utf8(originalArray);
                if (sorted_ind != SQL_NULL_DATA) record.sorted_array = wstring_to_utf8(sortedArray);
                if (time_ind != SQL_NULL_DATA) record.timestamp = wstring_to_utf8(timestamp);

                history.push_back(record);
            }
        }

    }
    catch (...) {
        // В случае ошибки возвращаем пустую историю
    }

    cleanupODBC(henv, hdbc, hstmt);
    return history;
}

// ================= ВАЛИДАЦИЯ ДАННЫХ =================

bool validateArray(const std::vector<int>& arr) {
    if (arr.empty() || arr.size() > 1000) return false;
    for (int num : arr) {
        if (num < -1000000 || num > 1000000) return false;
    }
    return true;
}