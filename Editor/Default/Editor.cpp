// Editor.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "Editor.h"
#include "Editor_Defines.h"

#include "EditorApp.h"
#include "GameInstance.h"

// ImGui WndProc 핸들러 전방선언
extern LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE           hInst;                                      // 현재 인스턴스입니다.
HWND                g_hWnd;
WCHAR               szTitle[MAX_LOADSTRING];                    // 제목 표시줄 텍스트입니다.
WCHAR               szWindowClass[MAX_LOADSTRING];              // 기본 창 클래스 이름입니다.
_bool               g_bResizing = { false };

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CEditorApp* pEditorApp = { nullptr };

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EDITOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EDITOR));

    MSG msg;

    // EditorApp 생성
    RECT rcClient{};
    GetClientRect(g_hWnd, &rcClient);

    pEditorApp = CEditorApp::Create(g_hWnd,
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top);

    if (nullptr == pEditorApp)
        return FALSE;

    CGameInstance* pGameInstance = CGameInstance::GetInstance();
    Safe_AddRef(pGameInstance);

    // Timer 등록
    // 1. 기본 타이머
    if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_Default"))))
        return E_FAIL;
    // 2. 60 프레임 타이머
    if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_60"))))
        return E_FAIL;

    _float fTimeAcc = { };


    // 게임 루프 (PeekMessage)
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
                break;

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        pGameInstance->Compute_Timer(TEXT("Timer_Default"));
        fTimeAcc += pGameInstance->Get_TimeDelta(TEXT("Timer_Default"));

        if (fTimeAcc >= 1.f / 60.f)
        {
            pGameInstance->Compute_Timer(TEXT("Timer_60"));

            pEditorApp->Update(pGameInstance->Get_TimeDelta(TEXT("Timer_60")));
            pEditorApp->Render();

            fTimeAcc = 0.f;
        }
    }

    Safe_Release(pGameInstance);
    Safe_Release(pEditorApp);

    return (int)msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITOR));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    // DXGI로 모니터 해상도 가져오기

    // 실패시 기본값
    _uint g_iWinSizeX = 1600;
    _uint g_iWinSizeY = 900;
    Get_MonitorResolution(&g_iWinSizeX, &g_iWinSizeY);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        0, 0,
        g_iWinSizeX,
        g_iWinSizeY,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    //ShowWindow(hWnd, nCmdShow);
    // 크기 최대 
    ShowWindow(hWnd, SW_MAXIMIZE);
    UpdateWindow(hWnd);

    g_hWnd = hWnd;

    return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 시스템 메시지 먼저 처리
    switch (message)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)  // Alt 키 메뉴 비활성화
            return 0;
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_ENTERSIZEMOVE:
    {
        g_bResizing = true;
    }
    return 0;
    case WM_EXITSIZEMOVE:
    {
        g_bResizing = false;

        RECT rcClient{};
        GetClientRect(hWnd, &rcClient);

        _uint iWidth = rcClient.right - rcClient.left;
        _uint iHeight = rcClient.bottom - rcClient.top;

        if (iWidth == 0 || iHeight == 0)
            break;

        CGameInstance* pGameInstance = CGameInstance::GetInstance();
        if (nullptr != pGameInstance)
            pGameInstance->OnResize(iWidth, iHeight);
    }
    return 0;
    case WM_SIZE:
    {

        if (wParam == SIZE_MINIMIZED)
            break;

        if (g_bResizing)
            break;

        _uint iWidth = LOWORD(lParam);
        _uint iHeight = HIWORD(lParam);

        if (iWidth == 0 || iHeight == 0)
            break;

        CGameInstance* pGameInstance = CGameInstance::GetInstance();
        if (nullptr != pGameInstance)
            pGameInstance->OnResize(iWidth, iHeight);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // ImGui 입력 처리
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    return DefWindowProc(hWnd, message, wParam, lParam);
}

