#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBase	abstract
{
protected:
	CBase();
	virtual ~CBase() = default;

public:
	// 레퍼런스 카운트를 증가시킨다.
	_uint AddRef();

	// 레퍼런스 카운트를 감소시킨다. or 삭제한다 
	_uint Release();

protected:
	_uint			m_iRefCnt = {};		// 0으로 유니폼 초기화

public:
	virtual void Free();

};

NS_END