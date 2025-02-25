#define UNICODE
#include <windows.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <string>
#include <sstream>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

#define ID_BTN_START 1
#define ID_BTN_STOP 2
#define ID_RADIO_NORMAL 3
#define ID_RADIO_CRONOMETRO 4
#define ID_EDIT_TIEMPO 5

// Variables globales
HWND hwndButtonStart, hwndEditDelay1, hwndEditDelay2, hwndEditDelay3;
HWND hwndRadioNormal, hwndRadioCronometro;
HWND hwndTiempo;
std::thread automationThread;
std::mutex mtx;
bool running = false;
bool modoCronometro = true;
double userDelay1 = 1.0, userDelay2 = 0.45, userDelay3 = 0.35;
double sistemOverhead = 0.0;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> random_delay(0.01, 0.05);

// Función para medir el overhead del sistema
double medirOverhead() {
    const int iteraciones = 1000;
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < iteraciones; i++) {
        LARGE_INTEGER dummy;
        QueryPerformanceCounter(&dummy);
    }
    
    QueryPerformanceCounter(&end);
    return (end.QuadPart - start.QuadPart) / (double)frequency.QuadPart / iteraciones;
}

inline void MoverCursorSuavemente(int xInicial, int yInicial, int xFinal, int yFinal, int pasos = 5) {
    const double deltaX = (xFinal - xInicial) / static_cast<double>(pasos);
    const double deltaY = (yFinal - yInicial) / static_cast<double>(pasos);
    for (int i = 0; i <= pasos; ++i) {
        SetCursorPos(xInicial + static_cast<int>(deltaX * i), yInicial + static_cast<int>(deltaY * i));
        std::this_thread::sleep_for(std::chrono::milliseconds(2 + static_cast<int>(random_delay(gen) * 5)));
    }
}

inline void HacerClic(int x, int y) {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    MoverCursorSuavemente(cursorPos.x, cursorPos.y, x, y, 3);
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}

inline void EnviarTeclas(std::vector<WORD> teclas, bool mantener = false) {
    std::vector<INPUT> inputs(teclas.size() * (mantener ? 1 : 2));
    for (size_t i = 0; i < teclas.size(); ++i) {
        inputs[i].type = INPUT_KEYBOARD;
        inputs[i].ki.wVk = teclas[i];
        if (!mantener) {
            inputs[teclas.size() + i].type = INPUT_KEYBOARD;
            inputs[teclas.size() + i].ki.wVk = teclas[i];
            inputs[teclas.size() + i].ki.dwFlags = KEYEVENTF_KEYUP;
        }
    }
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

void AutomatizarClics() {
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    if (modoCronometro) {
        HacerClic(505, 628);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay1 + random_delay(gen)));
        HacerClic(370, 128);
    } else {
        HacerClic(910, 739);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay1 + random_delay(gen)));
        HacerClic(904, 618);
    }
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay2 + random_delay(gen)));
    EnviarTeclas({VK_CONTROL, 'A'}, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EnviarTeclas({VK_CONTROL, 'A'}, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EnviarTeclas({VK_DELETE});
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EnviarTeclas({'1'});
    
    // Evitar delay aleatorio en operaciones críticas
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3));
    if (modoCronometro) {
        HacerClic(140, 691);
    } else {
        HacerClic(888, 659);
    }
    
    // Evitar delay aleatorio en operaciones críticas
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3));
    if (modoCronometro) {
        HacerClic(350, 650);
    } else {
        HacerClic(847, 637);
    }

    QueryPerformanceCounter(&end);
    double segundos = (end.QuadPart - start.QuadPart) / (double)frequency.QuadPart - sistemOverhead;
    
    // Actualizar el tiempo con microsegundos
    wchar_t tiempo[50];
    _snwprintf_s(tiempo, 50, _TRUNCATE, L"%.6f segundos", segundos);
    SendMessage(hwndTiempo, WM_SETTEXT, 0, (LPARAM)tiempo);
}

void IniciarAutomatizacion() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = true;
    }
    
    modoCronometro = (SendMessage(hwndRadioCronometro, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    wchar_t buffer[10];
    GetWindowText(hwndEditDelay1, buffer, 10);
    userDelay1 = _wtof(buffer);
    GetWindowText(hwndEditDelay2, buffer, 10);
    userDelay2 = _wtof(buffer);
    GetWindowText(hwndEditDelay3, buffer, 10);
    userDelay3 = _wtof(buffer);
    
    automationThread = std::thread([]() {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        AutomatizarClics();
        {
            std::lock_guard<std::mutex> lock(mtx);
            running = false;
        }
    });
    automationThread.detach();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // No creamos los radio buttons aquí, ya se crean en WinMain
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_START && !running) {
                IniciarAutomatizacion();
            }
            break;
        case WM_DESTROY:
            {
                std::lock_guard<std::mutex> lock(mtx);
                running = false;
            }
            timeEndPeriod(1);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Reducir la granularidad del temporizador a 1ms
    timeBeginPeriod(1);
    
    // Medir el overhead del sistema
    sistemOverhead = medirOverhead();
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MiClaseVentana";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Crear la ventana principal
    HWND hwnd = CreateWindowEx(
        0,
        L"MiClaseVentana",
        L"Automatización",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 310,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        timeEndPeriod(1);
        return 0;
    }

    const int centerX = 200;
    const int labelWidth = 70;
    const int editWidth = 60;
    const int startX = centerX - (labelWidth + editWidth) / 2;

    // Botón Ejecutar centrado
    hwndButtonStart = CreateWindowEx(
        0, L"BUTTON", L"Ejecutar",
        WS_VISIBLE | WS_CHILD,
        centerX - 50, 20, 100, 30,
        hwnd, (HMENU)ID_BTN_START,
        hInstance, NULL
    );

    // Campos de entrada para los delays
    CreateWindowEx(0, L"STATIC", L"Delay 1:", WS_VISIBLE | WS_CHILD,
        startX, 70, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay1 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"1.0",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 70, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

    CreateWindowEx(0, L"STATIC", L"Delay 2:", WS_VISIBLE | WS_CHILD,
        startX, 100, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay2 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.45",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 100, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

    CreateWindowEx(0, L"STATIC", L"Delay 3:", WS_VISIBLE | WS_CHILD,
        startX, 130, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay3 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.35",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 130, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

    // Radio buttons centrados
    hwndRadioNormal = CreateWindowEx(
        0, L"BUTTON", L"Normal",
        WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
        centerX - 100, 170, 100, 30,
        hwnd, (HMENU)ID_RADIO_NORMAL,
        hInstance, NULL
    );

    hwndRadioCronometro = CreateWindowEx(
        0, L"BUTTON", L"Cronómetro",
        WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
        centerX, 170, 100, 30,
        hwnd, (HMENU)ID_RADIO_CRONOMETRO,
        hInstance, NULL
    );

    // Campo para mostrar el tiempo de ejecución
    CreateWindowEx(
        0, L"STATIC", L"Tiempo de ejecución:",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 220, 400, 20,
        hwnd, NULL, hInstance, NULL
    );
    
    hwndTiempo = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.000000 segundos",
        WS_VISIBLE | WS_CHILD | ES_READONLY | ES_CENTER,
        centerX - 65, 240, 130, 20,
        hwnd, (HMENU)ID_EDIT_TIEMPO,
        hInstance, NULL
    );

    // Cambiar la selección predeterminada a Normal
    SendMessage(hwndRadioNormal, BM_SETCHECK, BST_CHECKED, 0);

    // Centrar la ventana en la pantalla
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hwnd, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}