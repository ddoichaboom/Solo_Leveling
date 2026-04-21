#pragma once

#include "Client_Defines.h"
#include "PartObject.h"
#include "CharacterAnimTable.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CAnimController;
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

#pragma region PREVIEW BLOCK
public:
    HRESULT                             Begin_Preview(_uint iAnimationIndex);
    HRESULT                             Restart_Preview();
    HRESULT                             End_Preview();

    _bool                               Is_Previewing() const { return m_bPreviewMode; }
    _uint                               Get_PreviewAnimationIndex() const { return m_iPreviewAnimationIndex; }

#pragma endregion
public:
    virtual HRESULT                     Initialize_Prototype() override;
    virtual HRESULT                     Initialize(void* pArg) override;
    virtual void                        Priority_Update(_float fTimeDelta) override;
    virtual void                        Update(_float fTimeDelta) override;
    virtual void                        Late_Update(_float fTimeDelta) override;
    virtual HRESULT                     Render() override;

private:
    CShader*                            m_pShaderCom = { nullptr };
    CModel*                             m_pModelCom = { nullptr };
    CAnimController*                    m_pAnimController = { nullptr };

private:
    const _uint*                        m_pParentState = { nullptr };
    const CHARACTER_ANIM_TABLE_DESC*    m_pAnimTable = { nullptr };

    CHARACTER_ANIM_SET                  m_eAnimSet = { CHARACTER_ANIM_SET::SUNGJINWOO_ERANK };
    CHARACTER_WEAPON_STATE              m_eWeaponState = { CHARACTER_WEAPON_STATE::COMMON };
    CHARACTER_ACTION                    m_eCurrentAction = { CHARACTER_ACTION::IDLE };

    _bool                               m_bPreviewMode = { false };
    _uint                               m_iPreviewAnimationIndex = { static_cast<_uint>(-1) };

private:
    HRESULT                             Ready_Components();
    HRESULT                             Bind_ShaderResources();

    HRESULT                             Ready_AnimationTable();
    HRESULT                             Register_AnimationClips();
    HRESULT                             Play_Action(CHARACTER_ACTION eAction);

    _bool                               Has_Action(CHARACTER_ACTION eAction, CHARACTER_WEAPON_STATE eWeapon) const;
    _uint64                             Resolve_ActionKey(CHARACTER_ACTION eAction) const;
    const CHARACTER_ACTION_POLICY*      Find_ActionPolicy(CHARACTER_ACTION eAction) const;

public:
    static CBody_Player*                Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*                Clone(void* pArg) override;
    virtual void                        Free() override;
};

NS_END