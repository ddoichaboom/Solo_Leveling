#include "Timer.h"

CTimer::CTimer()
	: m_fTimeDelta(0.f)
{
	// 이미 멤버 초기화 리스트에서 유니폼 초기화가 이루어지고 있으므로, ZeroMemory 호출은 불필요.
	//ZeroMemory(&m_FixTime, sizeof(LARGE_INTEGER));
	//ZeroMemory(&m_LastTime, sizeof(LARGE_INTEGER));
	//ZeroMemory(&m_FrameTime, sizeof(LARGE_INTEGER));
	//ZeroMemory(&m_CpuTick, sizeof(LARGE_INTEGER));
}

HRESULT CTimer::Ready_Timer()
{
	// QueryPerformanceCounter 함수는 고해상도 타이머의 현재 값을 LARGE_INTEGER 구조체에 저장하는 함수
	QueryPerformanceCounter(&m_FrameTime);			// 1077
	QueryPerformanceCounter(&m_LastTime);			// 1085
	QueryPerformanceCounter(&m_FixTime);			// 1090

	QueryPerformanceFrequency(&m_CpuTick);		// cpu tick 값을 얻어오는 함수

	return S_OK;
}

void CTimer::Update_Timer()
{
	QueryPerformanceCounter(&m_FrameTime);			// 1500

	if (m_FrameTime.QuadPart - m_FixTime.QuadPart >= m_CpuTick.QuadPart)
	{
		QueryPerformanceFrequency(&m_CpuTick);
		m_FixTime = m_FrameTime;
	}

	m_fTimeDelta = (m_FrameTime.QuadPart - m_LastTime.QuadPart) / static_cast<_float>(m_CpuTick.QuadPart);

	m_LastTime = m_FrameTime;
}

CTimer* CTimer::Create()
{
	CTimer* pInstance = new CTimer;

	if (FAILED(pInstance->Ready_Timer()))
	{
		MSG_BOX("Failed to Created : CTimer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTimer::Free()
{
	__super::Free();
}