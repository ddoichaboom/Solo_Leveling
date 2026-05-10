#include "Elite_Monster.h"

CElite_Monster::CElite_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMonster{ pDevice, pContext }
{
}

CElite_Monster::CElite_Monster(const CElite_Monster& Prototype)
    : CMonster{ Prototype }
{
}

HRESULT CElite_Monster::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CElite_Monster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    Desc.eSpawnType = SPAWN_TYPE::MONSTER_ELITE;

    if (0.f == Desc.fSpeedPerSec)
        Desc.fSpeedPerSec = 3.5f;

    if (0.f == Desc.fRotationPerSec)
        Desc.fRotationPerSec = XMConvertToRadians(420.f);

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 300.f;

    return __super::Initialize(&Desc);
}

CElite_Monster* CElite_Monster::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CElite_Monster* pInstance = new CElite_Monster(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CElite_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CElite_Monster::Clone(void* pArg)
{
    CElite_Monster* pInstance = new CElite_Monster(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CElite_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CElite_Monster::Free()
{
    __super::Free();
}