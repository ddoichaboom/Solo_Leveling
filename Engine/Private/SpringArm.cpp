#include "SpringArm.h"

CSpringArm::CSpringArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CSpringArm::CSpringArm(const CSpringArm& Prototype)
	: CComponent { Prototype }
{
}

_vector CSpringArm::Get_TargetPoint() const
{
	_vector vPos = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	if (nullptr != m_pTargetWorldMatrix)
	{
		_matrix mTarget = XMLoadFloat4x4(m_pTargetWorldMatrix);
		vPos = mTarget.r[3];
	}

	return XMVectorAdd(vPos, XMLoadFloat3(&m_vHeightOffset));
}

_vector CSpringArm::Get_LookDirection() const
{
	_vector q = XMLoadFloat4(&m_qOrientation);

	return XMVector3Normalize(XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), q));
}

_vector CSpringArm::Get_EyePosition() const
{
	_vector vTarget = Get_TargetPoint();
	_vector vLook = Get_LookDirection();

	return XMVectorSubtract(vTarget, XMVectorScale(vLook, m_fCurrentDistance));
}

_float CSpringArm::Get_Yaw() const
{
	_vector q = XMLoadFloat4(&m_qOrientation);
	_vector vF = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), q);

	// XZ 평면 투영 각도 
	return atan2f(XMVectorGetX(vF), XMVectorGetZ(vF));
}

HRESULT CSpringArm::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CSpringArm::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	auto pDesc = static_cast<SPRING_ARM_DESC*>(pArg);

	// Config
	m_pTargetWorldMatrix	= pDesc->pTargetWorldMatrix;
	m_vHeightOffset			= pDesc->vHeightOffset;
	m_fIdealDistance		= pDesc->fIdealDistance;
	m_fArmLerpSpeed			= pDesc->fArmLerpSpeed;
	m_fPitchMin				= pDesc->fPitchMin;
	m_fPitchMax				= pDesc->fPitchMax;
	m_fMouseSensor			= pDesc->fMouseSensor;

	// Runtime state 초기화
	_vector qInit = XMQuaternionRotationRollPitchYaw(
		pDesc->fInitialPitch, pDesc->fInitialPitch, 0.f);
	XMStoreFloat4(&m_qOrientation, qInit);

	m_fCurrentDistance = m_fIdealDistance;		// 시작 시 충돌 없다고 가정

	return S_OK;
}

void CSpringArm::Update_Rotation(_long lMouseDX, _long lMouseDY)
{
	const _vector vWorldUp			= XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector vDefaultForward	= XMVectorSet(0.f, 0.f, 1.f, 0.f);

	const _float fYawDelta = lMouseDX * m_fMouseSensor;
	const _float fPitchDelta = lMouseDY * m_fMouseSensor;		// 아래로 움직일 때 내려다보게

	_vector q = XMLoadFloat4(&m_qOrientation);

	// 1) Yaw 적용 - World Up 축, 항상 허용
	if (0.f != fYawDelta)
	{
		_vector qYaw = XMQuaternionRotationAxis(vWorldUp, fYawDelta);
		q = XMQuaternionMultiply(qYaw, q);
	}

	// 2) Pitch  
	if (0.f != fPitchDelta)
	{
		_vector vLocalRight = XMVector3Rotate(XMVectorSet(1.f, 0.f, 0.f, 0.f), q);
		_vector qPitch = XMQuaternionRotationAxis(vLocalRight, fPitchDelta);

		_vector qTentative = XMQuaternionMultiply(q, qPitch);

		// Pitch 추출
		_vector vForward = XMVector3Rotate(vDefaultForward, qTentative);
		_float fPitchNow = asinf(XMVectorGetY(vForward));

		if (fPitchNow >= m_fPitchMin && fPitchNow <= m_fPitchMax)
			q = qTentative;

		// 범위 밖이면 Pitch 델타 버림
	}

	// 3) Drift 방지 
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&m_qOrientation, q);
}

void CSpringArm::Update_Arm(_float fTimeDelta)
{
	// E-1a : 충돌 없으므로 즉시 Ideal 유지
	m_fCurrentDistance = m_fIdealDistance;

	// E-1b 에서 아래 패턴으로 교체:
	//   _float fReducedDist = Raycast(Target → IdealEye)  // 충돌 시 단축값
	//   if (fReducedDist < m_fCurrentDistance)
	//       m_fCurrentDistance = fReducedDist;            // 즉시 단축
	//   else
	//       // Ideal 쪽으로 보간 복귀
	//       m_fCurrentDistance += (m_fIdealDistance - m_fCurrentDistance) * m_fArmLerpSpeed * fTimeDelta;
}

CSpringArm* CSpringArm::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CSpringArm* pInstance = new CSpringArm(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CSpringArm");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CSpringArm::Clone(void* pArg)
{
	CSpringArm* pInstance = new CSpringArm(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSpringArm");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CSpringArm::Free()
{
	__super::Free();
}
