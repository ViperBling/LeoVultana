#include "ImGuiHelper.h"

static HWND gHWND;

bool ImGUI_Init(void *hwnd)
{
    gHWND = (HWND)hwnd;

    // Keyboard mapping.
    // ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    io.RenderDrawListsFn = nullptr;
    io.ImeWindowHandle = gHWND;

    return true;
}

void ImGUI_Shutdown()
{
    ImGui::Shutdown();
    gHWND = nullptr;
}

void ImGUI_UpdateIO()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    GetClientRect(gHWND, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
    // io.MousePos : filled by WM_MOUSEMOVE events
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Hide OS mouse cursor if ImGui is drawing it
    if (io.MouseDrawCursor)
        SetCursor(nullptr);    // Start the frame
}

static bool IsAnyMouseButtonDown()
{
    ImGuiIO& io = ImGui::GetIO();
    for (int n = 0; n < IM_ARRAYSIZE(io.MouseDown); n++)
        if (io.MouseDown[n])
            return true;
    return false;
}

IMGUI_API LRESULT ImGUI_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN) button = 0;
            else if (msg == WM_RBUTTONDOWN) button = 1;
            else if (msg == WM_MBUTTONDOWN) button = 2;
            if (!IsAnyMouseButtonDown() && GetCapture() == NULL)
                SetCapture(hwnd);
            io.MouseDown[button] = true;
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP) button = 0;
            else if (msg == WM_RBUTTONUP) button = 1;
            else if (msg == WM_MBUTTONUP) button = 2;
            io.MouseDown[button] = false;
            if (!IsAnyMouseButtonDown() && GetCapture() == hwnd)
                ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL:
            io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
            return 0;
        case WM_MOUSEMOVE:
            io.MousePos.x = (signed short)(lParam);
            io.MousePos.y = (signed short)(lParam >> 16);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256)
                io.KeysDown[wParam] = true;
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256)
                io.KeysDown[wParam] = false;
            return 0;
        case WM_CHAR:
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (wParam > 0 && wParam < 0x10000)
                io.AddInputCharacter((unsigned short)wParam);
            return 0;
    }
    return 0;
}
