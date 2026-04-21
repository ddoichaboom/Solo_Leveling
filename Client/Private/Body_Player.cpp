#include "Body_Player.h"
#include "GameInstance.h"
#include "Player.h"

#include "Model.h"
#include "Shader.h"
#include "AnimController.h"

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

    return m_pModelCom->Get_LastRootMotionDelta();
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
    _bool bFinished = m_pAnimController->Update(fTimeDelta);

    if (bFinished)
    {
        const CHARACTER_ACTION_POLICY* pPolicy = Find_ActionPolicy(m_eCurrentAction);

        if (nullptr != pPolicy && true == pPolicy->bAutoReturn)
            Play_Action(pPolicy->eReturnAction);
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
        TEXT("Prototype_Component_Model_SungJinWoo"),
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
    m_pAnimTable = Find_CharacterAnimTable(m_eAnimSet);

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

    for (size_t i = 0; i < m_pAnimTable->iNumClips; i++)
    {
        const CHARACTER_ANIM_BIND_DESC& Clip = m_pAnimTable->pClips[i];

        CAnimController::ANIM_CLIP_DESC Desc{};
        Desc.pAnimationName = Clip.pAnimationName;
        Desc.bRestartOnEnter = Clip.bRestartOnEnter;

        if (FAILED(m_pAnimController->Register_Clip(
            Make_CharacterAnimKey(Clip.eAction, Clip.eWeapon), Desc)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CBody_Player::Play_Action(CHARACTER_ACTION eAction)
{
    if (nullptr == m_pAnimController || nullptr == m_pAnimTable)
        return E_FAIL;

    const _uint64 iKey = Resolve_ActionKey(eAction);

    if (FAILED(m_pAnimController->Play(iKey)))
        return E_FAIL;

    m_eCurrentAction = eAction;

    return S_OK;
}

_bool CBody_Player::Has_Action(CHARACTER_ACTION eAction, CHARACTER_WEAPON_STATE eWeapon) const
{
    if (nullptr == m_pAnimTable)
        return false;

    for (size_t i = 0; i < m_pAnimTable->iNumClips; i++)
    {
        const CHARACTER_ANIM_BIND_DESC& Clip = m_pAnimTable->pClips[i];

        if (Clip.eAction == eAction && Clip.eWeapon == eWeapon)
            return true;
    }

    return false;
}

_uint64 CBody_Player::Resolve_ActionKey(CHARACTER_ACTION eAction) const
{
    if (Has_Action(eAction, m_eWeaponState))
        return Make_CharacterAnimKey(eAction, m_eWeaponState);

    return Make_CharacterAnimKey(eAction, CHARACTER_WEAPON_STATE::COMMON);
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