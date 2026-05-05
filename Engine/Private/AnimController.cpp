#include "AnimController.h"
#include "Model.h"
#include "NotifyListener.h"

CAnimController::CAnimController(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CAnimController::CAnimController(const CAnimController& Prototype)
	: CComponent{ Prototype }
	, m_Clips{ Prototype.m_Clips }
	, m_iCurrentKey { Prototype.m_iCurrentKey }
	, m_bHasCurrentClip{ false }
	, m_bFinished { false } 
{
}

HRESULT CAnimController::Bind_Model(CModel* pModel)
{
	if (nullptr == pModel)
		return E_FAIL;

	Safe_Release(m_pModel);
	m_pModel = nullptr;

	m_pModel = pModel;
	Safe_AddRef(m_pModel);

	m_bHasCurrentClip = false;
	m_bFinished = false;

	return S_OK;
}

HRESULT CAnimController::Register_Clip(_uint64 iKey, const ANIM_CLIP_DESC& Desc)
{
	if (nullptr == m_pModel)
		return E_FAIL;

	if (nullptr == Desc.pAnimationName)
		return E_FAIL;

	_int iAnimationIndex = m_pModel->Get_AnimationIndex(Desc.pAnimationName);
	if (-1 == iAnimationIndex)
	{
		return E_FAIL;
	}

	ANIM_CLIP Clip{};
	Clip.iAnimationIndex = static_cast<_uint>(iAnimationIndex);
	Clip.bRestartOnEnter = Desc.bRestartOnEnter;

	m_Clips[iKey] = Clip;

	return S_OK;
}

HRESULT CAnimController::Play(_uint64 iKey, _float fBlendTime)
{
	if (nullptr == m_pModel)
		return E_FAIL;

	auto iter = m_Clips.find(iKey);
	if (iter == m_Clips.end())
		return E_FAIL;

	const ANIM_CLIP& Clip = iter->second;

	// 같은 키 재요청
	if (m_bHasCurrentClip && m_iCurrentKey == iKey)
	{
		if (Clip.bRestartOnEnter)
		{
			m_pModel->Restart_Animation();
			m_bFinished = false;
		}
		return S_OK;
	}

	// 다른 클립으로 요청 ( BlendTime > 0 이면 cross-fade, 아니면 즉시 전환
	if (false == m_bHasCurrentClip || fBlendTime <= 0.f)
		m_pModel->Set_AnimationIndex(Clip.iAnimationIndex);
	else
		m_pModel->Set_AnimationIndex_WithBlend(Clip.iAnimationIndex, fBlendTime, Clip.bRestartOnEnter);

	m_iCurrentKey		= iKey;
	m_bHasCurrentClip	= true;
	m_bFinished			= false;


	return S_OK;
}

_bool CAnimController::Update(_float fTimeDelta)
{
	if (nullptr == m_pModel)
		return false;

	if (false == m_bHasCurrentClip)
		return false;

	m_bFinished = m_pModel->Play_Animation(fTimeDelta, m_pListener);

	return m_bFinished;
}

void CAnimController::Restart()
{
	if (nullptr == m_pModel)
		return;

	if (false == m_bHasCurrentClip)
		return;

	m_pModel->Restart_Animation();
	m_bFinished = false;
}

CAnimController* CAnimController::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CAnimController* pInstance = new CAnimController(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CAnimController");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CAnimController::Clone(void* pArg)
{
	CAnimController* pInstance = new CAnimController(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CAnimController");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CAnimController::Free()
{
	__super::Free();

	Safe_Release(m_pModel);
	m_pModel = nullptr;

	m_Clips.clear();

}
