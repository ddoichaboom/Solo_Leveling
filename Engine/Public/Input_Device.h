#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CInput_Device final : public CBase
{
private:
	CInput_Device();
	virtual ~CInput_Device() = default;

public:
	// 키보드 특정 키 상태. VK_W 등 Virtual Key Code 사용, 0x80 비트로 눌림 판별
	_byte					Get_KeyState(_ubyte byKeyID) {
		return m_byKeyState[byKeyID];
	}

	// 마우스 버튼 상태
	_byte					Get_MouseBtnState(MOUSEBTN eBtn) {
		return m_byMouseBtnState[ETOUI(eBtn)];
	}

	// 마우스 이동량
	_long					Get_MouseDelta(MOUSEAXIS eAxis) {
		return m_lMouseDelta[ETOUI(eAxis)];
	}

public:
	HRESULT					Initialize(HWND hWnd);
	void					Update();

	// WndProc에서 WM_INPUT 발생 시 호출하는 정적 콜백
	static void				Process_Input(LPARAM lParam);

private:
	// 프레임 데이터 (Update에서 복사됨, 게임 로직이 읽음)
	_byte					m_byKeyState[256] = {};
	_byte					m_byMouseBtnState[ETOUI(MOUSEBTN::END)] = {};
	_long					m_lMouseDelta[ETOUI(MOUSEAXIS::END)] = {};
	
	// 누적 버퍼 (WndProc에서 기록, Update에서 복사 후 초기화)
	static _byte			s_byRawKeyState[256];
	static _byte			s_byRawMouseBtn[ETOUI(MOUSEBTN::END)];
	static _long			s_lRawMouseAccum[ETOUI(MOUSEAXIS::END)];

public:
	static CInput_Device*	Create(HWND hWnd);
	virtual void			Free() override;
};

NS_END