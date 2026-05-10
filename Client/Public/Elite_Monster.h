#pragma once
#include "Monster.h"

NS_BEGIN(Client)

class CLIENT_DLL CElite_Monster final : public CMonster
{
private:
    CElite_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CElite_Monster(const CElite_Monster& Prototype);
    virtual ~CElite_Monster() = default;

public:
    virtual HRESULT                 Initialize_Prototype() override;
    virtual HRESULT                 Initialize(void* pArg) override;

protected:
    virtual const _tchar* Get_DefaultName() const override { return TEXT("Elite_Monster"); }
    virtual SPAWN_TYPE              Get_DefaultSpawnType() const override { return SPAWN_TYPE::MONSTER_ELITE; }

public:
    static CElite_Monster* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void                    Free() override;
};

NS_END
