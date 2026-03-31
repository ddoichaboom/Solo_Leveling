#include "Shader.h"

CShader::CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{

}

CShader::CShader(const CShader& Prototype)
	: CComponent{ Prototype }
	, m_pEffect{ Prototype.m_pEffect }
	, m_iNumPasses{ Prototype.m_iNumPasses }
	, m_vInputLayouts{ Prototype.m_vInputLayouts }
{
	for (auto& pInputLayout : m_vInputLayouts)
		Safe_AddRef(pInputLayout);

	Safe_AddRef(m_pEffect);
}

HRESULT CShader::Initialize_Prototype(const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements)
{
	// (1) HLSL 컴파일 플래그 설정
	_uint	iHlslFlag = {};

#ifdef _DEBUG
	iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	iHlslFlag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif

	// (2) HLSL 파일을 컴파일하여 Effect 객체 생성
	if (FAILED(D3DX11CompileEffectFromFile(
		pShaderFilePath,							// HLSL 파일 경로
		nullptr,									// #define 매크로 (없음)
		D3D_COMPILE_STANDARD_FILE_INCLUDE,			// #include 핸들러 (HLSL 파일에서도 Include 하여 컴파일 할 수 있게 함)
		iHlslFlag,									// 컴파일 플래그
		0,											// Effect 플래그
		m_pDevice,									// D3D11 Device
		&m_pEffect,									// [Out] Effect 객체
		nullptr)))									// 에러 블롭 (메모리 덩어리) - 기존에는 특정 값들을 반환? 오류 메세지를 출력하기 위해 사용되었지만 현재 출력창에 전부 출력됨

		return E_FAIL;

	// (3) 첫 번째 Technique 가져오기
	ID3DX11EffectTechnique* pTechnique = m_pEffect->GetTechniqueByIndex(0);
	if (nullptr == pTechnique)
		return E_FAIL;

	// (4) Technique의 Pass 개수 확인
	D3DX11_TECHNIQUE_DESC TechniqueDesc{};
	pTechnique->GetDesc(&TechniqueDesc);
	m_iNumPasses = TechniqueDesc.Passes;


	// (5)  각 Pass에 대해 InputLayout 생성
	for (size_t i = 0; i < m_iNumPasses; i++)
	{
		ID3DX11EffectPass* pPass = pTechnique->GetPassByIndex(i);

		D3DX11_PASS_DESC PassDesc{};
		pPass->GetDesc(&PassDesc);

		ID3D11InputLayout* pInputLayout = { nullptr };

		// PassDesc에서 셰이더 바이트 코드 시그니처를 가져와 InputLayout 생성
		if (FAILED(m_pDevice->CreateInputLayout(
			pElements,										// 입력 요소 배열
			iNumElements,									// 요소 개수
			PassDesc.pIAInputSignature,						// 셰이더 입력 시그니처
			PassDesc.IAInputSignatureSize,					// 시그니처 크기 
			&pInputLayout)))								// [OUT]
			return E_FAIL;

		m_vInputLayouts.push_back(pInputLayout);
	}

	return S_OK;
}


HRESULT CShader::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CShader::Begin(_uint iPassIndex)
{
	if (iPassIndex >= m_iNumPasses)
		return E_FAIL;

	// (1) 해당 패스의 InputLayout을 IA 스테이지에 설정
	m_pContext->IASetInputLayout(m_vInputLayouts[iPassIndex]);

	// (2) 해당 패스의 모든 상태(VS, PS 등)를 파이프라인에 적용
	m_pEffect->GetTechniqueByIndex(0)->GetPassByIndex(iPassIndex)->Apply(0, m_pContext);	// Apply가 핵심. 해당 Pass에 설정된 VertexShader, PixelShader, 그리고 기타 렌더 스테이트를 DeviceContext에 바인딩한다.

	return S_OK;
}

HRESULT CShader::Bind_Matrix(const _char* pConstantName, const _float4x4* pMatrix)
{
	// (1) Effect에서 이름으로 변수 검색
	ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(pConstantName);
	if (!pVariable->IsValid())
		return E_FAIL;

	// (2) 행렬 타입으로 캐스팅
	ID3DX11EffectMatrixVariable* pMatrixVariable = pVariable->AsMatrix();
	if (!pMatrixVariable->IsValid())
		return E_FAIL;

	// (3) HLSL 전역 변수에 행렬 데이터 설정
	return pMatrixVariable->SetMatrix(reinterpret_cast<const _float*>(pMatrix));
}

HRESULT CShader::Bind_SRV(const _char* pConstantName, ID3D11ShaderResourceView* pSRV)
{
	// (1) Effect에서 이름으로 변수 검색
	ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(pConstantName);
	if (!pVariable->IsValid())
		return E_FAIL;

	// (2) ShaderResource타입으로 캐스팅
	ID3DX11EffectShaderResourceVariable* pSRVariable = pVariable->AsShaderResource();
	if (!pSRVariable->IsValid())
		return E_FAIL;

	// (3) HLSL 전역 변수에 SRV 바인딩
	return pSRVariable->SetResource(pSRV);
}

CShader* CShader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements)
{
	CShader* pInstance = new CShader(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pShaderFilePath, pElements, iNumElements)))
	{
		MSG_BOX("Failed to Created : CShader");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CShader::Clone(void* pArg)
{
	CShader* pInstance = new CShader(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CShader");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CShader::Free()
{
	__super::Free();

	for (auto& pInputLayout : m_vInputLayouts)
		Safe_Release(pInputLayout);

	m_vInputLayouts.clear();

	Safe_Release(m_pEffect);
}