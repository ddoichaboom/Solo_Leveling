#include "Transform_3D.h"

CTransform_3D::CTransform_3D(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CTransform { pDevice, pContext }
{
}

CTransform_3D::CTransform_3D(const CTransform_3D& Prototype)
	: CTransform { Prototype }
{
}

void CTransform_3D::Go_Straight(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vLook = Get_State(STATE::LOOK);

	vPosition += XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Go_Backward(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vLook = Get_State(STATE::LOOK);

	vPosition -= XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Go_Left(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vRight = Get_State(STATE::RIGHT);

	vPosition -= XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Go_Right(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vRight = Get_State(STATE::RIGHT);

	vPosition += XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Go_Up(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vUp = Get_State(STATE::UP);

	vPosition += XMVector3Normalize(vUp) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Go_Down(_float fTimeDelta)
{
	_vector vPosition = Get_State(STATE::POSITION);
	_vector vUp = Get_State(STATE::UP);

	vPosition -= XMVector3Normalize(vUp) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform_3D::Rotation(_fvector vAxis, _float fRadian)
{
	_float3 vScale = Get_Scale();

	_vector vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f) * vScale.x;
	_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f) * vScale.y;
	_vector vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f) * vScale.z;

	_matrix RotationMatrix = XMMatrixRotationAxis(vAxis, fRadian);

	Set_State(STATE::RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector3TransformNormal(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CTransform_3D::Turn(_fvector vAxis, _float fTimeDelta)
{
	_vector vRight = Get_State(STATE::RIGHT);
	_vector vUp = Get_State(STATE::UP);
	_vector vLook = Get_State(STATE::LOOK);

	_matrix RotationMatrix = XMMatrixRotationAxis(vAxis, m_fRotationPerSec * fTimeDelta);

	Set_State(STATE::RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector3TransformNormal(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CTransform_3D::LookAt(_fvector vAt)
{
	_vector vLook = vAt - Get_State(STATE::POSITION);
	_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
	_vector vUp = XMVector3Cross(vLook, vRight);

	_float3 vScale = Get_Scale();

	Set_State(STATE::RIGHT, XMVector3Normalize(vRight) * vScale.x);
	Set_State(STATE::UP, XMVector3Normalize(vUp) * vScale.y);
	Set_State(STATE::LOOK, XMVector3Normalize(vLook) * vScale.z);
}

void CTransform_3D::Rotate_Toward_XZ(_fvector vDirWorld, _float fMaxRadian)
{
	// 입력 방향의 XZ 성분만 추출하여 정규화 ( 길이 0 이면 무시 )
	_float fDirX = XMVectorGetX(vDirWorld);
	_float fDirZ = XMVectorGetZ(vDirWorld);
	const _float fLenSq = fDirX * fDirX + fDirZ * fDirZ;
	if (fLenSq < 1e-8f)
		return;

	// 목표 Yaw / 현재 Yaw 계산 (atan2(x, z) 좌표계)
	const _float fTargetYaw = atan2f(fDirX, fDirZ);

	_vector vLook = Get_State(STATE::LOOK);
	const _float fCurrentYaw = atan2f(XMVectorGetX(vLook), XMVectorGetZ(vLook));

	// 최단 경로 각도 차
	_float fDiff = fTargetYaw - fCurrentYaw;
	while (fDiff > XM_PI) 
		fDiff -= XM_2PI;
	while (fDiff < -XM_PI) 
		fDiff += XM_2PI;

	// 이번 프레임 최대 회전량으로 클램프 -> 스냅도 자동 처리됨
	if (fMaxRadian > 0.f)
	{
		if (fDiff > fMaxRadian)
			fDiff = fMaxRadian;
		if (fDiff < -fMaxRadian)
			fDiff = -fMaxRadian;
	}

	const _float fNewYaw = fCurrentYaw + fDiff;

	// 스케일 보존하며 RiGHT/UP/LOOK 재구성
	_vector vRight0 = Get_State(STATE::RIGHT);
	_vector vUp0 = Get_State(STATE::UP);
	_vector vLook0 = Get_State(STATE::LOOK);

	const _float fScaleX = XMVectorGetX(XMVector3Length(vRight0));
	const _float fScaleY = XMVectorGetX(XMVector3Length(vUp0));
	const _float fScaleZ = XMVectorGetX(XMVector3Length(vLook0));

	const _float fSin = sinf(fNewYaw);
	const _float fCos = cosf(fNewYaw);

	Set_State(STATE::RIGHT, XMVectorScale(XMVectorSet(fCos, 0.f, -fSin, 0.f), fScaleX));
	Set_State(STATE::UP, XMVectorScale(XMVectorSet(0.f, 1.f, 0.f, 0.f), fScaleY));
	Set_State(STATE::LOOK, XMVectorScale(XMVectorSet(fSin, 0.f, fCos, 0.f), fScaleZ));
}

CTransform_3D* CTransform_3D::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTransform_3D* pInstance = new CTransform_3D(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTransform_3D");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CTransform_3D::Clone(void* pArg)
{
	CTransform_3D* pInstance = new CTransform_3D(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTransform_3D");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTransform_3D::Free()
{
	__super::Free();
}
