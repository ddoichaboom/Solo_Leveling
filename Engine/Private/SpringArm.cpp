#include "SpringArm.h"

CSpringArm::CSpringArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CSpringArm::CSpringArm(const CSpringArm& Prototype)
	: CComponent { Prototype }
{
}

void CSpringArm::Set_TargetWorldMatrix(const _float4x4* pTargetMat)
{
	m_pTargetWorldMatrix = pTargetMat;
}

void CSpringArm::Set_DesiredDistance(_float fDistance)
{
	m_fDesiredDistance = max(0.f, min(fDistance, m_fIdealDistance));
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

	m_fYaw					= pDesc->fInitialYaw;
	m_fPitch				= pDesc->fInitialPitch;

	{
		_vector qYaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), m_fYaw);
		_vector qPitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), m_fPitch);
		_vector q = XMQuaternionMultiply(qPitch, qYaw);
		XMStoreFloat4(&m_qOrientation, q);
	}

	m_fCurrentDistance		= m_fIdealDistance;
	m_fDesiredDistance		= m_fIdealDistance;

	return S_OK;
}

void CSpringArm::Update_Rotation(_long lMouseDX, _long lMouseDY)
{
	// 1) 누적은 스칼라로만
	// 쿼터니언 누적 오차 없음
	m_fYaw += lMouseDX * m_fMouseSensor;
	m_fPitch += lMouseDY * m_fMouseSensor;

	if (m_fYaw > XM_PI) 
		m_fYaw -= XM_2PI;
	if (m_fYaw < -XM_PI) 
		m_fYaw += XM_2PI;

	// 2) Pitch 클램프는 스칼라 단계에서 한 줄로 
	// asinf 역산 불필요
	if (m_fPitch < m_fPitchMin)
		m_fPitch = m_fPitchMin;
	if (m_fPitch > m_fPitchMax)
		m_fPitch = m_fPitchMax;

	// 3) 매 프레임 처음부터 재구성 - 이전 q를 읽지 않는다.
	_vector qYaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), m_fYaw);
	_vector qPitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), m_fPitch);
	_vector q = XMQuaternionMultiply(qPitch, qYaw);

	// 4) 캐시에 저장 
	XMStoreFloat4(&m_qOrientation, XMQuaternionNormalize(q));
}

void CSpringArm::Update_Arm(_float fTimeDelta)
{
	const _float fTargetDistance = m_fDesiredDistance;

	if (fTargetDistance < m_fCurrentDistance)
	{
		// Collision shrink: apply immediately so the camera does not pass through walls.
		m_fCurrentDistance = fTargetDistance;
	}
	else
	{
		// Recovery only: when collision is gone, return to ideal distance smoothly.
		_float fAlpha = 1.f;

		if (m_fArmLerpSpeed > 0.f)
			fAlpha = 1.f - expf(-m_fArmLerpSpeed * fTimeDelta);

		fAlpha = max(0.f, min(1.f, fAlpha));

		m_fCurrentDistance += (fTargetDistance - m_fCurrentDistance) * fAlpha;
	}

	// Until camera collision exists, next frame defaults back to ideal distance.
	m_fDesiredDistance = m_fIdealDistance;
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
