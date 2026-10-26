#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string>

#define ID_CLIENT_SEND    32771
#define ID_CLIENT_DEFAULT 32772
#define IDM_EXIT          203

HINSTANCE hInst;
static HWND hwndEdit;
char szBuf[1024];

HMENU CreateClientMenu() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hClientMenu = CreatePopupMenu();
    AppendMenuA(hFileMenu, MF_STRING, IDM_EXIT, "Exit");
    AppendMenuA(hClientMenu, MF_STRING, ID_CLIENT_SEND, "Send Message (Pipe)");
    AppendMenuA(hClientMenu, MF_STRING, ID_CLIENT_DEFAULT, "Restore Default");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hClientMenu, "Client");
    return hMenuBar;
}

void SetDefaultMetrics(HWND hWnd) {
    // Метрики для 8 варіанту: величина прямокутника подвійного клацання
    int doubleClkWidth = GetSystemMetrics(SM_CXDOUBLECLK);
    int doubleClkHeight = GetSystemMetrics(SM_CYDOUBLECLK);

    char mess[1024];
    sprintf_s(mess, sizeof(mess), "Дані Клієнта #1 (Варіант 8):\r\n- Ширина прямокутника подвійного кліку (SM_CXDOUBLECLK) = %d\r\n- Висота прямокутника подвійного кліку (SM_CYDOUBLECLK) = %d",
        doubleClkWidth, doubleClkHeight);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        hwndEdit = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 0, 0, 400, 200, hWnd, NULL, hInst, NULL);
        SetDefaultMetrics(hWnd);
        break;
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CLIENT_SEND: {
            DWORD cbWritten = SendMessageA(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
            SendMessageA(hwndEdit, WM_GETTEXT, cbWritten + 1, (LPARAM)szBuf);

            // Підключення до Named Pipe
            HANDLE hPipe = CreateFileA(
                "\\\\.\\pipe\\LOCAL\\MyLab7Pipe",
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hPipe != INVALID_HANDLE_VALUE) {
                DWORD dwWritten;
                // Записуємо дані у канал
                WriteFile(hPipe, szBuf, strlen(szBuf) + 1, &dwWritten, NULL);
                CloseHandle(hPipe);
            }
            else {
                MessageBoxA(hWnd, "Помилка підключення до каналу. Сервер не запущено!", "Помилка", MB_OK | MB_ICONERROR);
            }
        } break;
        case ID_CLIENT_DEFAULT: SetDefaultMetrics(hWnd); break;
        case IDM_EXIT: DestroyWindow(hWnd); break;
        } break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcA(hWnd, message, wParam, lParam);
    } return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, "PipeClient1Class", LoadIcon(NULL, IDI_APPLICATION) };
    RegisterClassExA(&wcex);
    HWND hWnd = CreateWindowA("PipeClient1Class", "Client 1 - Named Pipes", WS_OVERLAPPEDWINDOW, 580, 100, 460, 270, NULL, CreateClientMenu(), hInstance, NULL);
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); } return (int)msg.wParam;
}