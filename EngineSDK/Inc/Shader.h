#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CShader final : public CComponent
{
private:
	CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CShader(const CShader& Prototype);
	virtual ~CShader() = default;

public:
	virtual HRESULT				Initialize_Prototype(const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements);
	virtual HRESULT				Initialize(void* pArg);

public:
	HRESULT						Begin(_uint iPassIndex);

public:
	HRESULT						Bind_Matrix(const _char* pConstantName, const _float4x4* pMatrix);
	HRESULT						Bind_Matrices(const _char* pConstantName, const _float4x4* pMatrices, _uint iNumMatrices);
	HRESULT						Bind_SRV(const _char* pConstantName, ID3D11ShaderResourceView* pSRV);
	HRESULT						Bind_RawValue(const _char* pConstantName, const void* pValue, _uint iLength);

private:
	_uint						m_iNumPasses = {};
	ID3DX11Effect*				m_pEffect = { nullptr };	// FX11 Effect 객체를 소유. 셰이더 컴파일 결과가 여기 저장됨
	vector<ID3D11InputLayout*>	m_vInputLayouts;			// pass 별 InputLayout


public:
	static CShader*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements);
	virtual CComponent*			Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END