#include "sort_test.h"
#include "bucket_sort.h"
#include <iostream>
#include <algorithm>

void runSortTest(std::vector<int> arr) {
    std::cout << "=== Тест сортировки ===\n";
    std::cout << "Исходный массив: ";
    for (int n : arr) std::cout << n << " ";
    std::cout << "\n";

    bucketSort(arr);

    std::cout << "Отсортированный массив: ";
    for (int n : arr) std::cout << n << " ";
    std::cout << "\n";

    if (std::is_sorted(arr.begin(), arr.end()))
        std::cout << "Тест пройден успешно!\n";
    else
        std::cout << "Ошибка сортировки!\n";
}
