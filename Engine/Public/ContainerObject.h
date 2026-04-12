#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CContainerObject abstract : public CGameObject
{
protected:
	CContainerObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CContainerObject(const CContainerObject& Prototype);
	virtual ~CContainerObject() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	const map<const _wstring, class CPartObject*>& Get_PartObjects() const { return m_PartObjects; }

protected:
	map<const _wstring, class CPartObject*>			m_PartObjects;

protected:
	HRESULT					Add_PartObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag,
											const _wstring& strPartTag, void* pArg = nullptr);

public:
	virtual CGameObject*	Clone(void* pArg) PURE;
	virtual void			Free() override;
};

NS_END
