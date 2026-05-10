#include "Boss_Monster.h"

CBoss_Monster::CBoss_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMonster{ pDevice, pContext }
{
}

CBoss_Monster::CBoss_Monster(const CBoss_Monster& Prototype)
    : CMonster{ Prototype }
{
}

HRESULT CBoss_Monster::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CBoss_Monster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    Desc.eSpawnType = SPAWN_TYPE::MONSTER_BOSS;

    if (0.f == Desc.fSpeedPerSec)
        Desc.fSpeedPerSec = 4.f;

    if (0.f == Desc.fRotationPerSec)
        Desc.fRotationPerSec = XMConvertToRadians(360.f);

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 1000.f;

    return __super::Initialize(&Desc);
}

CBoss_Monster* CBoss_Monster::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBoss_Monster* pInstance = new CBoss_Monster(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CBoss_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CBoss_Monster::Clone(void* pArg)
{
    CBoss_Monster* pInstance = new CBoss_Monster(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBoss_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBoss_Monster::Free()
{
    __super::Free();
}