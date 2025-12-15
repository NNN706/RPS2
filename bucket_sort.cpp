#include "bucket_sort.h"
#include <algorithm>

void bucketSort(std::vector<int>& arr) {
    if (arr.empty()) return;

    int minValue = *std::min_element(arr.begin(), arr.end());
    int maxValue = *std::max_element(arr.begin(), arr.end());
    int bucketCount = arr.size();

    std::vector<std::vector<int>> buckets(bucketCount);

    for (int num : arr) {
        int idx = (num - minValue) * (bucketCount - 1) / (maxValue - minValue);
        buckets[idx].push_back(num);
    }

    arr.clear();
    for (auto& bucket : buckets) {
        std::sort(bucket.begin(), bucket.end());
        arr.insert(arr.end(), bucket.begin(), bucket.end());
    }
}
