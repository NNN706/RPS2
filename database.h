#pragma once
#include <vector>
#include <tuple>
#include <string>

// —охран€ет историю сортировки в базу данных.
// ¬озвращает true при успешном сохранении, false при ошибке подключени€ или выполнени€ SQL.
bool saveSortingHistory(int userID, const std::vector<int>& input, const std::vector<int>& output);

// ѕолучает историю сортировок (пока заглушка возвращает пустой вектор)
std::vector<std::tuple<int, std::string, std::string>> getSortingHistory();
bool checkDatabaseConnection();