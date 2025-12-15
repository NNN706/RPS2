#include "bucket_sort.h"
#include "database.h"
#include <cassert>
#include <vector>
#include <iostream>

void testBucketSort() {
    std::cout << "Запуск теста блочной сортировки...\n";

    std::vector<int> arr = { 64, 34, 25, 12, 22, 11, 90 };
    std::vector<int> expected = { 11, 12, 22, 25, 34, 64, 90 };

    bucketSort(arr);

    bool passed = (arr == expected);
    assert(passed);

    if (passed) {
        std::cout << "? Тест блочной сортировки пройден успешно!\n";
    }
    else {
        std::cout << "? Тест блочной сортировки провален!\n";
    }
}

void testDatabaseConnection() {
    std::cout << "Запуск теста подключения к базе данных...\n";

    bool connected = checkDatabaseConnection();

    if (connected) {
        std::cout << "? Подключение к SQL Server установлено!\n";
    }
    else {
        std::cout << "? Не удалось подключиться к SQL Server!\n";
        std::cout << "  Убедитесь, что:\n";
        std::cout << "  1. SQL Server запущен\n";
        std::cout << "  2. База данных SortingDB существует\n";
        std::cout << "  3. ODBC Driver 17 for SQL Server установлен\n";
    }
}

void testArrayValidation() {
    std::cout << "Запуск теста валидации массивов...\n";

    std::vector<int> validArray = { 1, 2, 3, 4, 5 };
    std::vector<int> emptyArray = {};
    std::vector<int> tooLargeArray(1001, 1); // 1001 элемент
    std::vector<int> invalidValues = { 9999999, -9999999 };

    assert(validateArray(validArray) == true);
    assert(validateArray(emptyArray) == false);
    assert(validateArray(tooLargeArray) == false);
    assert(validateArray(invalidValues) == false);

    std::cout << "? Тест валидации массивов пройден успешно!\n";
}

void testSaveToDatabase() {
    std::cout << "Запуск теста сохранения в базу данных...\n";

    std::vector<int> testArray = { 5, 3, 8, 1, 2 };
    std::vector<int> sortedArray = { 1, 2, 3, 5, 8 };

    bool saved = saveSortingHistory(1, testArray, sortedArray);

    if (saved) {
        std::cout << "? Тест сохранения в базу данных пройден успешно!\n";
    }
    else {
        std::cout << "? Тест сохранения в базу данных провален!\n";
        std::cout << "  Это может быть нормально, если:\n";
        std::cout << "  1. Нет подключения к БД\n";
        std::cout << "  2. Таблица SortingHistory не существует\n";
    }
}

void runAllTests() {
    std::cout << "=========================================\n";
    std::cout << "ЗАПУСК ВСЕХ ТЕСТОВ СИСТЕМЫ СОРТИРОВКИ\n";
    std::cout << "=========================================\n\n";

    try {
        testBucketSort();
        std::cout << std::endl;

        testDatabaseConnection();
        std::cout << std::endl;

        testArrayValidation();
        std::cout << std::endl;

        testSaveToDatabase();
        std::cout << std::endl;

        std::cout << "=========================================\n";
        std::cout << "ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ!\n";
        std::cout << "=========================================\n";

    }
    catch (const std::exception& e) {
        std::cerr << "\n? Ошибка при выполнении тестов: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "\n? Неизвестная ошибка при выполнении тестов!" << std::endl;
    }
}

int main() {
    runAllTests();

    // Ждем нажатия Enter перед выходом
    std::cout << "\nНажмите Enter для выхода...";
    std::cin.get();

    return 0;
}