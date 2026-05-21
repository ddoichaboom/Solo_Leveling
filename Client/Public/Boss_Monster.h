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
    virtual void                    Update(_float fTimeDelta) override;

    void                            Begin_Encounter();
    virtual void                    Handle_ActionTransition(MONSTER_ACTION eFromAction, MONSTER_ACTION_STEP eFromStep,
                                                            MONSTER_ACTION eToAction, MONSTER_ACTION_STEP eToStep,
                                                            _bool bInitial) override;

protected:
    virtual const _tchar*           Get_DefaultName() const override { return TEXT("Boss_Monster"); }
    virtual SPAWN_TYPE              Get_DefaultSpawnType() const override { return SPAWN_TYPE::MONSTER_BOSS; }

    virtual MONSTER_ANIM_SET        Get_DefaultAnimSet() const override
    {
        return MONSTER_ANIM_SET::IGRIS_BOSS;
    }
    virtual const _tchar*           Get_DefaultBodyModelPrototypeTag() const override
    {
        return TEXT("Prototype_Component_Model_Monster_Igris_Body");
    }
    virtual const _tchar*           Get_DefaultWeaponModelPrototypeTag() const override
    {
        return TEXT("Prototype_Component_Model_Monster_Igris_Weapon");
    }
    virtual const _char*            Get_DefaultWeaponSocketBoneName() const override
    {
        return "Bip001 Prop1";
    }

protected:
    virtual MONSTER_ACTION          Select_AIAction(CGameObject* pTarget, _float fDistance) override;
    virtual MONSTER_ACTION_STEP     Select_AIActionStep(MONSTER_ACTION eAction) const override;
    virtual void                    Apply_RootMotion(const _float3& vLocalDelta) override;

    virtual void                    On_AttackHitboxNotify(_bool bActive) override;

private:
    MONSTER_ACTION                  Select_PostCrashPattern(CGameObject* pTarget, _float fDistance);

    void                            Apply_RadiusDamage(_float fRadius, _float fDamage);

    void                            Begin_Skill01Dash(CGameObject* pTarget);
    void                            End_Skill01Dash();

    void                            Tick_PatternCooldowns(_float fTimeDelta);
    void                            Start_PatternCooldown(MONSTER_ACTION eAction);
    _bool                           Is_PatternReady(MONSTER_ACTION eAction) const;
    _float                          Get_PatternCooldown(MONSTER_ACTION eAction) const;

    _float3                         Get_DirectionToTargetXZ(CGameObject* pTarget) const;
    void                            Face_TargetImmediately(CGameObject* pTarget);
    void                            Face_TargetTracking(CGameObject* pTarget, _float fTimeDelta);

private:
    _bool                           m_bEncounterStarted = { false };
    _bool                           m_bOpeningSkillUsed = { false };
    _bool                           m_bPostCrashPatternPending = { false };
    _bool                           m_bSkill01DashActive = { false };

    _float                          m_fSkill10LoopElapsed = { 0.f };
    _float                          m_fSkill10LoopDuration = { 2.1f };

    _float3                         m_vSkill01DashTargetPosition = {};
    _float                          m_fSkill01RootMotionScale = { 1.f };
    _float                          m_fSkill01StopDistance = { 2.8f };
    _float                          m_fSkill01BaseTravelDistance = { 10.5f };

    _float                          m_fPatternCooldowns[static_cast<_uint>(MONSTER_ACTION::END)] = {};

public:
    static CBoss_Monster*           Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*            Clone(void* pArg) override;
    virtual void                    Free() override;
};

NS_END
