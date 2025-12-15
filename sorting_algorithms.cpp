#include "sorting_algorithms.h"
#include <algorithm>
#include <vector>

std::vector<int> bucketSort(const std::vector<int>& arr) {
    if (arr.empty()) return {};

    int min_val = *std::min_element(arr.begin(), arr.end());
    int max_val = *std::max_element(arr.begin(), arr.end());

    int bucket_count = arr.size();
    std::vector<std::vector<int>> buckets(bucket_count);

    float range = max_val - min_val + 1;
    for (int num : arr) {
        int bucket_index = ((num - min_val) / range) * (bucket_count - 1);
        buckets[bucket_index].push_back(num);
    }

    for (auto& bucket : buckets) {
        std::sort(bucket.begin(), bucket.end());
    }

    std::vector<int> sorted_arr;
    for (const auto& bucket : buckets) {
        sorted_arr.insert(sorted_arr.end(), bucket.begin(), bucket.end());
    }

    return sorted_arr;
}

// Дополнительная оптимизированная версия (если нужно)
std::vector<int> optimizedBucketSort(const std::vector<int>& arr) {
    // Реализация оптимизированной версии
    return bucketSort(arr); // временно возвращаем базовую версию
}