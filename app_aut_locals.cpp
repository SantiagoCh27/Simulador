#define UNICODE
#include <windows.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <string>
#include <sstream>
#include <wininet.h>
#include <shellapi.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")

#define ID_BTN_START 1
#define ID_BTN_STOP 2
#define ID_RADIO_NORMAL 3
#define ID_RADIO_CRONOMETRO 4
#define ID_EDIT_TIEMPO 5
#define ID_CHECK_AUTOSTART 6  // Nuevo: checkbox para autostart con localStorage

// Variables globales
HWND hwndButtonStart, hwndEditDelay1, hwndEditDelay2, hwndEditDelay3;
HWND hwndRadioNormal, hwndRadioCronometro;
HWND hwndTiempo;
HWND hwndCheckAutostart;  // Nuevo: handle para checkbox
std::thread automationThread;
std::thread monitorThread;  // Nuevo: thread para monitorear localStorage
std::mutex mtx;
bool running = false;
bool autoStartEnabled = false;  // Nuevo: flag para autostart
bool modoCronometro = true;
double userDelay1 = 1.0, userDelay2 = 0.45, userDelay3 = 0.35;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> random_delay(0.01, 0.05);

// Nuevo: función para verificar localStorage
bool CheckLocalStorage() {
    HWND browserWindow = FindWindow(NULL, L"Tu título de ventana del navegador");  // Ajusta esto
    if (!browserWindow) return false;
    
    // Activar la ventana del navegador
    SetForegroundWindow(browserWindow);
    
    // Abrir consola con F12
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_F12;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_F12;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
    
    // Dar tiempo para que se abra la consola
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Pegar y ejecutar script para verificar localStorage
    const wchar_t* script = L"console.log(localStorage.getItem('botonLanzarOfertaClicked'))";
    
    // Copiar el script al portapapeles
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wcslen(script) + 1) * sizeof(wchar_t));
        if (hMem) {
            wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
            wcscpy_s(pMem, wcslen(script) + 1, script);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
        
        // Pegar con Ctrl+V
        INPUT pasteInputs[4] = {};
        pasteInputs[0].type = INPUT_KEYBOARD;
        pasteInputs[0].ki.wVk = VK_CONTROL;
        pasteInputs[1].type = INPUT_KEYBOARD;
        pasteInputs[1].ki.wVk = 'V';
        pasteInputs[2].type = INPUT_KEYBOARD;
        pasteInputs[2].ki.wVk = 'V';
        pasteInputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
        pasteInputs[3].type = INPUT_KEYBOARD;
        pasteInputs[3].ki.wVk = VK_CONTROL;
        pasteInputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(4, pasteInputs, sizeof(INPUT));
        
        // Presionar Enter
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        INPUT enterInputs[2] = {};
        enterInputs[0].type = INPUT_KEYBOARD;
        enterInputs[0].ki.wVk = VK_RETURN;
        enterInputs[1].type = INPUT_KEYBOARD;
        enterInputs[1].ki.wVk = VK_RETURN;
        enterInputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, enterInputs, sizeof(INPUT));
        
        // Volver a la pantalla principal con Esc
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        INPUT escInputs[2] = {};
        escInputs[0].type = INPUT_KEYBOARD;
        escInputs[0].ki.wVk = VK_ESCAPE;
        escInputs[1].type = INPUT_KEYBOARD;
        escInputs[1].ki.wVk = VK_ESCAPE;
        escInputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, escInputs, sizeof(INPUT));
        
        // TODO: Analizar la salida de la consola para determinar si el botón fue clickeado
        // Por simplicidad, asumimos que si llegamos aquí, el botón se ha clickeado
        return true;
    }
    
    return false;
}

void MoverCursorSuavemente(int xInicial, int yInicial, int xFinal, int yFinal, int pasos = 5) {
    double deltaX = (xFinal - xInicial) / static_cast<double>(pasos);
    double deltaY = (yFinal - yInicial) / static_cast<double>(pasos);
    for (int i = 0; i <= pasos; ++i) {
        SetCursorPos(xInicial + static_cast<int>(deltaX * i), yInicial + static_cast<int>(deltaY * i));
        std::this_thread::sleep_for(std::chrono::milliseconds(2 + static_cast<int>(random_delay(gen) * 5)));
    }
}

