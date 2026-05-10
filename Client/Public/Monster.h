#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)
class CNavMesh;
class CNavigationAgent;
NS_END

NS_BEGIN(Client)

class CBody_Monster;

class CLIENT_DLL CMonster abstract : public CContainerObject
{
public:
	typedef struct tagMonsterDesc : public CGameObject::GAMEOBJECT_DESC
	{
		CNavMesh*			pNavMesh = { nullptr };
		_int				iStartCellIndex = { NAVMESH_INVALID_INDEX };

		SPAWN_TYPE			eSpawnType = { SPAWN_TYPE::END };
		MONSTER_ANIM_SET	eAnimSet = { MONSTER_ANIM_SET::NONE };
		const _tchar*		pBodyModelPrototypeTag = { nullptr };

		_float				fMaxHP = { 0.f };
	}MONSTER_DESC;

protected:
	CMonster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster(const CMonster& Prototype);
	virtual ~CMonster() = default;

public:
	SPAWN_TYPE				Get_SpawnType() const { return m_eSpawnType; }
	MONSTER_ANIM_SET		Get_AnimSet() const { return m_eAnimSet; }

	_float					Get_MaxHP() const { return m_fMaxHP; }
	_float					Get_CurrentHP() const { return m_fCurrentHP; }

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

protected:
	virtual const _tchar*	Get_DefaultName() const { return TEXT("Monster"); }
	virtual SPAWN_TYPE		Get_DefaultSpawnType() const { return SPAWN_TYPE::END; }
	virtual MONSTER_ANIM_SET	Get_DefaultAnimSet() const { return MONSTER_ANIM_SET::NONE; }
	virtual const _tchar*	Get_DefaultBodyModelPrototypeTag() const { return nullptr; }

protected:
	HRESULT					Ready_Components(const MONSTER_DESC& Desc);
	virtual HRESULT			Ready_PartObjects(const MONSTER_DESC& Desc);

	_bool					Try_ApplyNavigationPosition(const _float3& vCandidatePosition);

protected:
	CNavigationAgent*		m_pNavigationAgent = { nullptr };
	CBody_Monster*			m_pBody = { nullptr };

	SPAWN_TYPE				m_eSpawnType = { SPAWN_TYPE::END };
	MONSTER_ANIM_SET		m_eAnimSet = { MONSTER_ANIM_SET::NONE };

	_float					m_fMaxHP = { 1.f };
	_float					m_fCurrentHP = { 1.f };

public:
	virtual CGameObject*		Clone(void* pArg) PURE;
	virtual void			Free() override;
};

NS_END

