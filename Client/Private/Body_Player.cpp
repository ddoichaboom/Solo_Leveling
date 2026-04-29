#include "Body_Player.h"
#include "GameInstance.h"
#include "Player.h"

#include "Model.h"
#include "Shader.h"
#include "AnimController.h"
#include "NotifyListener.h"

CBody_Player::CBody_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPartObject { pDevice, pContext }
{
}

CBody_Player::CBody_Player(const CBody_Player& Prototype)
	: CPartObject{ Prototype }
{
}

const _float4x4* CBody_Player::Get_BoneMatrixPtr(const _char* pBoneName) const
{
	return m_pModelCom->Get_BoneMatrixPtr(pBoneName);
}

_float3   CBody_Player::Get_LastRootMotionDelta() const
{
    if (nullptr == m_pModelCom)
        return _float3{};

    if (true == m_bPreviewMode)
        return _float3{};

    return m_pModelCom->Get_LastRootMotionDelta();
}

HRESULT CBody_Player::Begin_Preview(_uint iAnimationIndex)
{
    if (nullptr == m_pModelCom)
        return E_FAIL;

    if (iAnimationIndex >= m_pModelCom->Get_NumAnimations())
        return E_FAIL;

    m_bPreviewMode = true;
    m_iPreviewAnimationIndex = iAnimationIndex;

    m_pModelCom->Set_AnimationIndex(iAnimationIndex);
    m_pModelCom->Restart_Animation();
    m_pModelCom->Set_AnimationPlaying(true);

    return S_OK;
}

HRESULT CBody_Player::Restart_Preview()
{
    if (nullptr == m_pModelCom)
        return E_FAIL;

    if (false == m_bPreviewMode)
        return E_FAIL;

    if (static_cast<_uint>(-1) == m_iPreviewAnimationIndex)
        return E_FAIL;

    m_pModelCom->Restart_Animation();
    m_pModelCom->Set_AnimationPlaying(true);

    return S_OK;
}

HRESULT CBody_Player::End_Preview()
{
    if (false == m_bPreviewMode)
        return S_OK;

    m_bPreviewMode = false;
    m_iPreviewAnimationIndex = static_cast<_uint>(-1);

    return Play_Action(CHARACTER_ACTION::IDLE);
}

HRESULT CBody_Player::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CBody_Player::Initialize(void* pArg)
{
    auto pDesc = static_cast<BODY_PLAYER_DESC*>(pArg);

    m_pParentState = pDesc->pParentState;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    if (FAILED(Ready_AnimationTable()))
        return E_FAIL;

    if (FAILED(Play_Action(CHARACTER_ACTION::IDLE)))
        return E_FAIL;

    return S_OK;
}

void CBody_Player::Priority_Update(_float fTimeDelta)
{
}

void CBody_Player::Update(_float fTimeDelta)
{
    if (true == m_bPreviewMode)
    {
        if (nullptr != m_pModelCom)
            m_pModelCom->Play_Animation(fTimeDelta);

        return;
    }

    _bool bFinished = m_pAnimController->Update(fTimeDelta);

    if (bFinished && nullptr != m_pListener)
    {
        NOTIFY_EVENT Event{};
        Event.eType = NOTIFY_TYPE::ACTION_FINISHED;
        Event.iPayload = ETOUI(m_eCurrentAction);
        Event.pData = nullptr;

        m_pListener->OnNotify(Event);
    }
}

void CBody_Player::Late_Update(_float fTimeDelta)
{
    __super::Compute_CombinedWorldMatrix(XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()));

    m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CBody_Player::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    size_t iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, "g_DiffuseTexture", i, TEXTURE_TYPE::DIFFUSE, 0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        if (FAILED(m_pShaderCom->Begin(0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Render(i)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CBody_Player::Ready_Components()
{
    if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_Component_Model_SungJinWoo_OverDrive"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(ETOUI(LEVEL::STATIC),
        TEXT("Prototype_Component_AnimController"),
        TEXT("Com_AnimController"),
        reinterpret_cast<CComponent**>(&m_pAnimController))))
        return E_FAIL;


    return S_OK;
}

HRESULT CBody_Player::Bind_ShaderResources()
{
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform(D3DTS::VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform(D3DTS::PROJ))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4))))
        return E_FAIL;

    const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (nullptr == pLightDesc)
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4))))
        return E_FAIL;

    return S_OK;
}