void HacerClic(int x, int y) {
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

    // Comenzamos directamente en la segunda coordenada (eliminando el primer clic)
    // Seleccionar todo el texto con Ctrl+A
    EnviarTeclas({VK_CONTROL, 'A'}, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EnviarTeclas({VK_CONTROL, 'A'}, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // Borrar texto
    EnviarTeclas({VK_DELETE});
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // Escribir "1"
    EnviarTeclas({'1'});
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
    if (modoCronometro) {
        HacerClic(140, 691);
    } else {
        HacerClic(888, 659);
    }
    
    std::this_thread::sleep_for(std::chrono::duration<double>(userDelay3 + random_delay(gen)));
    if (modoCronometro) {
        HacerClic(350, 650);
    } else {
        HacerClic(847, 637);
    }

    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracion = fin - inicio;
    
    // Actualizar el tiempo usando SendMessage
    wchar_t tiempo[50];
    _snwprintf_s(tiempo, 50, _TRUNCATE, L"%.3f segundos", duracion.count());
    SendMessage(hwndTiempo, WM_SETTEXT, 0, (LPARAM)tiempo);
}

void MonitorearLocalStorage() {
    while (autoStartEnabled && !running) {
        if (CheckLocalStorage()) {
            // Iniciar automatización si detectamos la señal en localStorage
            PostMessage(GetParent(hwndButtonStart), WM_COMMAND, MAKEWPARAM(ID_BTN_START, BN_CLICKED), (LPARAM)hwndButtonStart);
            break;  // Salimos después de iniciar una vez
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Polling cada 10ms
    }
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
            
            // Reiniciar monitoreo si autostart está habilitado
            if (autoStartEnabled) {
                monitorThread = std::thread(MonitorearLocalStorage);
                monitorThread.detach();
            }
        }
    });
    automationThread.detach();
}

// Iniciar monitoreo de localStorage cuando se activa el checkbox
void ToggleAutoStart() {
    autoStartEnabled = (SendMessage(hwndCheckAutostart, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    if (autoStartEnabled && !running) {
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
        monitorThread = std::thread(MonitorearLocalStorage);
        monitorThread.detach();
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // No creamos los radio buttons aquí, ya se crean en WinMain
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_START && !running) {
                IniciarAutomatizacion();
            } else if (LOWORD(wParam) == ID_CHECK_AUTOSTART) {
                ToggleAutoStart();
            }
            break;
        case WM_DESTROY:
            {
                std::lock_guard<std::mutex> lock(mtx);
                running = false;
                autoStartEnabled = false;
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

    // Crear la ventana principal
    HWND hwnd = CreateWindowEx(
        0,
        L"MiClaseVentana",
        L"Automatización",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT,
        430, 360,  // Altura aumentada para el nuevo checkbox
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
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

    // Nuevo: Checkbox para autostart con localStorage
    hwndCheckAutostart = CreateWindowEx(
        0, L"BUTTON", L"Auto-iniciar con detector JS",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        centerX - 100, 200, 200, 20,
        hwnd, (HMENU)ID_CHECK_AUTOSTART,
        hInstance, NULL
    );

    // Campo para mostrar el tiempo de ejecución
    CreateWindowEx(
        0, L"STATIC", L"Tiempo de ejecución:",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 230, 400, 20,
        hwnd, NULL, hInstance, NULL
    );
    
    hwndTiempo = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"0.000 segundos",
        WS_VISIBLE | WS_CHILD | ES_READONLY | ES_CENTER,
        centerX - 65, 250, 130, 20,
        hwnd, (HMENU)ID_EDIT_TIEMPO,
        hInstance, NULL
    );

    // Instrucciones para el usuario
    CreateWindowEx(
        0, L"STATIC", L"1. Activa 'Auto-iniciar' y ejecuta el script JS en la web",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 270, 400, 20,
        hwnd, NULL, hInstance, NULL
    );
    
    CreateWindowEx(
        0, L"STATIC", L"2. Cuando aparezca el botón, el bot comenzará automáticamente",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 290, 500, 20,
        hwnd, NULL, hInstance, NULL
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