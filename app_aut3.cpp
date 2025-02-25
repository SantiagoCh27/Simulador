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

HWND hwndButtonStart, hwndEditDelay1, hwndEditDelay2, hwndEditDelay3;
HWND hwndRadioNormal, hwndRadioCronometro;
HWND hwndTiempo;
std::thread automationThread;
std::mutex mtx;
bool running = false;
bool modoCronometro = true;
double userDelay1 = 1.0, userDelay2 = 0.45, userDelay3 = 0.35;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> random_delay(0.005, 0.02); // Reducido el rango de delay aleatorio

void HacerClicRapido(int x, int y) {
    SetCursorPos(x, y); // Movimiento directo sin suavizado
    
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}

void EnviarTeclas(std::vector<WORD> teclas, bool mantener = false) {
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
    
    // Optimización: Combinar las operaciones de teclado
    INPUT inputs[6] = {};
    // Ctrl Down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    // A Down
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'A';
    // A Up
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'A';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    // Ctrl Up
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    // Delete
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = VK_DELETE;
    // 1
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = '1';
    
    SendInput(6, inputs, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
    
    if (modoCronometro) {
        HacerClicRapido(140, 691);
        std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
        HacerClicRapido(395, 650);
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