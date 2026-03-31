#pragma once

#include "Base.h"

// 보관하고 있는 레벨의 반복적인 Update, Render를 수행한다.
// 현재 할당된 레벨의 주소를 보관한다.
// 원활한 레벨 교체작업을 수행한다.
// - 레벨 교체 시 이전 레벨을 삭제한다.
// - 기존 레벨 용 자원을 정리해준다.

NS_BEGIN(Engine)

class CLevel_Manager final : public CBase
{
private:
	CLevel_Manager();
	virtual ~CLevel_Manager() = default;

public:
	HRESULT					Change_Level(_int iNewLevelIndex, class CLevel* pNewLevel);
	void					Update(_float fTimeDelta);
	HRESULT					Render();

private:
	class CLevel*			m_pCurrentLevel = { nullptr };
	class CGameInstance*	m_pGameInstance = { nullptr };

	// Engine에서는 Client의 열거체를 알고 있을 수 없기 때문에
	// END를 사용할 수 없어서 -1로 초기화한다.
	_int					m_iCurrentLevelIndex = { -1 };			 

public:
	static CLevel_Manager*	Create();
	virtual void			Free() override;
};

NS_END