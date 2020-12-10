#include "Window.h"
#include <cassert>
#include <utility>


HWND Window::ConstructWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_Dimensions.width = windowRect.right - windowRect.left;
    m_Dimensions.height = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - m_Dimensions.width) / 2);
    int windowY = std::max<int>(0, (screenHeight - m_Dimensions.height) / 2);
    HWND hWnd = ::CreateWindowExW(
        NULL,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        m_Dimensions.width,
        m_Dimensions.height,
        NULL,
        NULL,
        hInst,
        nullptr
    );

    assert(hWnd && "Failed to create window");

    //put this pointer in the hWnd
    SetWindowLongPtr(hWnd, GWLP_USERDATA, LONG_PTR(this));

    return hWnd;
}

void Window::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
    // Register a window class for creating our render window with.
    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProcStatic;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, NULL);
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm = ::LoadIcon(hInst, NULL);

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

LRESULT Window::WndProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* pThisWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (pThisWindow) 
    {
       return pThisWindow->WndProc(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Input(hwnd, message, wParam, lParam);
    switch (message)
    {
        //input
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        case VK_F11:
            SetFullscreen(!m_Fullscreen);
        }
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        break;
    case WM_SYSCHAR: //ALT+letter = systemcommand
    case WM_CHAR:
        break;
    case WM_DESTROY:
    case WM_CLOSE:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return 0;
}

void Window::SetFullscreen(bool fullscreen)
{
    if (m_Fullscreen != fullscreen)
    {
        m_Fullscreen = fullscreen;

        if (m_Fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored 
            // when switching out of fullscreen state.
            ::GetWindowRect(m_Hwnd, &m_WindowRect);
            // Set the window style to a borderless window so the client area fills
            // the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(m_Hwnd, GWL_STYLE, windowStyle);
            // Query the name of the nearest display device for the window.
            // This is required to set the fullscreen dimensions of the window
            // when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(m_Hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);
            ::SetWindowPos(m_Hwnd, HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_Hwnd, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(m_Hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(m_Hwnd, HWND_NOTOPMOST,
                m_WindowRect.left,
                m_WindowRect.top,
                m_WindowRect.right - m_WindowRect.left,
                m_WindowRect.bottom - m_WindowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_Hwnd, SW_NORMAL);
        }
    }
}

Window::Window(HINSTANCE hInstance, uint32_t width, uint32_t height)
    :m_Dimensions{width, height}
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Window class name. Used for registering / creating the window.
    const wchar_t* windowClassName = L"DX12WindowClass";
    RegisterWindowClass(hInstance, windowClassName);
    m_Hwnd = ConstructWindow(windowClassName, hInstance, L"Ellipsoids",
        m_Dimensions.width, m_Dimensions.height);

    // Initialize the global window rect variable.
    ::GetWindowRect(m_Hwnd, &m_WindowRect);
    ::ShowWindow(m_Hwnd, SW_SHOW);
}


HWND Window::GetHandle()
{
    return m_Hwnd;
}

Dimensions<uint32_t> Window::GetDimensions()
{
    return m_Dimensions;
}

InputListener::~InputListener()
{
    if (m_pNextListener) m_pNextListener->m_pPreviousListener = m_pPreviousListener;
    if (m_pPreviousListener) m_pPreviousListener->m_pNextListener = m_pNextListener;
}

void InputListener::AddListener(InputListener* pListener)
{
    if (pListener == nullptr) return;

    if (m_pNextListener)
    {
        m_pNextListener->AddListener(pListener);
    }
    else
    {
        m_pNextListener = pListener;
        pListener->m_pPreviousListener = this;
    }
}

void InputListener::RemoveListener(InputListener* pListener)
{
    if (pListener && m_pNextListener == pListener)
    {
        m_pNextListener = m_pNextListener->m_pNextListener;
        if (m_pNextListener)
        {
            m_pNextListener->m_pPreviousListener = this;
        }
    }
}

void InputListener::Input(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HandleInput(hwnd, message, wParam, lParam);
    if (m_pNextListener)
    {
        m_pNextListener->Input(hwnd, message, wParam, lParam);
    }
}
