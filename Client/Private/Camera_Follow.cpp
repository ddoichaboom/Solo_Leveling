#include "Camera_Follow.h"
#include "SpringArm.h"
#include "GameInstance.h"
#include "Transform_3D.h"

CCamera_Follow::CCamera_Follow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CCamera { pDevice, pContext }
{
}

CCamera_Follow::CCamera_Follow(const CCamera_Follow& Prototype)
	: CCamera { Prototype }
{
}

HRESULT CCamera_Follow::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCamera_Follow::Initialize(void* pArg)
{
	m_strName = TEXT("Camera");
	m_strTag = TEXT("Follow");

	if (nullptr == pArg)
		return E_FAIL;

	auto pDesc = static_cast<CAMERA_FOLLOW_DESC*>(pArg);

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(*pDesc)))
		return E_FAIL;

	return S_OK;
}

void CCamera_Follow::Priority_Update(_float fTimeDelta)
{

}

void CCamera_Follow::Update(_float fTimeDelta)
{
}

void CCamera_Follow::Late_Update(_float fTimeDelta)
{
	// (1) 입력 수집 & SpringArm에 전달
	_long lDX = m_pGameInstance->Get_MouseDelta(MOUSEAXIS::X);
	_long lDY = m_pGameInstance->Get_MouseDelta(MOUSEAXIS::Y);

	m_pSpringArm->Update_Rotation(lDX, lDY);
	m_pSpringArm->Update_Arm(fTimeDelta);

	// 2) SpringArm 결과를 Transform State 로 적용
	Apply_SpringArmToTransform();

	//static _float fLastYaw = 999.f;
	//if (fabsf(m_pSpringArm->Get_Yaw() - fLastYaw) > 0.01f)
	//{
	//	_vector vEye = m_pSpringArm->Get_EyePosition();
	//	_vector vTarget = m_pSpringArm->Get_TargetPoint();
	//	_vector vLook = m_pSpringArm->Get_LookDirection();

	//	wchar_t szBuf[512];
	//	swprintf_s(szBuf, L"[Camera] Tgt=(%.2f, %.2f, %.2f)  Eye=(%.2f, %.2f, %.2f)  Look=(%.3f, %.3f, %.3f)\n",
	//		XMVectorGetX(vTarget), XMVectorGetY(vTarget), XMVectorGetZ(vTarget),
	//		XMVectorGetX(vEye), XMVectorGetY(vEye), XMVectorGetZ(vEye),
	//		XMVectorGetX(vLook), XMVectorGetY(vLook), XMVectorGetZ(vLook));
	//	OutputDebugStringW(szBuf);
	//	fLastYaw = m_pSpringArm->Get_Yaw();
	//}

	__super::Update_PipeLine();
}

HRESULT CCamera_Follow::Render()
{
	return S_OK;
}

_float	CCamera_Follow::Get_Yaw() const
{
	if (nullptr == m_pSpringArm)
		return 0.f;

	return m_pSpringArm->Get_Yaw();
}

HRESULT CCamera_Follow::Ready_Components(const CAMERA_FOLLOW_DESC& Desc)
{
	CSpringArm::SPRING_ARM_DESC ArmDesc{};
	ArmDesc.pTargetWorldMatrix = Desc.pTargetWorldMatrix;
	ArmDesc.vHeightOffset = Desc.vHeightOffset;
	ArmDesc.fIdealDistance = Desc.fIdealDistance;
	ArmDesc.fArmLerpSpeed = Desc.fArmLerpSpeed;
	ArmDesc.fInitialYaw = Desc.fInitialYaw;
	ArmDesc.fInitialPitch = Desc.fInitialPitch;
	ArmDesc.fPitchMin = Desc.fPitchMin;
	ArmDesc.fPitchMax = Desc.fPitchMax;
	ArmDesc.fMouseSensor = Desc.fMouseSensor;

	if (FAILED(__super::Add_Component(
				ETOUI(LEVEL::STATIC),
				TEXT("Prototype_Component_SpringArm"),
				TEXT("Com_SpringArm"),
				reinterpret_cast<CComponent**>(&m_pSpringArm),
				&ArmDesc)))
		return E_FAIL;

	return S_OK;
}

void CCamera_Follow::Apply_SpringArmToTransform()
{
	_vector vEye		= m_pSpringArm->Get_EyePosition();
	_vector vTarget		= m_pSpringArm->Get_TargetPoint();
	_vector vLook		= m_pSpringArm->Get_LookDirection();

	// World Up 기준 
	_vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vLook));
	_vector vUp = XMVector3Cross(vLook, vRight);

	CTransform_3D* pTransform = static_cast<CTransform_3D*>(m_pTransformCom);
	pTransform->Set_State(STATE::RIGHT, vRight);
	pTransform->Set_State(STATE::UP, vUp);
	pTransform->Set_State(STATE::LOOK, vLook);
	pTransform->Set_State(STATE::POSITION, vEye);
}

CCamera_Follow* CCamera_Follow::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCamera_Follow* pInstance = new CCamera_Follow(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CCamera_Follow");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCamera_Follow::Clone(void* pArg)
{
	CCamera_Follow* pInstance = new CCamera_Follow(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CCamera_Follow");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCamera_Follow::Free()
{
	__super::Free();

	Safe_Release(m_pSpringArm);
}
