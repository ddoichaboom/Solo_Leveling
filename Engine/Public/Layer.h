#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CGameObject;

class CLayer final : public CBase
{
private:
	CLayer();
	virtual ~CLayer() = default;

public:
	const list<CGameObject*>&	Get_GameObjects() const 
	{ 
		return m_GameObjects; 
	}

public:
	HRESULT						Add_GameObject(CGameObject* pGameObject);
	void						Priority_Update(_float fTimeDelta);
	void						Update(_float fTimeDelta);
	void						Late_Update(_float fTimeDelta);

private:
	list<CGameObject*>	m_GameObjects;

public:
	static CLayer*				Create();
	virtual void				Free() override;
};

NS_END