HRESULT CBody_Player::Ready_AnimationTable()
{
    m_pAnimTable = Find_CharacterAnimTable(m_eCharacterType);

    if (nullptr == m_pAnimTable)
        return E_FAIL;

    if (nullptr == m_pAnimController || nullptr == m_pModelCom)
        return E_FAIL;

    if (nullptr != m_pAnimTable->pRootBoneName)
        m_pModelCom->Set_RootBoneName(m_pAnimTable->pRootBoneName);

    if (FAILED(m_pAnimController->Bind_Model(m_pModelCom)))
        return E_FAIL;

    if (FAILED(Register_AnimationClips()))
        return E_FAIL;

    return S_OK;
}

HRESULT CBody_Player::Register_AnimationClips()
{
    if (nullptr == m_pAnimTable)
        return E_FAIL;

    for (size_t i = 0; i < m_pAnimTable->iNumBinds; i++)
    {
        const CHARACTER_ANIM_BIND_DESC& Bind = m_pAnimTable->pBinds[i];

        CAnimController::ANIM_CLIP_DESC Desc{};
        Desc.pAnimationName = Bind.pAnimationName;
        Desc.bRestartOnEnter = Bind.bRestartOnEnter;

        if (FAILED(m_pAnimController->Register_Clip(
            Make_CharacterAnimKey(Bind.eState, Bind.eAction, Bind.eWeapon), Desc)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CBody_Player::Play_Action(CHARACTER_ACTION eAction)
{
    if (nullptr == m_pAnimController || nullptr == m_pAnimTable)
        return E_FAIL;

    const _uint64 iKey = Resolve_ActionKey(eAction);

    const CHARACTER_ACTION_POLICY* pPolicy = Find_ActionPolicy(eAction);
    const _float fBlendTime = (nullptr != pPolicy) ? pPolicy->fEnterBlendTime : 0.f;

    if (FAILED(m_pAnimController->Play(iKey, fBlendTime)))
        return E_FAIL;

    m_eCurrentAction = eAction;

    return S_OK;
}

CHARACTER_ACTION CBody_Player::Pick_RunEndAction() const
{
    // Foot(발) 본의 모델 로컬 Z값 비교 - 더 작은 z (뒤쪽에 위치) 쪽 발로 멈춤
    if (nullptr == m_pModelCom)
        return CHARACTER_ACTION::RUN_END;      // Clip Run_End에 바인딩할지 고민

    const _float4x4* pLFoot = m_pModelCom->Get_BoneMatrixPtr("FX_Point_L_Foot");
    const _float4x4* pRFoot = m_pModelCom->Get_BoneMatrixPtr("FX_Point_R_Foot");

    if (nullptr == pLFoot || nullptr == pRFoot)
        return CHARACTER_ACTION::RUN_END;

    return (pLFoot->_43 < pRFoot->_43)
        ? CHARACTER_ACTION::RUN_END_LEFT
        : CHARACTER_ACTION::RUN_END_RIGHT;
}

const CHARACTER_ANIM_BIND_DESC* CBody_Player::Find_Bind(CHARACTER_STATE eState, CHARACTER_ACTION eAction, WEAPON_TYPE eWeapon) const
{
    if (nullptr == m_pAnimTable)
        return nullptr;

    for (size_t i = 0; i < m_pAnimTable->iNumBinds; ++i)
    {
        const CHARACTER_ANIM_BIND_DESC& Bind = m_pAnimTable->pBinds[i];

        if (Bind.eState == eState && Bind.eAction == eAction && Bind.eWeapon == eWeapon)
            return &Bind;
    }

    return nullptr;
}

_uint64 CBody_Player::Resolve_ActionKey(CHARACTER_ACTION eAction) const
{
    if (nullptr != Find_Bind(m_eCurrentState, eAction, m_eWeaponState))
        return Make_CharacterAnimKey(m_eCurrentState, eAction, m_eWeaponState);

    return Make_CharacterAnimKey(m_eCurrentState, eAction, WEAPON_TYPE::DEFAULT);
}

const CHARACTER_ACTION_POLICY* CBody_Player::Find_ActionPolicy(CHARACTER_ACTION eAction) const
{
    if (nullptr == m_pAnimTable)
        return nullptr;

    for (size_t i = 0; i < m_pAnimTable->iNumPolicies; i++)
    {
        const CHARACTER_ACTION_POLICY& Policy = m_pAnimTable->pPolicies[i];

        if (Policy.eAction == eAction)
            return &Policy;
    }

    return nullptr;
}

CBody_Player* CBody_Player::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBody_Player* pInstance = new CBody_Player(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CBody_Player");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CBody_Player::Clone(void* pArg)
{
    CBody_Player* pInstance = new CBody_Player(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBody_Player");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBody_Player::Free()
{
    __super::Free();

    Safe_Release(m_pAnimController);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);    
}