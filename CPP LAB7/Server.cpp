#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string>

#define ID_PIPE_START 32771
#define ID_PIPE_STOP  32772
#define IDM_EXIT      107
#define WM_USER_NEW_DATA (WM_USER + 1)

HINSTANCE hInst;
static HWND hwndEdit;
char mess[4096] = "";
char* m_mess = mess;

HANDLE hThread = NULL;
BOOL bRunning = FALSE;

// Фоновий потік для прослуховування каналу (Pipe)
DWORD WINAPI PipeServerThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;

    while (bRunning) {
        // Створюємо екземпляр іменованого каналу
        HANDLE hPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\LOCAL\\MyLab7Pipe",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            1024, 1024, 0, NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(100);
            continue;
        }

        // Очікуємо підключення клієнта (блокуючий виклик)
        BOOL bConnected = ConnectNamedPipe(hPipe, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (bConnected && bRunning) {
            char buffer[1024] = { 0 };
            DWORD dwRead;
            // Читаємо дані з каналу
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL)) {
                buffer[dwRead] = '\0';

                // Динамічно виділяємо пам'ять для передачі рядка в UI-потік
                char* msg = new char[1024];
                strcpy_s(msg, 1024, buffer);
                PostMessageA(hWnd, WM_USER_NEW_DATA, (WPARAM)msg, 0);
            }
        }

        // Відключаємо клієнта та закриваємо екземпляр каналу
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    return 0;
}

HMENU CreateServerMenu() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hPipeMenu = CreatePopupMenu();

    AppendMenuA(hFileMenu, MF_STRING, IDM_EXIT, "Exit");
    AppendMenuA(hPipeMenu, MF_STRING, ID_PIPE_START, "Start Pipe Server");
    AppendMenuA(hPipeMenu, MF_STRING, ID_PIPE_STOP, "Stop Pipe Server");

    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hPipeMenu, "Named Pipe");

    return hMenuBar;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        strcpy_s(mess, sizeof(mess), "Лабораторна робота - Канали обміну даними (Pipes).\r\n");
        hwndEdit = CreateWindowA("EDIT", mess,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 400, 460, hWnd, NULL, hInst, NULL);
        break;

    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_PIPE_START:
            if (bRunning) {
                MessageBoxA(hWnd, "Сервер Named Pipe вже запущено!", "Info", MB_OK | MB_ICONINFORMATION);
                break;
            }

            bRunning = TRUE;
            hThread = CreateThread(NULL, 0, PipeServerThread, hWnd, 0, NULL);

            strcat_s(mess, sizeof(mess), "\r\nСервер Named Pipe запущено. Очікування підключень...\r\n");
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
            break;

        case ID_PIPE_STOP:
            if (bRunning) {
                bRunning = FALSE;

                // Підключаємо фіктивного клієнта, щоб розблокувати ConnectNamedPipe
                HANDLE hDummy = CreateFileA("\\\\.\\pipe\\LOCAL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if (hDummy != INVALID_HANDLE_VALUE) {
                    CloseHandle(hDummy);
                }

                if (hThread) {
                    WaitForSingleObject(hThread, 1000);
                    CloseHandle(hThread);
                    hThread = NULL;
                }

                strcat_s(mess, sizeof(mess), "\r\nСервер Named Pipe зупинено\r\n");
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
            }
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
    } break;

    case WM_USER_NEW_DATA: {
        // Отримання даних від фонового потоку
        char* receivedMsg = (char*)wParam;
        strcat_s(mess, sizeof(mess), "\r\nОтримано повідомлення:\r\n");
        strcat_s(mess, sizeof(mess), receivedMsg);
        strcat_s(mess, sizeof(mess), "\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);

        delete[] receivedMsg; // Звільняємо пам'ять
    } break;

    case WM_DESTROY:
        if (bRunning) {
            bRunning = FALSE;
            HANDLE hDummy = CreateFileA("\\\\.\\pipe\\LOCAL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hDummy != INVALID_HANDLE_VALUE) CloseHandle(hDummy);
            if (hThread) { WaitForSingleObject(hThread, 1000); CloseHandle(hThread); }
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    const char* szClassName = "PipeServerClass";
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, szClassName, LoadIcon(NULL, IDI_APPLICATION) };
    if (!RegisterClassExA(&wcex)) return FALSE;
    HWND hWnd = CreateWindowA(szClassName, "Server - Named Pipes (Variant 8)", WS_OVERLAPPEDWINDOW, 100, 100, 460, 560, NULL, CreateServerMenu(), hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}