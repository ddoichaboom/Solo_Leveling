#include "Transform.h"
#include "Shader.h"

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{

}

CTransform::CTransform(const CTransform& Prototype)
	: CComponent{ Prototype }
	, m_WorldMatrix { Prototype.m_WorldMatrix }
{

}

HRESULT CTransform::Initialize_Prototype()
{
	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());		// 항등 행렬로 초기화

	return S_OK;
}

HRESULT CTransform::Initialize(void* pArg)
{
	// pArg에 할당된 값이 없다면 기본값으로 Transform을 생성
	if (nullptr == pArg)
		return S_OK;

	// pArg가 nullptr이 아니라면 담긴 값으로 설정
	auto pDesc = static_cast<TRANSFORM_DESC*>(pArg);

	m_fRotationPerSec	= pDesc->fRotationPerSec;
	m_fSpeedPerSec		= pDesc->fSpeedPerSec;

	return S_OK;
}

HRESULT CTransform::Bind_ShaderResource(CShader* pShader, const _char* pConstantName)
{
	// 자신의 월드 행렬을 셰이더의 지정된 상수에 바인딩
	return pShader->Bind_Matrix(pConstantName, &m_WorldMatrix);
}

void CTransform::Set_Position(_float3 vPos)
{
	Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&vPos), 1.f));
}

_float3 CTransform::Get_Rotation()
{
	_float3 vScale = Get_Scale();

	// 스케일 제거 → 순수 회전 행렬 추출
	float r00 = m_WorldMatrix._11 / vScale.x;
	float r01 = m_WorldMatrix._12 / vScale.x;
	float r02 = m_WorldMatrix._13 / vScale.x;
	float r10 = m_WorldMatrix._21 / vScale.y;
	float r11 = m_WorldMatrix._22 / vScale.y;
	float r12 = m_WorldMatrix._23 / vScale.y;
	float r20 = m_WorldMatrix._31 / vScale.z;
	float r21 = m_WorldMatrix._32 / vScale.z;
	float r22 = m_WorldMatrix._33 / vScale.z;

	float pitch = asinf(-r21);
	float yaw, roll;

	if (fabsf(cosf(pitch)) > 0.001f)
	{
		yaw = atan2f(r20, r22);
		roll = atan2f(r01, r11);
	}
	else
	{
		yaw = atan2f(-r02, r00);
		roll = 0.f;
	}

	return _float3{
		XMConvertToDegrees(pitch),
		XMConvertToDegrees(yaw),
		XMConvertToDegrees(roll)
	};
}

void CTransform::Set_Rotation(_float3 vDegrees)
{
	_float3 vScale = Get_Scale();

	XMMATRIX matRot = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(vDegrees.x),
		XMConvertToRadians(vDegrees.y),
		XMConvertToRadians(vDegrees.z));

	Set_State(STATE::RIGHT, XMVector3Normalize(matRot.r[0]) * vScale.x);
	Set_State(STATE::UP, XMVector3Normalize(matRot.r[1]) * vScale.y);
	Set_State(STATE::LOOK, XMVector3Normalize(matRot.r[2]) * vScale.z);
}

void CTransform::Set_Scale(_float3 vScale)
{
	Set_State(STATE::RIGHT, XMVector3Normalize(Get_State(STATE::RIGHT)) * vScale.x);
	Set_State(STATE::UP, XMVector3Normalize(Get_State(STATE::UP)) * vScale.y);
	Set_State(STATE::LOOK, XMVector3Normalize(Get_State(STATE::LOOK)) * vScale.z);
}

void CTransform::Set_Scale(_float fScaleX, _float fScaleY, _float fScaleZ)
{
	Set_State(STATE::RIGHT, XMVector3Normalize(Get_State(STATE::RIGHT)) * fScaleX);
	Set_State(STATE::UP, XMVector3Normalize(Get_State(STATE::UP)) * fScaleY);
	Set_State(STATE::LOOK, XMVector3Normalize(Get_State(STATE::LOOK)) * fScaleZ);
}

void CTransform::Scaling(_float fScaleX, _float fScaleY, _float fScaleZ)
{
	Set_State(STATE::RIGHT, Get_State(STATE::RIGHT) * fScaleX);
	Set_State(STATE::UP, Get_State(STATE::UP) * fScaleY);
	Set_State(STATE::LOOK, Get_State(STATE::LOOK) * fScaleZ);
}

void CTransform::Free()
{
	__super::Free();
}
