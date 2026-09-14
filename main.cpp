#include <Windows.h>
#include <vector>

int g_mouseDeltaX = 0;
int g_mouseDeltaY = 0;
bool g_rightButtonDown = false;

bool RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    return RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INPUT: {
        UINT dataSize = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
        if (dataSize > 0) {
            std::vector<BYTE> buffer(dataSize);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    g_mouseDeltaX += raw->data.mouse.lLastX;
                    g_mouseDeltaY += raw->data.mouse.lLastY;
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) g_rightButtonDown = true;
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) g_rightButtonDown = false;
                }
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SimulateKeyPressByScan(WORD scanCode, bool extended) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    SendInput(1, &input, sizeof(INPUT));

    input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

#define SCAN_W      0x11
#define SCAN_A      0x1E
#define SCAN_S      0x1F
#define SCAN_D      0x20
#define SCAN_UP     0x48
#define SCAN_DOWN   0x50
#define SCAN_LEFT   0x4B
#define SCAN_RIGHT  0x4D

void MainLoop() {
    const float aimSensitivity = 0.6f;
    const float lookSensitivity = 1.5f;

    MSG msg;
    while (true) {
        // === 关键修复：消息循环必须在主循环里 ===
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_mouseDeltaX != 0 || g_mouseDeltaY != 0) {
            bool aiming = g_rightButtonDown;

            if (aiming) {
                if (g_mouseDeltaX > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * aimSensitivity; ++i) SimulateKeyPressByScan(SCAN_D, false);
                } else if (g_mouseDeltaX < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * aimSensitivity; ++i) SimulateKeyPressByScan(SCAN_A, false);
                }
                if (g_mouseDeltaY > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * aimSensitivity; ++i) SimulateKeyPressByScan(SCAN_S, false);
                } else if (g_mouseDeltaY < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * aimSensitivity; ++i) SimulateKeyPressByScan(SCAN_W, false);
                }
            } else {
                if (g_mouseDeltaX > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * lookSensitivity; ++i) SimulateKeyPressByScan(SCAN_RIGHT, true);
                } else if (g_mouseDeltaX < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * lookSensitivity; ++i) SimulateKeyPressByScan(SCAN_LEFT, true);
                }
                if (g_mouseDeltaY > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * lookSensitivity; ++i) SimulateKeyPressByScan(SCAN_DOWN, true);
                } else if (g_mouseDeltaY < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * lookSensitivity; ++i) SimulateKeyPressByScan(SCAN_UP, true);
                }
            }

            g_mouseDeltaX = 0;
            g_mouseDeltaY = 0;
        }
        Sleep(1);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RawInputWindowClass";
    RegisterClass(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, "RawInputWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (hwnd == nullptr) {
        MessageBox(nullptr, "Window creation failed", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    if (!RegisterRawInput(hwnd)) {
        MessageBox(nullptr, "Register Raw Input failed", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    MainLoop();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, hInstance);
    return 0;
}
