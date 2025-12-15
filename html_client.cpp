#include "html_client.h"
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

void sendToWeb(
    const std::string& inputArray,
    const std::string& outputArray,
    int executionTime
) {
    HINTERNET hSession = WinHttpOpen(
        L"SortingClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"localhost",
        8080,
        0);

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/save",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    std::string json =
        "{"
        "\"input\":\"" + inputArray + "\","
        "\"output\":\"" + outputArray + "\","
        "\"time\":" + std::to_string(executionTime) +
        "}";

    WinHttpSendRequest(
        hRequest,
        L"Content-Type: application/json\r\n",
        -1,
        (LPVOID)json.c_str(),
        json.length(),
        json.length(),
        0);

    WinHttpReceiveResponse(hRequest, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}
