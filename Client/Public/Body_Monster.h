#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CAnimController;
class INotifyListener;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CBody_Monster final : public CPartObject
{
public:
	typedef struct tagBodyMonsterDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _tchar*		pModelPrototypeTag = { nullptr };
		MONSTER_ANIM_SET	eAnimSet = { MONSTER_ANIM_SET::NONE };
	}BODY_MONSTER_DESC;

private:
	CBody_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBody_Monster(const CBody_Monster& Prototype);
	virtual ~CBody_Monster() = default;

public:
	virtual HRESULT					Initialize_Prototype() override;
	virtual HRESULT					Initialize(void* pArg) override;
	virtual void					Priority_Update(_float fTimeDelta) override;
	virtual void					Update(_float fTimeDelta) override;
	virtual void					Late_Update(_float fTimeDelta) override;
	virtual HRESULT					Render() override;

public:
	const _float4x4*				Get_BoneMatrixPtr(const _char* pBoneName) const;

	HRESULT							Play_Action(MONSTER_ACTION eAction,	MONSTER_ACTION_STEP eStep = MONSTER_ACTION_STEP::NONE, MONSTER_PHASE ePhase = MONSTER_PHASE::COMMON);

	void							Set_Listener(INotifyListener* pListener);
	_float3							Get_LastRootMotionDelta() const;

private:
	HRESULT							Ready_Components(const BODY_MONSTER_DESC& Desc);
	HRESULT							Bind_ShaderResources();

	HRESULT                         Ready_AnimationTable();
	HRESULT                         Register_AnimationClips();

	const MONSTER_ACTION_POLICY*	Find_ActionPolicy(MONSTER_ACTION eAction, MONSTER_ACTION_STEP eStep) const;
private:
	CShader*						m_pShaderCom = { nullptr };
	CModel*							m_pModelCom = { nullptr };
	CAnimController*				m_pAnimController = { nullptr };

	_wstring						m_strModelPrototypeTag;
	MONSTER_ANIM_SET				m_eAnimSet = { MONSTER_ANIM_SET::NONE };

	MONSTER_PHASE					m_ePhase = { MONSTER_PHASE::COMMON };
	MONSTER_ACTION					m_eCurrentAction = { MONSTER_ACTION::IDLE };
	MONSTER_ACTION_STEP				m_eCurrentStep = { MONSTER_ACTION_STEP::NONE };

	const MONSTER_ANIM_TABLE_DESC*	m_pAnimTable = { nullptr };
	INotifyListener*				m_pListener = { nullptr };

public:
	static CBody_Monster*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*			Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END

