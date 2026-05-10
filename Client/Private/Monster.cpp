#include "Monster.h"
#include "GameInstance.h"
#include "NavigationAgent.h"
#include "NavMesh.h"
#include "PartObject.h"
#include "Body_Monster.h"

CMonster::CMonster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice, pContext }
{
}

CMonster::CMonster(const CMonster& Prototype)
    : CContainerObject{ Prototype }
{
}

HRESULT CMonster::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    if (SPAWN_TYPE::END == Desc.eSpawnType)
        Desc.eSpawnType = Get_DefaultSpawnType();

    if (MONSTER_ANIM_SET::NONE == Desc.eAnimSet)
        Desc.eAnimSet = Get_DefaultAnimSet();

    if (nullptr == Desc.pBodyModelPrototypeTag)
        Desc.pBodyModelPrototypeTag = Get_DefaultBodyModelPrototypeTag();

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 1.f;

    m_strName = Get_DefaultName();
    m_strTag = Get_DefaultName();

    m_eSpawnType = Desc.eSpawnType;
    m_eAnimSet = Desc.eAnimSet;

    m_fMaxHP = Desc.fMaxHP;
    m_fCurrentHP = Desc.fMaxHP;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components(Desc)))
        return E_FAIL;

    if (FAILED(Ready_PartObjects(Desc)))
        return E_FAIL;

    return S_OK;
}

void CMonster::Priority_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Priority_Update(fTimeDelta);
    }
}

void CMonster::Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Update(fTimeDelta);
    }
}

void CMonster::Late_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Late_Update(fTimeDelta);
    }
}

HRESULT CMonster::Render()
{
    return S_OK;
}

HRESULT CMonster::Ready_Components(const MONSTER_DESC& Desc)
{
    CNavigationAgent::NAVIGATION_AGENT_DESC NavigationDesc{};
    NavigationDesc.pNavMesh = Desc.pNavMesh;
    NavigationDesc.iStartCellIndex = Desc.iStartCellIndex;

    if (FAILED(__super::Add_Component(
        ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_Component_NavigationAgent"),
        TEXT("Com_NavigationAgent"),
        reinterpret_cast<CComponent**>(&m_pNavigationAgent),
        &NavigationDesc)))
        return E_FAIL;

    if (nullptr != m_pNavigationAgent &&
        m_pNavigationAgent->Has_NavMesh() &&
        NAVMESH_INVALID_INDEX == m_pNavigationAgent->Get_CurrentCellIndex())
    {
        _float3 vPosition{};
        XMStoreFloat3(&vPosition, m_pTransformCom->Get_State(STATE::POSITION));
        m_pNavigationAgent->Find_CurrentCell(vPosition);
    }

    return S_OK;
}

HRESULT CMonster::Ready_PartObjects(const MONSTER_DESC& Desc)
{
    if (nullptr == Desc.pBodyModelPrototypeTag)
        return S_OK;

    CBody_Monster::BODY_MONSTER_DESC BodyDesc{};
    BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    BodyDesc.pModelPrototypeTag = Desc.pBodyModelPrototypeTag;
    BodyDesc.eAnimSet = Desc.eAnimSet;

    if (FAILED(__super::Add_PartObject(
        ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_GameObject_Body_Monster"),
        TEXT("Body"),
        &BodyDesc)))
        return E_FAIL;

    m_pBody = dynamic_cast<CBody_Monster*>(m_PartObjects[TEXT("Body")]);
    Safe_AddRef(m_pBody);

    return S_OK;
}

_bool CMonster::Try_ApplyNavigationPosition(const _float3& vCandidatePosition)
{
    if (nullptr == m_pTransformCom)
        return false;

    if (nullptr == m_pNavigationAgent || false == m_pNavigationAgent->Has_NavMesh())
    {
        m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&vCandidatePosition), 1.f));
        return true;
    }

    _float3 vAdjustedPosition{};
    if (false == m_pNavigationAgent->Try_Move(vCandidatePosition, &vAdjustedPosition))
        return false;

    m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&vAdjustedPosition), 1.f));
    return true;
}

void CMonster::Free()
{
    __super::Free();

    Safe_Release(m_pBody);
    Safe_Release(m_pNavigationAgent);
}
