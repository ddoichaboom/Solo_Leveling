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
