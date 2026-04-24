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
    virtual HRESULT         Initialize_Prototype() override;
    virtual HRESULT         Initialize(void* pArg) override;
    virtual void            Priority_Update(_float fTimeDelta) override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Late_Update(_float fTimeDelta) override;
    virtual HRESULT         Render() override;

public:
    void                    Apply_RootMotion(const _float3& vLocalDelta);
    void                    Handle_ActionTransition(CHARACTER_ACTION eFrom, CHARACTER_ACTION eTo, _bool bInitial);

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

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END