#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class CBody_Player;
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

public:
    _bool                   Can_ConsumeDashCharge() const { return m_iDashChargeCurent > 0; }
    _bool                   Consume_DashCharge();
    _int                    Get_DashCharge() const { return m_iDashChargeCurent; }
    _int                    Get_DashChargeMax() const { return m_iDashChargeMax; }

    void                    Tick_DashRegen(_float fTimeDelta);

private:
    _uint                   m_iState = {};
    CBody_Player*           m_pBody = { nullptr };
    CIntentResolver*        m_pIntentResolver = { nullptr };
    CPlayer_StateMachine*   m_pStateMachine = { nullptr };

private:
    HRESULT                 Ready_PartObjects();
    HRESULT                 Ready_StateMachine();
    void                    Gather_RawInput(PLAYER_RAW_INPUT_FRAME* pOutRaw);
    void                    Apply_MoveIntent(const PLAYER_INTENT_FRAME& Intent, _float fTimeDelta);

    _float                  Query_CameraYaw() const;

private:
    _int                    m_iDashChargeMax = { 3 };
    _int                    m_iDashChargeCurent = { 3 };
    _float                  m_fDashRegenInterval = { 3.f }; //  ∏Æ¡® ¡÷±‚
    _float                  m_fDashRegenTimer = { 0.f };

    _float                  m_fSpeedCoeff = { 1.f };

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END