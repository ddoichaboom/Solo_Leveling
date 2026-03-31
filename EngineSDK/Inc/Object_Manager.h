#pragma once

#include "Base.h"

// 1. 실제 동작을 할 객체(사본 객체)를 보관한다.
// 1-1. 레벨 별로 구분짓는다.
// 1-2. 레이어별로 그룹핑한다.

// 2. 보관하고 있는 객체들의 갱신(Update, Tick)을 수행해준다.

NS_BEGIN(Engine)

class CObject_Manager final : public CBase
{
private:
	CObject_Manager();
	virtual ~CObject_Manager() = default;

public:
	HRESULT										Initialize(_uint iNumLevels);
	//HRESULT									Add_GameObject(_uint iLevelIndex, const _wstring& strLayerTag, class CGameObject* pGameObject);
	HRESULT										Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag,
																_uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg);
	void										Priority_Update(_float fTimeDelta);
	void										Update(_float fTimeDelta);
	void										Late_Update(_float fTimeDelta);
	void										Clear(_uint iLevelIndex);

private:
	size_t										m_iNumLevels = {};
	map<const _wstring, class CLayer*>*			m_pLayers = { nullptr };
	typedef map<const _wstring, class CLayer*>	LAYERS;

	class CGameInstance*						m_pGameInstance = { nullptr };

private:
	class CLayer*								Find_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag);

public:
	static CObject_Manager*						Create(_uint iNumLevels);
	virtual void								Free() override;
};

NS_END