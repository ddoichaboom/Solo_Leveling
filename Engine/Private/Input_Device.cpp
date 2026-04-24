#include "Input_Device.h"

// static 멤버 초기화
_byte		CInput_Device::s_byRawKeyState[256] = {};
_byte		CInput_Device::s_byRawMouseBtn[ETOUI(MOUSEBTN::END)] = {};
_long		CInput_Device::s_lRawMouseAccum[ETOUI(MOUSEAXIS::END)] = {};

CInput_Device::CInput_Device()
{
	ZeroMemory(m_byKeyState, sizeof(m_byKeyState));
}

HRESULT CInput_Device::Initialize(HWND hWnd)
{
	// Raw Input Device 등록
	RAWINPUTDEVICE rid[2]{};

	// 키보드
	rid[0].usUsagePage	= 0x01;			// HID_USAGE_PAGE_GENERIC
	rid[0].usUsage		= 0x06;			// HID_USAGE_GENERIC_KEYBOARD
	rid[0].dwFlags		= 0;			// 포커스 있을 때만 수신
	rid[0].hwndTarget	= hWnd;

	// 마우스
	rid[1].usUsagePage	= 0x01;			// HID_USAGE_PAGE_GENERIC
	rid[1].usUsage		= 0x02;			// HID_USAGE_GENERIC_MOUSE
	rid[1].dwFlags		= 0;
	rid[1].hwndTarget	= hWnd;

	if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)))
		return E_FAIL;

	return S_OK;
}

// WndProc의 WM_INPUT 케이스에서 호출
// 여러 번 호출될 수 있으므로 마우스 이동은 누적(+=), 키/버튼은 덮어쓰기
void CInput_Device::Process_Input(LPARAM lParam)
{
	// (1) 데이터 크기 확인
	_uint dwSize = { 0 };
	GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),			// WndProc에서 넘어온 Raw Input 핸들
		RID_INPUT,									  
		nullptr,													// nullptr -> "데이터는 주지말고 크기만 알려줘"
		&dwSize,													// 필요한 바이트 수 저장
		sizeof(RAWINPUTHEADER));

	if (0 == dwSize)
		return;

	// (2) 메모리 할당 + 실제 데이터 수신
	_ubyte* lpb = new _ubyte[dwSize];
	
	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
		RID_INPUT,
		lpb,														// 데이터를 여기에 담아라.
		&dwSize,
		sizeof(RAWINPUTHEADER)) != dwSize)
	{
		Safe_Delete_Array(lpb);
		return;
	}

	// (3) RAWINPUT 구조체로 해석
	RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);

	// (4) 키보드 처리
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		// VKey = Vitual Key Code (VK_W 등, <Windows.h>에 정의)
		_ushort vKey = raw->data.keyboard.VKey;

		if (vKey < 256)
		{
			if (raw->data.keyboard.Flags & RI_KEY_BREAK)
				s_byRawKeyState[vKey] = 0x00;				// 키 떼짐
			else
				s_byRawKeyState[vKey] = 0x80;				// 키 눌림
		}
	}
	// (5) 마우스 처리
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		// 마우스 이동량 누적 (한 프레임에 WM_INPUT 여러 번 올 수 있음)
		s_lRawMouseAccum[ETOUI(MOUSEAXIS::X)] += raw->data.mouse.lLastX;
		s_lRawMouseAccum[ETOUI(MOUSEAXIS::Y)] += raw->data.mouse.lLastY;

		// 마우스 버튼 상태
		_ushort flags = raw->data.mouse.usButtonFlags;

		if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::LBUTTON)] = 0x80;
		if (flags & RI_MOUSE_LEFT_BUTTON_UP)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::LBUTTON)] = 0x00;

		if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::RBUTTON)] = 0x80;
		if (flags & RI_MOUSE_RIGHT_BUTTON_UP)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::RBUTTON)] = 0x00;

		if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::MBUTTON)] = 0x80;
		if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)
			s_byRawMouseBtn[ETOUI(MOUSEBTN::MBUTTON)] = 0x00;

		// 휠 스크롤
		if (flags & RI_MOUSE_WHEEL)
			s_lRawMouseAccum[ETOUI(MOUSEAXIS::WHEEL)] +=
					static_cast<_short>(raw->data.mouse.usButtonData);
	}

	// (6) 정리
	Safe_Delete_Array(lpb);
}

void CInput_Device::Update()
{
	memcpy(m_byPrevKeyState, m_byKeyState, sizeof(m_byKeyState));
	memcpy(m_byPrevMouseBtnState, m_byMouseBtnState, sizeof(m_byMouseBtnState));

	// 누적된 Raw Input 데이터를 프레임 데이터로 복사
	memcpy(m_byKeyState, s_byRawKeyState, sizeof(m_byKeyState));
	memcpy(m_byMouseBtnState, s_byRawMouseBtn, sizeof(m_byMouseBtnState));
	memcpy(m_lMouseDelta, s_lRawMouseAccum, sizeof(m_lMouseDelta));

	// 마우스 이동량은 프레임당 델타이므로 누적 초기화
	// 키/버튼 상태는 초기화하지 않음 (눌린채로 유지되어야 함)
	ZeroMemory(s_lRawMouseAccum, sizeof(s_lRawMouseAccum));
}

CInput_Device* CInput_Device::Create(HWND hWnd)
{
	CInput_Device* pInstance = new CInput_Device();

	if (FAILED(pInstance->Initialize(hWnd)))
	{
		MSG_BOX("Failed to Created : CInput_Device");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CInput_Device::Free()
{
	// COM 객체가 없으므로 별도 해제 불필요
}