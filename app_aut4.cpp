#define UNICODE
#include <windows.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <string>
#include <sstream>

#define ID_BTN_START 1
#define ID_BTN_STOP 2
#define ID_RADIO_NORMAL 3
#define ID_RADIO_CRONOMETRO 4
#define ID_EDIT_TIEMPO 5

// Optimized delay constants
#define MIN_DELAY 0.001
#define MAX_DELAY 0.005
#define BASE_DELAY_1 0.1
#define BASE_DELAY_2 0.05
#define BASE_DELAY_3 0.05

HWND hwndButtonStart, hwndEditDelay1, hwndEditDelay2, hwndEditDelay3;
HWND hwndRadioNormal, hwndRadioCronometro;
HWND hwndTiempo;
std::thread automationThread;
std::mutex mtx;
bool running = false;
bool modoCronometro = true;
double userDelay1 = BASE_DELAY_1, userDelay2 = BASE_DELAY_2, userDelay3 = BASE_DELAY_3;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> random_delay(MIN_DELAY, MAX_DELAY);

void EnviarInputsCombinados(const std::vector<INPUT>& inputs) {
    SendInput(static_cast<UINT>(inputs.size()), const_cast<LPINPUT>(inputs.data()), sizeof(INPUT));
}

void PrepararMouseInput(INPUT& input, int x, int y, DWORD flags) {
    input.type = INPUT_MOUSE;
    if (flags & MOUSEEVENTF_MOVE) {
        input.mi.dx = x * (65535 / GetSystemMetrics(SM_CXSCREEN));
        input.mi.dy = y * (65535 / GetSystemMetrics(SM_CYSCREEN));
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    } else {
        input.mi.dwFlags = flags;
    }
}

void HacerClicRapido(int x, int y) {
    std::vector<INPUT> inputs(3);
    PrepararMouseInput(inputs[0], x, y, MOUSEEVENTF_MOVE);
    PrepararMouseInput(inputs[1], 0, 0, MOUSEEVENTF_LEFTDOWN);
    PrepararMouseInput(inputs[2], 0, 0, MOUSEEVENTF_LEFTUP);
    EnviarInputsCombinados(inputs);
}

void AutomatizarClics() {
    // Set thread priority to highest
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    
    auto inicio = std::chrono::high_resolution_clock::now();

    if (modoCronometro) {
        HacerClicRapido(505, 628);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay1 + random_delay(gen)));
        HacerClicRapido(370, 128);
    } else {
        HacerClicRapido(910, 739);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay1 + random_delay(gen)));
        HacerClicRapido(904, 618);
    }
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay2 + random_delay(gen)));
    
    // Keyboard inputs optimized and combined
    std::vector<INPUT> keyboardInputs(6);
    for (auto& input : keyboardInputs) {
        input.type = INPUT_KEYBOARD;
    }
    
    // Ctrl+A
    keyboardInputs[0].ki.wVk = VK_CONTROL;
    keyboardInputs[1].ki.wVk = 'A';
    keyboardInputs[2].ki.wVk = 'A';
    keyboardInputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    keyboardInputs[3].ki.wVk = VK_CONTROL;
    keyboardInputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    
    // Delete + 1
    keyboardInputs[4].ki.wVk = VK_DELETE;
    keyboardInputs[5].ki.wVk = '1';
    
    EnviarInputsCombinados(keyboardInputs);
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
    
    if (modoCronometro) {
        HacerClicRapido(140, 691);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
        HacerClicRapido(350, 650);
    } else {
        HacerClicRapido(888, 659);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
        HacerClicRapido(847, 637);
    }

    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracion = fin - inicio;
    
    wchar_t tiempo[50];
    _snwprintf_s(tiempo, 50, _TRUNCATE, L"%.3f segundos", duracion.count());
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
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Set process priority to high
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MiClaseVentana";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

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
        return 0;
    }

    const int centerX = 200;
    const int labelWidth = 70;
    const int editWidth = 60;
    const int startX = centerX - (labelWidth + editWidth) / 2;

    // Create UI elements with optimized default values
    hwndButtonStart = CreateWindowEx(
        0, L"BUTTON", L"Ejecutar",
        WS_VISIBLE | WS_CHILD,
        centerX - 50, 20, 100, 30,
        hwnd, (HMENU)ID_BTN_START,
        hInstance, NULL
    );

    CreateWindowEx(0, L"STATIC", L"Delay 1:", WS_VISIBLE | WS_CHILD,
        startX, 70, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay1 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.1",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 70, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

    CreateWindowEx(0, L"STATIC", L"Delay 2:", WS_VISIBLE | WS_CHILD,
        startX, 100, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay2 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.05",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 100, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

    CreateWindowEx(0, L"STATIC", L"Delay 3:", WS_VISIBLE | WS_CHILD,
        startX, 130, labelWidth, 20, hwnd, NULL, hInstance, NULL);
    hwndEditDelay3 = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.05",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        startX + labelWidth, 130, editWidth, 20,
        hwnd, NULL, hInstance, NULL
    );

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

    CreateWindowEx(
        0, L"STATIC", L"Tiempo de ejecución:",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 220, 400, 20,
        hwnd, NULL, hInstance, NULL
    );
    
    hwndTiempo = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.000 segundos",
        WS_VISIBLE | WS_CHILD | ES_READONLY | ES_CENTER,
        centerX - 65, 240, 130, 20,
        hwnd, (HMENU)ID_EDIT_TIEMPO,
        hInstance, NULL
    );

    SendMessage(hwndRadioNormal, BM_SETCHECK, BST_CHECKED, 0);

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