#pragma once

#include "Client_Defines.h"
#include "PartObject.h"
#include "CharacterAnimTable.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CAnimController;
class INotifyListener;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CBody_Player final : public CPartObject
{
public:
	typedef struct tagBodyPlayerDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _uint* pParentState = { nullptr };
	}BODY_PLAYER_DESC;

private:
    CBody_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBody_Player(const CBody_Player& Prototype);
    virtual ~CBody_Player() = default;

public:
    const _float4x4*                    Get_BoneMatrixPtr(const _char* pBoneName) const;
    _float3                             Get_LastRootMotionDelta() const;

public:
    virtual HRESULT                     Initialize_Prototype() override;
    virtual HRESULT                     Initialize(void* pArg) override;
    virtual void                        Priority_Update(_float fTimeDelta) override;
    virtual void                        Update(_float fTimeDelta) override;
    virtual void                        Late_Update(_float fTimeDelta) override;
    virtual HRESULT                     Render() override;

public:
    HRESULT                             Play_Action(CHARACTER_ACTION eAction);
    void                                Set_Listener(INotifyListener* pListener);
    CHARACTER_ACTION                    Pick_RunEndAction() const;

private:
    CShader*                            m_pShaderCom = { nullptr };
    CModel*                             m_pModelCom = { nullptr };
    CAnimController*                    m_pAnimController = { nullptr };

private:
    const _uint*                        m_pParentState = { nullptr };
    const CHARACTER_ANIM_TABLE_DESC*    m_pAnimTable = { nullptr };

    CHARACTER_TYPE                      m_eCharacterType = { CHARACTER_TYPE::SUNGJINWOO_OVERDRIVE };
    CHARACTER_STATE                     m_eCurrentState = { CHARACTER_STATE::LOCOMOTION };
    WEAPON_TYPE                         m_eWeaponState = { WEAPON_TYPE::DEFAULT };
    CHARACTER_ACTION                    m_eCurrentAction = { CHARACTER_ACTION::IDLE };

    INotifyListener*                    m_pListener = { nullptr };

private:
    HRESULT                             Ready_Components();
    HRESULT                             Bind_ShaderResources();

    HRESULT                             Ready_AnimationTable();
    HRESULT                             Register_AnimationClips();

    const CHARACTER_ANIM_BIND_DESC*     Find_Bind(CHARACTER_STATE eState, CHARACTER_ACTION eAction, WEAPON_TYPE  eWeapon) const;
    _uint64                             Resolve_ActionKey(CHARACTER_ACTION eAction);
    const CHARACTER_ACTION_POLICY*      Find_ActionPolicy(CHARACTER_ACTION eAction) const;

public:
    static CBody_Player*                Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*                Clone(void* pArg) override;
    virtual void                        Free() override;
};

NS_END