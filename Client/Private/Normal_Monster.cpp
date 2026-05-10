#include "Normal_Monster.h"

CNormal_Monster::CNormal_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMonster{ pDevice, pContext }
{
}

CNormal_Monster::CNormal_Monster(const CNormal_Monster& Prototype)
    : CMonster{ Prototype }
{
}

HRESULT CNormal_Monster::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CNormal_Monster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    Desc.eSpawnType = SPAWN_TYPE::MONSTER_NORMAL;

    if (0.f == Desc.fSpeedPerSec)
        Desc.fSpeedPerSec = 3.f;

    if (0.f == Desc.fRotationPerSec)
        Desc.fRotationPerSec = XMConvertToRadians(360.f);

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 100.f;

    return __super::Initialize(&Desc);
}

CNormal_Monster* CNormal_Monster::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CNormal_Monster* pInstance = new CNormal_Monster(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CNormal_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CNormal_Monster::Clone(void* pArg)
{
    CNormal_Monster* pInstance = new CNormal_Monster(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CNormal_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CNormal_Monster::Free()
{
    __super::Free();
}