#include "web_server.h"
#include <fstream>
#include <cstdlib>
#include <iostream>

WebServer::WebServer(int port) : port(port) {}

void WebServer::start() {
    std::string command = "python3 -m http.server " + std::to_string(port) +
        " --directory ../web_interface &";
    system(command.c_str());
    std::cout << "Web server started on port " << port << std::endl;
}

void WebServer::stop() {
    // Команда для остановки веб-сервера (зависит от ОС)
#ifdef _WIN32
    system("taskkill /F /IM python.exe");
#else
    system("pkill -f \"http.server\"");
#endif
}

void WebServer::generateHelpPage() {
    std::ofstream help_file("web_interface/help.html");
    help_file << R"(
<!DOCTYPE html>
<html>
<head>
    <title>Справка - Система сортировки</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        h1, h2 { color: #333; }
        .section { margin-bottom: 30px; }
        code { background: #f4f4f4; padding: 2px 5px; }
    </style>
</head>
<body>
    <h1>📚 Справка по системе сортировки</h1>
    
    <div class="section">
        <h2>1. Блочная сортировка (Bucket Sort)</h2>
        <p><strong>Принцип работы:</strong></p>
        <ol>
            <li>Создаются "ведра" (buckets) для диапазонов значений</li>
            <li>Элементы распределяются по соответствующим ведрам</li>
            <li>Каждое ведро сортируется отдельно</li>
            <li>Отсортированные ведра объединяются</li>
        </ol>
        <p><strong>Сложность:</strong></p>
        <ul>
            <li>Лучший случай: O(n + k)</li>
            <li>Средний случай: O(n + k)</li>
            <li>Худший случай: O(n²)</li>
        </ul>
    </div>
    
    <div class="section">
        <h2>2. Регистрация и вход</h2>
        <p>Для использования системы требуется регистрация:</p>
        <ul>
            <li>Перейдите на страницу регистрации</li>
            <li>Заполните форму (имя пользователя, email, пароль)</li>
            <li>После регистрации выполните вход</li>
        </ul>
    </div>
    
    <div class="section">
        <h2>3. Использование сортировки</h2>
        <p>После входа в систему:</p>
        <ol>
            <li>Введите числа через запятую в поле ввода</li>
            <li>Нажмите кнопку "Сортировать"</li>
            <li>Результат сохранится в базе данных</li>
            <li>Просмотрите историю сортировок</li>
        </ol>
    </div>
</body>
</html>
    )";
    help_file.close();
}
