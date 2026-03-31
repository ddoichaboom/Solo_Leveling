#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CLayer final : public CBase
{
private:
	CLayer();
	virtual ~CLayer() = default;

public:
	HRESULT						Add_GameObject(class CGameObject* pGameObject);
	void						Priority_Update(_float fTimeDelta);
	void						Update(_float fTimeDelta);
	void						Late_Update(_float fTimeDelta);

private:
	list<class CGameObject*>	m_GameObjects;

public:
	static CLayer*				Create();
	virtual void				Free() override;
};

NS_END