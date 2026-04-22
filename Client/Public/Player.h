#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class CBody_Player;

class CLIENT_DLL CPlayer final : public CContainerObject
{
public:
    // юс╫ц
    enum PLAYER_STATE {
        IDLE    = 0x00000001,
        RUN     = 0x00000002,
        ATTACK  = 0x00000004,
    };

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

private:
    _uint                   m_iState = { PLAYER_STATE::IDLE };
    CBody_Player*           m_pBody = { nullptr };

private:
    HRESULT                 Ready_PartObjects();

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END