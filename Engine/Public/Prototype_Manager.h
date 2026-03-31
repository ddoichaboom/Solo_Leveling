#pragma once

#include "Model.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "VIBuffer_Terrain.h"

// 1. 원형 객체(CGameObject, CComponent)를 보관한다.
// 2. 선택된 원형 객체를 복제하여 반환해준다.

NS_BEGIN(Engine)

class CPrototype_Manager final : public CBase
{
private:
	CPrototype_Manager();
	virtual ~CPrototype_Manager() = default;

public:
	HRESULT										Initialize(_uint iNumLevels);
	HRESULT										Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype);
	CBase*										Clone_Prototype(PROTOTYPE eType, _uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg);
	void										Clear(_uint iLevelIndex);

private:
	size_t										m_iNumLevels = {};
	map<const _wstring, class CBase*>*			m_pPrototypes = { nullptr };
	typedef map<const _wstring, class CBase*>	PROTOTYPES;

private:
	CBase*										Find_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag);

public:
	static CPrototype_Manager*					Create(_uint iNumLevels);
	virtual void								Free() override;
};

NS_END