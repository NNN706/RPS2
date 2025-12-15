#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <locale>
#include <sstream>
#include <thread>
#include <fstream>
#include <map>

#include "bucket_sort.h"
#include "database.h"
#include "user.h"
#include "html_server.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

/* ================= ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ДЛЯ ПОЛЬЗОВАТЕЛЕЙ ================= */
map<string, string> users; // username -> password
string currentUser;

/* ================= HTTP SERVER С ПОДДЕРЖКОЙ РЕГИСТРАЦИИ ================= */
void runHttpServer(unsigned short port)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed!\n";
        return;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        cerr << "Не удалось создать сокет!\n";
        WSACleanup();
        return;
    }

    // Разрешаем переиспользование порта
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Bind failed! Порт " << port << " занят.\n";
        closesocket(server);
        WSACleanup();
        return;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed!\n";
        closesocket(server);
        WSACleanup();
        return;
    }

    cout << "HTTP сервер запущен: http://localhost:" << port << endl;

    while (true)
    {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        char buffer[8192]{};
        int bytesReceived = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) { closesocket(client); continue; }

        buffer[bytesReceived] = '\0';
        string request(buffer);

        // Определяем метод и путь
        string method, path, body;
        size_t methodEnd = request.find(' ');
        if (methodEnd != string::npos) {
            method = request.substr(0, methodEnd);
            size_t pathEnd = request.find(' ', methodEnd + 1);
            if (pathEnd != string::npos) {
                path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
            }
        }

        // Парсим тело запроса для POST
        if (method == "POST") {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != string::npos) {
                body = request.substr(bodyPos + 4);
            }
        }

        string response;

        // === ОБРАБОТКА РАЗНЫХ ПУТЕЙ ===

        // Главная страница
        if (path == "/" || path == "/index.html") {
            ifstream file("index.html", ios::binary);
            string htmlContent;
            if (file) {
                htmlContent.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            }
            else {
                htmlContent = "<h1>Файл не найден</h1>";
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: " + to_string(htmlContent.size()) + "\r\n\r\n" + htmlContent;
        }
        // CSS файл
        else if (path == "/style.css") {
            ifstream file("style.css", ios::binary);
            string cssContent;
            if (file) {
                cssContent.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/css\r\n"
                "Content-Length: " + to_string(cssContent.size()) + "\r\n\r\n" + cssContent;
        }
        // Регистрация
        else if (path == "/register" && method == "POST") {
            // Парсим JSON: {"username":"...","password":"..."}
            string username, password;
            size_t userPos = body.find("\"username\":\"");
            if (userPos != string::npos) {
                userPos += 12;
                size_t userEnd = body.find("\"", userPos);
                if (userEnd != string::npos) username = body.substr(userPos, userEnd - userPos);
            }

            size_t passPos = body.find("\"password\":\"");
            if (passPos != string::npos) {
                passPos += 12;
                size_t passEnd = body.find("\"", passPos);
                if (passEnd != string::npos) password = body.substr(passPos, passEnd - passPos);
            }

            string jsonResponse;
            if (username.empty() || password.empty()) {
                jsonResponse = R"({"status":"error","message":"Неверные данные"})";
            }
            else if (users.find(username) != users.end()) {
                jsonResponse = R"({"status":"error","message":"Пользователь уже существует"})";
            }
            else {
                users[username] = password;
                currentUser = username;
                cout << "Зарегистрирован новый пользователь: " << username << endl;
                jsonResponse = R"({"status":"success","message":"Регистрация успешна","username":")" + username + R"("})";
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: " + to_string(jsonResponse.size()) + "\r\n\r\n" + jsonResponse;
        }
        // Вход
        else if (path == "/login" && method == "POST") {
            string username, password;
            size_t userPos = body.find("\"username\":\"");
            if (userPos != string::npos) {
                userPos += 12;
                size_t userEnd = body.find("\"", userPos);
                if (userEnd != string::npos) username = body.substr(userPos, userEnd - userPos);
            }

            size_t passPos = body.find("\"password\":\"");
            if (passPos != string::npos) {
                passPos += 12;
                size_t passEnd = body.find("\"", passPos);
                if (passEnd != string::npos) password = body.substr(passPos, passEnd - passPos);
            }

            string jsonResponse;
            if (users.find(username) != users.end() && users[username] == password) {
                currentUser = username;
                cout << "Пользователь вошел: " << username << endl;
                jsonResponse = R"({"status":"success","message":"Вход успешен","username":")" + username + R"("})";
            }
            else {
                jsonResponse = R"({"status":"error","message":"Неверный логин или пароль"})";
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: " + to_string(jsonResponse.size()) + "\r\n\r\n" + jsonResponse;
        }
        // Сохранение в SQL
        else if (path == "/save" && method == "POST") {
            string jsonResponse;

            if (currentUser.empty()) {
                jsonResponse = R"({"status":"error","message":"Требуется вход в систему"})";
            }
            else {
                // Парсим массив из запроса
                string arrayStr;
                size_t arrPos = body.find("\"array\":\"");
                if (arrPos != string::npos) {
                    arrPos += 9;
                    size_t arrEnd = body.find("\"", arrPos);
                    if (arrEnd != string::npos) arrayStr = body.substr(arrPos, arrEnd - arrPos);
                }

                if (!arrayStr.empty()) {
                    // Преобразуем строку в вектор
                    vector<int> arr;
                    stringstream ss(arrayStr);
                    string token;

                    while (getline(ss, token, ',')) {
                        try {
                            arr.push_back(stoi(token));
                        }
                        catch (...) {}
                    }

                    if (!arr.empty()) {
                        int userID = getCurrentUserID();
                        bool success = saveSortingHistory(userID, arr, arr);

                        if (success) {
                            cout << "Данные сохранены в SQL для пользователя: " << currentUser << endl;
                            jsonResponse = R"({"status":"success","message":"Данные сохранены в SQL Server"})";
                        }
                        else {
                            jsonResponse = R"({"status":"error","message":"Ошибка сохранения в SQL"})";
                        }
                    }
                    else {
                        jsonResponse = R"({"status":"error","message":"Пустой массив"})";
                    }
                }
                else {
                    jsonResponse = R"({"status":"error","message":"Нет данных массива"})";
                }
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: " + to_string(jsonResponse.size()) + "\r\n\r\n" + jsonResponse;
        }
        // Получение текущего пользователя
        else if (path == "/current-user" && method == "GET") {
            string jsonResponse;
            if (currentUser.empty()) {
                jsonResponse = R"({"status":"error","message":"Не авторизован"})";
            }
            else {
                jsonResponse = R"({"status":"success","username":")" + currentUser + R"("})";
            }

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: " + to_string(jsonResponse.size()) + "\r\n\r\n" + jsonResponse;
        }
        // Выход
        else if (path == "/logout" && method == "POST") {
            if (!currentUser.empty()) {
                cout << "Пользователь вышел: " << currentUser << endl;
            }
            currentUser.clear();

            string jsonResponse = R"({"status":"success","message":"Выход выполнен"})";
            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: " + to_string(jsonResponse.size()) + "\r\n\r\n" + jsonResponse;
        }
        // 404 - не найден
        else {
            string errorContent = "<h1>404 - Страница не найдена</h1><p>" + path + "</p>";
            response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: " + to_string(errorContent.size()) + "\r\n\r\n" + errorContent;
        }

        send(client, response.c_str(), (int)response.size(), 0);
        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
}

/* ================= ОБНОВЛЕННАЯ ФУНКЦИЯ СОЗДАНИЯ HTML С РЕГИСТРАЦИЕЙ ================= */
bool createHtmlWithAuth(const vector<int>& currentArray) {
    // Создаем CSS
    ofstream css("style.css");
    if (!css.is_open()) return false;

    css <<
        "body{font-family:Arial,sans-serif;background:#f5f9fc;margin:0;padding:20px}\n"
        ".container{max-width:800px;margin:auto;background:white;padding:30px;border-radius:15px;box-shadow:0 5px 15px rgba(0,0,0,0.1)}\n"
        "header{background:#4a6fa5;color:white;padding:20px;border-radius:10px 10px 0 0;margin:-30px -30px 20px}\n"
        "h1{margin:0}\n"
        ".auth-box{background:#e8f4fc;padding:15px;border-radius:8px;margin-bottom:20px}\n"
        "input{margin:5px;padding:8px;width:200px;border:1px solid #ccc;border-radius:4px}\n"
        "button{margin:5px;padding:10px 15px;border:none;border-radius:6px;cursor:pointer;font-weight:bold}\n"
        ".btn-primary{background:#4a6fa5;color:white}\n"
        ".btn-success{background:#28a745;color:white}\n"
        ".btn-warning{background:#ffc107;color:black}\n"
        ".btn-danger{background:#dc3545;color:white}\n"
        ".array-box{background:#f8f9fa;padding:15px;border-radius:8px;margin:10px 0}\n"
        "#user-info{margin:10px 0;font-weight:bold;color:#4a6fa5}\n"
        ".toast{position:fixed;bottom:20px;right:20px;background:#333;color:white;padding:12px;border-radius:6px;opacity:0;transition:0.3s}\n"
        ".toast.show{opacity:1}\n";

    css.close();

    // Преобразуем массив в строку
    ostringstream arrStr;
    for (size_t i = 0; i < currentArray.size(); ++i) {
        if (i) arrStr << ",";
        arrStr << currentArray[i];
    }

    // Создаем HTML с формой регистрации/входа
    ofstream html("index.html", ios::binary);
    if (!html.is_open()) return false;

    string htmlContent =
        u8"<!DOCTYPE html><html lang='ru'><head>"
        u8"<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
        u8"<title>Система блочной сортировки</title>"
        u8"<link rel='stylesheet' href='style.css'></head><body>"

        u8"<div class='container'>"
        u8"<header><h1>Система блочной сортировки</h1></header>"

        // Блок авторизации
        u8"<div class='auth-box' id='auth-box'>"
        u8"<h3>Регистрация / Вход</h3>"
        u8"<input id='username' placeholder='Имя пользователя'>"
        u8"<input id='password' type='password' placeholder='Пароль'>"
        u8"<button class='btn-primary' onclick='register()'>Регистрация</button>"
        u8"<button class='btn-success' onclick='login()'>Вход</button>"
        u8"</div>"

        // Информация о пользователе
        u8"<div id='user-info'></div>"

        // Работа с массивом (только для авторизованных)
        u8"<div id='main-content' style='display:none'>"
        u8"<h3>Работа с массивом</h3>"
        u8"<input id='manual-input' placeholder='Введите числа через запятую'>"
        u8"<button class='btn-primary' onclick='applyManual()'>Применить</button>"
        u8"<button class='btn-warning' onclick='autoFill()'>Автозаполнение</button>"
        u8"<button class='btn-success' onclick='saveToSQL()'>Сохранить в SQL Server</button>"
        u8"<button class='btn-danger' onclick='logout()'>Выход</button>"

        u8"<div class='array-box'>"
        u8"<strong>Текущий массив:</strong> <span id='current-array'>" + arrStr.str() + u8"</span>"
        u8"</div>"

        u8"<div class='array-box'>"
        u8"<strong>Отсортированный массив:</strong> <span id='sorted-array'>" + arrStr.str() + u8"</span>"
        u8"</div>"
        u8"</div>" // закрываем main-content

        u8"</div>" // закрываем container

        // Уведомления
        u8"<div class='toast' id='toast'></div>"

        // JavaScript
        u8"<script>"
        u8"function toast(msg, type='info'){"
        u8"let t=document.getElementById('toast');"
        u8"t.textContent=msg;t.className='toast show';"
        u8"setTimeout(()=>t.classList.remove('show'),3000)}"

        u8"function sortArray(arr){"
        u8"return arr.slice().sort((a,b)=>a-b)}"

        u8"function updateDisplay(arr){"
        u8"document.getElementById('current-array').textContent=arr.join(',');"
        u8"document.getElementById('sorted-array').textContent=sortArray(arr).join(',');"
        u8"return arr}"

        u8"function applyManual(){"
        u8"let val=document.getElementById('manual-input').value;"
        u8"if(!val)return;"
        u8"let arr=val.split(',').map(x=>parseInt(x.trim())).filter(x=>!isNaN(x));"
        u8"if(arr.length>0){updateDisplay(arr);toast('Массив обновлен')}"
        u8"else toast('Некорректный ввод','error')}"

        u8"function autoFill(){"
        u8"let arr=[];"
        u8"for(let i=0;i<10;i++)arr.push(Math.floor(Math.random()*100));"
        u8"updateDisplay(arr);toast('Сгенерирован случайный массив')}"

        u8"function register(){"
        u8"let user=document.getElementById('username').value;"
        u8"let pass=document.getElementById('password').value;"
        u8"if(!user||!pass){toast('Заполните все поля','error');return}"
        u8"fetch('/register',{"
        u8"method:'POST',headers:{'Content-Type':'application/json'},"
        u8"body:JSON.stringify({username:user,password:pass})})"
        u8".then(r=>r.json()).then(data=>{"
        u8"if(data.status==='success'){"
        u8"document.getElementById('user-info').innerHTML='Пользователь: '+data.username;"
        u8"document.getElementById('auth-box').style.display='none';"
        u8"document.getElementById('main-content').style.display='block';"
        u8"toast('Регистрация успешна!','success');"
        u8"checkUser();}"
        u8"else toast(data.message,'error')})}"

        u8"function login(){"
        u8"let user=document.getElementById('username').value;"
        u8"let pass=document.getElementById('password').value;"
        u8"if(!user||!pass){toast('Заполните все поля','error');return}"
        u8"fetch('/login',{"
        u8"method:'POST',headers:{'Content-Type':'application/json'},"
        u8"body:JSON.stringify({username:user,password:pass})})"
        u8".then(r=>r.json()).then(data=>{"
        u8"if(data.status==='success'){"
        u8"document.getElementById('user-info').innerHTML='Пользователь: '+data.username;"
        u8"document.getElementById('auth-box').style.display='none';"
        u8"document.getElementById('main-content').style.display='block';"
        u8"toast('Вход выполнен','success');"
        u8"checkUser();}"
        u8"else toast(data.message,'error')})}"

        u8"function logout(){"
        u8"fetch('/logout',{method:'POST'})"
        u8".then(()=>{"
        u8"document.getElementById('auth-box').style.display='block';"
        u8"document.getElementById('main-content').style.display='none';"
        u8"document.getElementById('user-info').innerHTML='';"
        u8"toast('Вы вышли из системы')})}"

        u8"function saveToSQL(){"
        u8"let arrText=document.getElementById('sorted-array').textContent;"
        u8"fetch('/save',{"
        u8"method:'POST',headers:{'Content-Type':'application/json'},"
        u8"body:JSON.stringify({array:arrText})})"
        u8".then(r=>r.json()).then(data=>{"
        u8"if(data.status==='success')toast(''+data.message,'success');"
        u8"else toast(''+data.message,'error')})"
        u8".catch(()=>toast('Ошибка соединения','error'))}"

        u8"function checkUser(){"
        u8"fetch('/current-user')"
        u8".then(r=>r.json()).then(data=>{"
        u8"if(data.status==='success'){"
        u8"document.getElementById('user-info').innerHTML='Пользователь: '+data.username;"
        u8"document.getElementById('auth-box').style.display='none';"
        u8"document.getElementById('main-content').style.display='block';}"
        u8"})}"

        u8"// Проверяем при загрузке"
        u8"window.onload=checkUser;"

        u8"</script></body></html>";

    html.write(htmlContent.c_str(), htmlContent.size());
    html.close();

    return true;
}

/* ================= MAIN ================= */
int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // Инициализация тестового пользователя
    users["admin"] = "admin123";
    users["test"] = "test123";

    cout << "======================================\n";
    cout << "Система блочной сортировки с SQL Server\n";
    cout << "======================================\n\n";

    srand((unsigned)time(nullptr));
    vector<int> arr(10);
    for (int& x : arr) x = rand() % 100;

    cout << "Исходный массив: ";
    for (int x : arr) cout << x << " ";
    cout << endl;

    bucketSort(arr);

    cout << "Отсортированный массив: ";
    for (int x : arr) cout << x << " ";
    cout << endl;

    int userID = getCurrentUserID();

    if (!checkDatabaseConnection()) {
        cerr << "Ошибка подключения к SQL Server!\n";
        MessageBoxA(NULL, "Не удалось подключиться к SQL Server!\nПроверьте запущен ли SQL Server.", "Ошибка", MB_ICONERROR);
    }
    else {
        cout << "Подключение к SQL Server установлено\n";

        if (!saveSortingHistory(userID, arr, arr)) {
            cerr << "Ошибка сохранения данных в SQL!\n";
        }
        else {
            cout << "Данные успешно сохранены в SQL Server\n";
        }
    }

    if (!createHtmlWithAuth(arr)) {
        cerr << "Ошибка создания HTML\n";
        return 1;
    }

    cout << "HTML сайт создан\n";

    // Запускаем HTTP сервер в отдельном потоке
    thread serverThread(runHttpServer, (unsigned short)8080); // Явное преобразование типа

    // Используем detach вместо join для фоновой работы
    serverThread.detach();

    Sleep(500); // Даем время серверу запуститься

    // Открываем браузер
    cout << "\nОткрываю браузер...\n";
    ShellExecuteA(nullptr, "open", "http://localhost:8080", nullptr, nullptr, SW_SHOWNORMAL);

    cout << "\n======================================\n";
    cout << "Сервер запущен: http://localhost:8080\n";
    cout << "Тестовые пользователи:\n";
    cout << "  admin / admin123\n";
    cout << "  test / test123\n";
    cout << "Нажмите Enter для выхода...\n";
    cout << "======================================\n";

    cin.get();

    return 0;
}