#include <winsock2.h>
#define _WINSOCKAPI_
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "gui.h"
#include "dns_resolver.h"
#include <sstream>

#define IDC_INPUT 101
#define IDC_OUTPUT 102
#define IDC_BUTTON 103

GUI::GUI(HINSTANCE hInstance)
    : hInstance_(hInstance), hwndMain_(nullptr), hwndInput_(nullptr), hwndOutput_(nullptr),
    hwndButton_(nullptr), resolver_(new DNSResolver()) {}

GUI::~GUI() {
    delete resolver_; 
}

void GUI::Run() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = L"DNSResolverGUI";
    RegisterClass(&wc);

    hwndMain_ = CreateWindow(
        L"DNSResolverGUI",
        L"DNS Resolver",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInstance_, this
    );

    ShowWindow(hwndMain_, SW_SHOW);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void GUI::AddControls(HWND hwnd) {
    hwndInput_ = CreateWindow(
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        10, 10, 300, 25,
        hwnd, (HMENU)IDC_INPUT, hInstance_, nullptr
    );

    hwndOutput_ = CreateWindow(
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL,
        10, 50, 360, 150,
        hwnd, (HMENU)IDC_OUTPUT, hInstance_, nullptr
    );

    hwndButton_ = CreateWindow(
        L"BUTTON", L"Resolve",
        WS_CHILD | WS_VISIBLE,
        320, 10, 60, 25,
        hwnd, (HMENU)IDC_BUTTON, hInstance_, nullptr
    );
}

LRESULT CALLBACK GUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        GUI* pGUI = (GUI*)pcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pGUI);
        pGUI->AddControls(hwnd);
        return 0;
    }

    GUI* pGUI = (GUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pGUI) {
        switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON) {
                pGUI->OnResolveClicked();
            }
            break;

        case WM_USER + 1: {
            auto results = reinterpret_cast<std::wstring*>(wParam);
            int len = GetWindowTextLength(pGUI->hwndOutput_);
            if (len > 0) {
                SendMessage(pGUI->hwndOutput_, EM_SETSEL, len, len);
                SendMessage(pGUI->hwndOutput_, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
            }
            SendMessage(pGUI->hwndOutput_, EM_REPLACESEL, 0, (LPARAM)results->c_str());

            delete results;  
        } break;

        case WM_TIMER: {
            if (wParam == 1) { 
                const auto& results = pGUI->resolver_->getResults();
                for (const auto& address : results) {
                    int len = GetWindowTextLength(pGUI->hwndOutput_);
                    if (len > 0) {
                        SendMessage(pGUI->hwndOutput_, EM_SETSEL, len, len);
                        SendMessage(pGUI->hwndOutput_, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
                    }
                    SendMessage(pGUI->hwndOutput_, EM_REPLACESEL, 0, (LPARAM)address.c_str());
                }

                KillTimer(hwnd, 1);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GUI::OnResolveClicked() {
    wchar_t hostBuffer[256];
    GetWindowText(hwndInput_, hostBuffer, 256);

    SetWindowText(hwndOutput_, L"");
    std::wstring host(hostBuffer);
    if (host.empty()) {
        SetWindowText(hwndOutput_, L"Please enter a DNS name.");
        return;
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, host.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string host_utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, host.c_str(), -1, &host_utf8[0], size_needed, nullptr, nullptr);

    resolver_->setOutputHandle(hwndOutput_);
    resolver_->resolveHostAsync(host_utf8);

    SetTimer(hwndMain_, 1, 500, nullptr); 
}