#pragma once

#include "Monster.h"

NS_BEGIN(Client)

class CLIENT_DLL CNormal_Monster final : public CMonster
{
private:
    CNormal_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CNormal_Monster(const CNormal_Monster& Prototype);
    virtual ~CNormal_Monster() = default;

public:
    virtual HRESULT                 Initialize_Prototype() override;
    virtual HRESULT                 Initialize(void* pArg) override;

protected:
    virtual const _tchar* Get_DefaultName() const override { return TEXT("Normal_Monster"); }
    virtual SPAWN_TYPE              Get_DefaultSpawnType() const override { return SPAWN_TYPE::MONSTER_NORMAL; }

public:
    static CNormal_Monster* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void                    Free() override;
};

NS_END

