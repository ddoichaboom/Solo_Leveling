#pragma once
#include "Monster.h"

NS_BEGIN(Client)

class CLIENT_DLL CBoss_Monster final : public CMonster
{
private:
    CBoss_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBoss_Monster(const CBoss_Monster& Prototype);
    virtual ~CBoss_Monster() = default;

public:
    virtual HRESULT                 Initialize_Prototype() override;
    virtual HRESULT                 Initialize(void* pArg) override;

protected:
    virtual const _tchar* Get_DefaultName() const override { return TEXT("Boss_Monster"); }
    virtual SPAWN_TYPE              Get_DefaultSpawnType() const override { return SPAWN_TYPE::MONSTER_BOSS; }

public:
    static CBoss_Monster* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void                    Free() override;
};

NS_END