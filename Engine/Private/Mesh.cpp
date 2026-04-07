#include "Mesh.h"
#include "Bone.h"
#include "Shader.h"

CMesh::CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{
}

CMesh::CMesh(const CMesh& Prototype)
	: CVIBuffer{ Prototype }
{
}

HRESULT CMesh::Initialize_Prototype(MODEL eType, const MESH_DESC& Desc)
{
	strcpy_s(m_szName, Desc.szName);
	m_iMaterialIndex = Desc.iMaterialIndex;

	m_iNumVertices = Desc.iNumVertices;
	m_iNumIndices = Desc.iNumIndices;
	m_iVertexStride = Desc.iVertexStride;

	m_iIndexStride = sizeof(_uint);
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_iNumVertexBuffers = 1;
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	if (MODEL::NONANIM == eType)
	{
		if (FAILED(Ready_NonAnimMesh(Desc)))
			return E_FAIL;
	}
	else
	{
		if (FAILED(Ready_AnimMesh(Desc)))
			return E_FAIL;
	}

	// 인덱스 버퍼(공용)
	D3D11_BUFFER_DESC IBDesc = {};
	IBDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IBInitialData = {};
	IBInitialData.pSysMem = Desc.pIndices;

	if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &IBInitialData, &m_pIB)))
		return E_FAIL;

	// PICK_DATA - 인덱스
	m_pPickData->iNumIndices = m_iNumIndices;
	m_pPickData->pIndices = new _uint[m_iNumIndices];
	memcpy(m_pPickData->pIndices, Desc.pIndices, sizeof(_uint) * m_iNumIndices);

	return S_OK;
}

HRESULT CMesh::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CMesh::Bind_BoneMatrices(CShader* pShader, const _char* pConstantName, const vector<class CBone*>& Bones)
{
	for (_uint i = 0; i < m_iNumBones; ++i)
	{
		_matrix OffsetMatrix = XMLoadFloat4x4(&m_OffsetMatrices[i]);
		_matrix CombinedMatrix = XMLoadFloat4x4(Bones[m_BoneIndices[i]]->Get_CombinedTransformMatrixPtr());

		XMStoreFloat4x4(&m_BoneMatrices[i], OffsetMatrix * CombinedMatrix);
	}

	return pShader->Bind_RawValue(pConstantName, m_BoneMatrices, sizeof(_float4x4) * m_iNumBones);
}

HRESULT CMesh::Ready_NonAnimMesh(const MESH_DESC& Desc)
{
	// 정점 버퍼 - VTXMESH
	D3D11_BUFFER_DESC VBDesc = {};
	VBDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VBInitialData = {};
	VBInitialData.pSysMem = Desc.pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	// PICK_DATA
	m_pPickData = new PICK_DATA;
	m_pPickData->iNumVertices = m_iNumVertices;
	m_pPickData->pVerticesPos = new _float3[m_iNumVertices];

	VTXMESH* pVtx = static_cast<VTXMESH*>(Desc.pVertices);
	for (_uint i = 0; i < m_iNumVertices; ++i)
		m_pPickData->pVerticesPos[i] = pVtx[i].vPosition;

	return S_OK;
}

HRESULT CMesh::Ready_AnimMesh(const MESH_DESC& Desc)
{
	// 정점 버퍼 - VTXANIMMESH
	D3D11_BUFFER_DESC VBDesc = {};
	VBDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VBInitialData = {};
	VBInitialData.pSysMem = Desc.pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	// 본 데이터 저장
	m_iNumBones = Desc.iNumBones;

	m_BoneIndices.resize(m_iNumBones);
	memcpy(m_BoneIndices.data(), Desc.pBoneIndices, sizeof(_uint) * m_iNumBones);

	m_OffsetMatrices.resize(m_iNumBones);
	memcpy(m_OffsetMatrices.data(), Desc.pOffsetMatrices, sizeof(_float4x4) * m_iNumBones);

	// PICK_DATA
	m_pPickData = new PICK_DATA;
	m_pPickData->iNumVertices = m_iNumVertices;
	m_pPickData->pVerticesPos = new _float3[m_iNumVertices];

	VTXANIMMESH* pVtx = static_cast<VTXANIMMESH*>(Desc.pVertices);
	for (_uint i = 0; i < m_iNumVertices; ++i)
		m_pPickData->pVerticesPos[i] = pVtx[i].vPosition;

	return S_OK;
}

CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODEL eType, const MESH_DESC& Desc)
{
	CMesh* pInstance = new CMesh(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(eType, Desc)))
	{
		MSG_BOX("Failed to Created : CMesh");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CMesh::Clone(void* pArg)
{
	return new CMesh(*this);
}

void CMesh::Free()
{
	__super::Free();
}
