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

class CBody_Player;
class CWeapon;
class CIntentResolver;
class CPlayer_StateMachine;
class CMonster;

class CLIENT_DLL CPlayer final : public CContainerObject
{
public:
    typedef struct tagPlayerDesc : public CGameObject::GAMEOBJECT_DESC
    {
        CNavMesh* pNavMesh = { nullptr };
        _int  iStartCellIndex = { NAVMESH_INVALID_INDEX };
    }PLAYER_DESC;

private:
    CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer(const CPlayer& Prototype);
    virtual ~CPlayer() = default;

public:
    _float                  Get_SpeedCoeff() const { return m_fSpeedCoeff; }
    void                    Set_SpeedCoeff(_float fCoeff) { m_fSpeedCoeff = fCoeff; }

    _float                  Get_MaxHP() const { return m_fMaxHP; }
    _float                  Get_CurrentHP() const { return m_fCurrentHP; }

    _float                  Get_MaxMP() const { return m_fMaxMP; }
    _float                  Get_CurrentMP() const { return m_fCurrentMP; }

    void                    Take_Damage(_float fAmount);
    _bool                   Try_GetDashHUDWorldPosition(_float3* pOutPosition) const;

public:
    virtual HRESULT         Initialize_Prototype() override;
    virtual HRESULT         Initialize(void* pArg) override;
    virtual void            Priority_Update(_float fTimeDelta) override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Late_Update(_float fTimeDelta) override;
    virtual HRESULT         Render() override;

public:
    void                    Apply_RootMotion(const _float3& vLocalDelta);
    void                    Handle_ActionTransition(CHARACTER_ACTION eFrom, CHARACTER_ACTION eTo, _bool bInitial);
    void                    Face_DirectionImmediately(const _float3& vDirWorld);
    CHARACTER_ACTION        Pick_RunEndByFoot() const;
    CHARACTER_ACTION        Pick_RunFastVariant(const _float3& vMoveDirWorld, CHARACTER_ACTION eCurrent) const;

    void                    Set_EquippedWeapon(EQUIPPED_WEAPON_ID eId);
    EQUIPPED_WEAPON_ID      Get_EquippedWeapon() const { return m_eEquippedWeapon; }
    _bool                   Can_UseWeaponSkill() const { return m_eEquippedWeapon != EQUIPPED_WEAPON_ID::NONE; }

public:
    _bool                   Can_ConsumeDashCharge() const { return m_iDashChargeCurrent > 0; }
    _bool                   Consume_DashCharge();
    _int                    Get_DashCharge() const { return m_iDashChargeCurrent; }
    _int                    Get_DashChargeMax() const { return m_iDashChargeMax; }

    void                    Set_WeaponsVisible(_bool bVisible);
    _bool                   Is_WeaponsVisible() const { return m_bWeaponsVisible; }

    void                    Tick_DashRegen(_float fTimeDelta);
    void                    Tick_WeaponHideTimer(_float fTimeDelta);

    void                    Enter_FloatReaction(CHARACTER_ACTION eFloatAction);


private:
    _uint                   m_iState = {};
    CBody_Player*           m_pBody = { nullptr };
    CWeapon*                m_pWeaponR = { nullptr };
    CWeapon*                m_pWeaponL = { nullptr };
    CIntentResolver*        m_pIntentResolver = { nullptr };
    CPlayer_StateMachine*   m_pStateMachine = { nullptr };
    CCollider*              m_pCollider = { nullptr };

    set<pair<class CWeapon*, CGameObject*>>       m_AttackHitTargets;

private:
    HRESULT                 Ready_PartObjects();
    HRESULT                 Ready_StateMachine();
    HRESULT                 Ready_Components(const PLAYER_DESC& Desc);

    _bool                   Resolve_NavigationPosition(const _float3& vCandidatePosition, _float3* pOutPosition);
    _bool                   Resolve_BodyBlockingPosition(const _float3& vCurrentPosition, const _float3& vCandidatePosition, _float3* pOutPosition);
    _bool                   Resolve_BodyOverlapPosition(const _float3& vPosition, _float3* pOutPosition) const;
    void                    Resolve_BodyBlockOverlap();

    _bool                   Try_ApplyMovementPosition(const _float3& vCandidatePosition);

    BODY_BLOCK_POLICY       Get_BodyBlockPolicy() const;
    _float                  Get_MonsterBodyBlockRadius(const CMonster* pMonster) const;
    void                    Add_BodyBlockCandidateCell(_int* pCandidateCells,
                                                        _uint* pNumCandidateCells,
                                                        _int iCellIndex) const;
    _bool                   Contains_BodyBlockCandidateCell(const _int* pCandidateCells,
                                                            _uint iNumCandidateCells,
                                                            _int iCellIndex) const;
    void                    Collect_BodyBlockCandidateCells(const CNavMesh* pNavMesh,
                                                            _int iCellIndex,
                                                            _int* pCandidateCells,
                                                            _uint* pNumCandidateCells) const;
    _bool                   Clip_SegmentByCircleXZ(const _float3& vCurrentPosition,
                                                    const _float3& vCandidatePosition,
                                                    const _float3& vCircleCenter,
                                                    _float fRadius,
                                                    _float* pOutT) const;

    void                    Gather_RawInput(PLAYER_RAW_INPUT_FRAME* pOutRaw);
    void                    Apply_MoveIntent(const PLAYER_INTENT_FRAME& Intent, _float fTimeDelta);

    _float                  Query_CameraYaw() const;
    void                    Apply_Loadout();

    void                    Refresh_WeaponVisibility();
    void                    Update_WeaponHitboxes();

    void                    On_WeaponHitEnter(CWeapon* pSourceWeapon, CCollider* pOther);

    const WEAPON_INFO*      Find_WeaponInfo(EQUIPPED_WEAPON_ID eId);



private:
    CNavigationAgent*       m_pNavigationAgent = { nullptr };

private:
    _float                  m_fIdleThreshold = { 3.f };

    _bool                   m_bWeaponsVisible = { false };
    _bool                   m_bLeftVisibleFromLoadOut = { true };

    _float                  m_fAttackBufferTimer = { 0.f };
    _float                  m_fIdleTimer = { 0.f };
    static constexpr _float ATTACK_BUFFER_DURATION = { 0.18f };

    _float                  m_fGuardHoldGraceTimer = { 0.f };
    static constexpr _float GUARD_HOLD_GRACE = { 0.10f };

    _int                    m_iDashChargeMax = { 3 };
    _int                    m_iDashChargeCurrent = { 3 };
    _float                  m_fDashRegenInterval = { 3.f }; //  ∏Æ¡® ¡÷±‚
    _float                  m_fDashRegenTimer = { 0.f };

    _float                  m_fSpeedCoeff = { 0.f };

    EQUIPPED_WEAPON_ID      m_eEquippedWeapon = { EQUIPPED_WEAPON_ID::NONE };
    _bool                   m_bPrevAttackHitboxActive = { false };
    _uint                   m_iPrevAttackHitboxWindowSerial = { 0 };

    _float                  m_fMaxHP = { 100.f };
    _float                  m_fCurrentHP = { 100.f };

    _float                  m_fMaxMP = { 100.f };
    _float                  m_fCurrentMP = { 100.f };

    static constexpr _uint  BODY_BLOCK_MAX_CANDIDATE_CELLS = { 16 };

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END