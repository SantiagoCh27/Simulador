#define UNICODE
#include <windows.h>
#include <chrono>
#include <stdio.h>

#define ID_BTN_START 1

HWND hwndButtonStart;

// Función para realizar un clic en coordenadas específicas
void HacerClic(int x, int y) {
    SetCursorPos(x, y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    Sleep(100); // Pequeña pausa entre clics
}

// Función de automatización de clics
void AutomatizarClics() {
    auto inicio = std::chrono::high_resolution_clock::now();
    
    // Coordenadas de los clics (ajústalas según sea necesario)
    HacerClic(505, 628);
    HacerClic(370, 128);
    
    // Simulación de Ctrl + A y Delete
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('A', 0, 0, 0);
    keybd_event('A', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_DELETE, 0, 0, 0);
    keybd_event(VK_DELETE, 0, KEYEVENTF_KEYUP, 0);
    
    // Escribir "1"
    keybd_event('1', 0, 0, 0);
    keybd_event('1', 0, KEYEVENTF_KEYUP, 0);
    
    HacerClic(140, 791);
    HacerClic(350, 650);
    
    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracion = fin - inicio;
    
    wchar_t mensaje[100];
    _snwprintf_s(mensaje, 100, _TRUNCATE, L"Tiempo de ejecución: %.3f segundos", duracion.count());



    MessageBox(NULL, mensaje, L"Finalizado", MB_OK);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hwndButtonStart = CreateWindowEx(0, L"BUTTON", L"Iniciar Automatización",
                                             WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                             50, 50, 200, 30,
                                             hwnd, (HMENU)ID_BTN_START, GetModuleHandle(NULL), NULL);
            break;
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_START) {
                AutomatizarClics();
            }
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"VentanaAutomatizacion";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Automatización de Clics", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
                               NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
