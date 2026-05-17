#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)
class CNavMesh;
class CNavigationAgent;
class CCollider;
NS_END

NS_BEGIN(Client)

class CBody_Monster;
class CWeapon;
class CMonster_StateMachine;

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
		const _tchar*		pWeaponModelPrototypeTag = { nullptr };
		const _char*		pWeaponSocketBoneName = { nullptr };
		_bool				bWeaponInitiallyVisible = { true };

		_float				fMaxHP = { 0.f };
	}MONSTER_DESC;

protected:
	CMonster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster(const CMonster& Prototype);
	virtual ~CMonster() = default;

public:
	SPAWN_TYPE					Get_SpawnType() const { return m_eSpawnType; }
	MONSTER_ANIM_SET			Get_AnimSet() const { return m_eAnimSet; }

	_float						Get_MaxHP() const { return m_fMaxHP; }
	_float						Get_CurrentHP() const { return m_fCurrentHP; }

	void						Take_Damage(_float fAmount);

	void						Handle_ActionTransition(MONSTER_ACTION eFromAction, MONSTER_ACTION_STEP eFromStep,
														MONSTER_ACTION eToAction, MONSTER_ACTION_STEP eToStep,
														_bool bInitial);

	void						Set_WeaponHitboxActive(_bool bActive);

#ifdef _DEBUG
public:
	void						Debug_TryAction(MONSTER_ACTION eAction, MONSTER_ACTION_STEP eStep = MONSTER_ACTION_STEP::NONE);
#endif


public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Priority_Update(_float fTimeDelta) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

protected:
	virtual const _tchar*		Get_DefaultName() const { return TEXT("Monster"); }
	virtual SPAWN_TYPE			Get_DefaultSpawnType() const { return SPAWN_TYPE::END; }
	virtual MONSTER_ANIM_SET	Get_DefaultAnimSet() const { return MONSTER_ANIM_SET::NONE; }
	virtual const _tchar*		Get_DefaultBodyModelPrototypeTag() const { return nullptr; }
	virtual const _tchar*		Get_DefaultWeaponModelPrototypeTag() const { return nullptr; }
	virtual const _char*		Get_DefaultWeaponSocketBoneName() const { return nullptr; }


protected:
	HRESULT						Ready_Components(const MONSTER_DESC& Desc);
	virtual HRESULT				Ready_PartObjects(const MONSTER_DESC& Desc);
	HRESULT						Ready_StateMachine();

	_bool						Try_ApplyNavigationPosition(const _float3& vCandidatePosition);
	void						Apply_RootMotion(const _float3& vLocalDelta);
	
	void						On_WeaponHitEnter(CCollider* pOther);

protected:
	CNavigationAgent*			m_pNavigationAgent = { nullptr };
	CBody_Monster*				m_pBody = { nullptr };
	CWeapon*					m_pWeapon = { nullptr };
	CCollider*					m_pCollider = { nullptr };
	CMonster_StateMachine*		m_pStateMachine = { nullptr };

	SPAWN_TYPE					m_eSpawnType = { SPAWN_TYPE::END };
	MONSTER_ANIM_SET			m_eAnimSet = { MONSTER_ANIM_SET::NONE };

	_float						m_fMaxHP = { 1.f };
	_float						m_fCurrentHP = { 1.f };

	_float						m_fMaxShield = { 1.f };
	_float						m_fCurrentShield = { 1.f };

	set<CGameObject*>			m_AttackHitTargets;

public:
	virtual CGameObject*		Clone(void* pArg) PURE;
	virtual void				Free() override;
};

NS_END

