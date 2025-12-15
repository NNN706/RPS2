#include <iostream>
#include <thread>
#include "sorting_algorithms.h"
#include "database_handler.h"
#include "user_manager.h"
#include "web_server.h"

using namespace std;

void runTests() {
    system("cd tests && ./run_tests");
}

int main() {
    cout << "=== Система сортировки с БД и веб-интерфейсом ===" << endl;

    // Подключение к БД
    DatabaseHandler dbHandler;
    if (!dbHandler.connect()) {
        cerr << "Failed to connect to database!" << endl;
        return 1;
    }

    // Инициализация БД
    dbHandler.initializeDatabase();

    // Менеджер пользователей
    UserManager userManager(&dbHandler);

    // Веб-сервер
    WebServer webServer(8080);
    webServer.generateHelpPage();

    // Запуск веб-сервера в отдельном потоке
    thread web_server_thread(&WebServer::start, &webServer);

    // Основное меню консоли
    while (true) {
        cout << "\n=== Главное меню ===" << endl;
        cout << "1. Запустить веб-интерфейс в браузере" << endl;
        cout << "2. Выполнить тесты сортировки" << endl;
        cout << "3. Показать справку" << endl;
        cout << "4. Выход" << endl;
        cout << "Выберите действие: ";

        int choice;
        cin >> choice;

        switch (choice) {
        case 1: {
            // Открываем браузер с веб-интерфейсом
#ifdef _WIN32
            system("start http://localhost:8080");
#else
            system("xdg-open http://localhost:8080");
#endif
            break;
        }
        case 2: {
            runTests();
            break;
        }
        case 3: {
            // Открываем справку
#ifdef _WIN32
            system("start web_interface/help.html");
#else
            system("xdg-open web_interface/help.html");
#endif
            break;
        }
        case 4: {
            cout << "Выход из программы..." << endl;
            webServer.stop();
            web_server_thread.detach();
            return 0;
        }
        default:
            cout << "Неверный выбор!" << endl;
        }
    }

    return 0;
}