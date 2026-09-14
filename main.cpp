#include <Windows.h>
#include <vector>

// 鼠标移动增量
int g_mouseDeltaX = 0;
int g_mouseDeltaY = 0;
// 右键状态（从Raw Input直接获取，比GetAsyncKeyState可靠）
bool g_rightButtonDown = false;

// 注册Raw Input
bool RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    return RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE;
}

// 窗口消息处理
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
                    // 从Raw Input直接捕获右键状态
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

// 用硬件扫描码模拟按键（DirectInput才能识别）
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

// 扫描码定义
#define SCAN_W      0x11
#define SCAN_A      0x1E
#define SCAN_S      0x1F
#define SCAN_D      0x20
#define SCAN_UP     0x48
#define SCAN_DOWN   0x50
#define SCAN_LEFT   0x4B
#define SCAN_RIGHT  0x4D

// 主循环
void MainLoop() {
    const float aimSensitivity = 0.6f;
    const float lookSensitivity = 1.5f;

    while (true) {
        if (g_mouseDeltaX != 0 || g_mouseDeltaY != 0) {
            bool aiming = g_rightButtonDown;

            if (aiming) {
                // 瞄准：鼠标 -> WASD
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
                // 观察：鼠标 -> 方向键（扩展键）
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

// 程序入口
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
