#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class CBody_Player;
class CWeapon;
class CIntentResolver;
class CPlayer_StateMachine;

class CLIENT_DLL CPlayer final : public CContainerObject
{
public:
    typedef struct tagPlayerDesc : public CGameObject::GAMEOBJECT_DESC
    {

    }PLAYER_DESC;

private:
    CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer(const CPlayer& Prototype);
    virtual ~CPlayer() = default;

public:
    _float                  Get_SpeedCoeff() const { return m_fSpeedCoeff; }
    void                    Set_SpeedCoeff(_float fCoeff) { m_fSpeedCoeff = fCoeff; }

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
    _bool                   Can_ConsumeDashCharge() const { return m_iDashChargeCurent > 0; }
    _bool                   Consume_DashCharge();
    _int                    Get_DashCharge() const { return m_iDashChargeCurent; }
    _int                    Get_DashChargeMax() const { return m_iDashChargeMax; }

    void                    Set_WeaponsVisible(_bool bVisible);
    _bool                   Is_WeaponsVisible() const { return m_bWeaponsVisible; }

    void                    Tick_DashRegen(_float fTimeDelta);
    void                    Tick_WeaponHideTimer(_float fTimeDelta);

private:
    _uint                   m_iState = {};
    CBody_Player*           m_pBody = { nullptr };
    CWeapon*                m_pWeaponR = { nullptr };
    CWeapon*                m_pWeaponL = { nullptr };
    CIntentResolver*        m_pIntentResolver = { nullptr };
    CPlayer_StateMachine*   m_pStateMachine = { nullptr };

    EQUIPPED_WEAPON_ID      m_eEquippedWeapon = { EQUIPPED_WEAPON_ID::NONE };

private:
    HRESULT                 Ready_PartObjects();
    HRESULT                 Ready_StateMachine();
    void                    Gather_RawInput(PLAYER_RAW_INPUT_FRAME* pOutRaw);
    void                    Apply_MoveIntent(const PLAYER_INTENT_FRAME& Intent, _float fTimeDelta);

    _float                  Query_CameraYaw() const;
    void                    Apply_Loadout();

    void                    Refresh_WeaponVisibility();

private:
    _bool                   m_bWeaponsVisible = { true };
    _bool                   m_bLeftVisibleFromLoadOut = { true };

    _float                  m_fIdleTimer = { 0.f };
    _float                  m_fIdleThreshold = { 3.f };

    _int                    m_iDashChargeMax = { 3 };
    _int                    m_iDashChargeCurent = { 3 };
    _float                  m_fDashRegenInterval = { 3.f }; //  ∏Æ¡® ¡÷±‚
    _float                  m_fDashRegenTimer = { 0.f };

    _float                  m_fSpeedCoeff = { 0.f };

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END