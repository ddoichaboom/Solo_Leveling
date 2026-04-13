#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class CLIENT_DLL CMapObject final : public CContainerObject
{
public:
	typedef struct tagMapObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{

	}MAPOBJECT_DESC;

private:
	CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMapObject(const CMapObject& Prototype);
	virtual ~CMapObject() = default;

public:
	virtual HRESULT         Initialize_Prototype() override;
	virtual HRESULT         Initialize(void* pArg) override;
	virtual void            Priority_Update(_float fTimeDelta) override;
	virtual void            Update(_float fTimeDelta) override;
	virtual void            Late_Update(_float fTimeDelta) override;
	virtual HRESULT         Render() override;

private:
	HRESULT                 Ready_PartObjects();

public:
	static CMapObject*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;

};

NS_END