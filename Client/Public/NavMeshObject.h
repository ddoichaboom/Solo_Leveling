#pragma once

#include "Client_Defines.h"
#include "GameObject.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)
class CNavMesh;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CNavMeshObject final : public CGameObject
{
public:
	typedef struct tagNavMeshObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		const NAVMESH_SNAPSHOT* pInitialSnapshot = { nullptr };
	}NAVMESHOBJECT_DESC;

private:
	CNavMeshObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNavMeshObject(const CNavMeshObject& Prototype);
	virtual ~CNavMeshObject() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	CNavMesh*				Get_NavMesh() const { return m_pNavMeshCom; }

private:
	CNavMesh*				m_pNavMeshCom = { nullptr };

private:
	HRESULT					Ready_Components(const NAVMESHOBJECT_DESC* pDesc);

public:
	static CNavMeshObject*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;

};

NS_END