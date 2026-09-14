#include <Windows.h>
#include <vector>

// 全局变量：存储鼠标的原始移动数据
int g_mouseDeltaX = 0;
int g_mouseDeltaY = 0;

// 1. 注册Raw Input设备，捕获鼠标原始数据
bool RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    return RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE;
}

// 2. 窗口消息处理
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

// 3. 模拟键盘按键
void SimulateKeyPress(WORD vkCode) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// 4. 主循环：鼠标位移 -> 根据右键状态切换 WASD / 方向键
void MainLoop() {
    // 瞄准时灵敏度（按住右键）
    const float aimSensitivity = 0.6f;
    // 观察时灵敏度（不按右键）
    const float lookSensitivity = 1.5f;

    while (true) {
        if (g_mouseDeltaX != 0 || g_mouseDeltaY != 0) {
            // 检测右键是否按下（瞄准状态）
            bool aiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

            if (aiming) {
                // === 瞄准状态：鼠标 -> WASD ===
                if (g_mouseDeltaX > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * aimSensitivity; ++i) SimulateKeyPress('D');
                } else if (g_mouseDeltaX < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * aimSensitivity; ++i) SimulateKeyPress('A');
                }
                if (g_mouseDeltaY > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * aimSensitivity; ++i) SimulateKeyPress('S');
                } else if (g_mouseDeltaY < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * aimSensitivity; ++i) SimulateKeyPress('W');
                }
            } else {
                // === 普通状态：鼠标 -> 方向键（观察视角） ===
                if (g_mouseDeltaX > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * lookSensitivity; ++i) SimulateKeyPress(VK_RIGHT);
                } else if (g_mouseDeltaX < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaX) * lookSensitivity; ++i) SimulateKeyPress(VK_LEFT);
                }
                if (g_mouseDeltaY > 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * lookSensitivity; ++i) SimulateKeyPress(VK_DOWN);
                } else if (g_mouseDeltaY < 0) {
                    for (int i = 0; i < abs(g_mouseDeltaY) * lookSensitivity; ++i) SimulateKeyPress(VK_UP);
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
