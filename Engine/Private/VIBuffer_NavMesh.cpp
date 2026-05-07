#include "VIBuffer_NavMesh.h"

CVIBuffer_NavMesh::CVIBuffer_NavMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CVIBuffer{ pDevice, pContext }
{
}

CVIBuffer_NavMesh::CVIBuffer_NavMesh(const CVIBuffer_NavMesh& Prototype)
	: CVIBuffer{ Prototype }
{
}

HRESULT CVIBuffer_NavMesh::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CVIBuffer_NavMesh::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CVIBuffer_NavMesh::Build(const vector<_float3>& Vertices, const vector<NAVMESH_CELL>& Cells, _float fYOffset)
{
    Release_Buffers();

    vector<VTXPOS> LineVertices;
    vector<_uint> LineIndices;

    LineVertices.reserve(Cells.size() * 3);
    LineIndices.reserve(Cells.size() * 6);

    for (const auto& Cell : Cells)
    {
        _bool bValid = true;

        for (_uint i = 0; i < ETOUI(NAVMESH_POINT::END); ++i)
        {
            const _int iVertexIndex = Cell.iVertexIndices[i];

            if (iVertexIndex < 0 || static_cast<size_t>(iVertexIndex) >= Vertices.size())
            {
                bValid = false;
                break;
            }
        }

        if (false == bValid)
            continue;

        const _uint iBaseIndex = static_cast<_uint>(LineVertices.size());

        for (_uint i = 0; i < ETOUI(NAVMESH_POINT::END); ++i)
        {
            VTXPOS Vertex{};
            Vertex.vPosition = Vertices[Cell.iVertexIndices[i]];
            Vertex.vPosition.y += fYOffset;

            LineVertices.push_back(Vertex);
        }

        LineIndices.push_back(iBaseIndex + 0);
        LineIndices.push_back(iBaseIndex + 1);
        LineIndices.push_back(iBaseIndex + 1);
        LineIndices.push_back(iBaseIndex + 2);
        LineIndices.push_back(iBaseIndex + 2);
        LineIndices.push_back(iBaseIndex + 0);
    }

    if (LineVertices.empty() || LineIndices.empty())
        return S_OK;

    m_iNumVertexBuffers = 1;
    m_iNumVertices = static_cast<_uint>(LineVertices.size());
    m_iVertexStride = sizeof(VTXPOS);

    m_iNumIndices = static_cast<_uint>(LineIndices.size());
    m_iIndexStride = sizeof(_uint);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

    D3D11_BUFFER_DESC VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;

    D3D11_SUBRESOURCE_DATA VertexInitialData{};
    VertexInitialData.pSysMem = LineVertices.data();

    if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
        return E_FAIL;

    D3D11_BUFFER_DESC IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;

    D3D11_SUBRESOURCE_DATA IndexInitialData{};
    IndexInitialData.pSysMem = LineIndices.data();

    if (FAILED(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB)))
        return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_NavMesh::Render()
{
    if (nullptr == m_pVB || nullptr == m_pIB || 0 == m_iNumIndices)
        return S_OK;

    return __super::Render();
}

void CVIBuffer_NavMesh::Release_Buffers()
{
    Safe_Release(m_pVB);
    Safe_Release(m_pIB);

    m_iNumVertexBuffers = 0;
    m_iNumVertices = 0;
    m_iVertexStride = 0;
    m_iNumIndices = 0;
    m_iIndexStride = 0;
    m_eIndexFormat = DXGI_FORMAT_UNKNOWN;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

CVIBuffer_NavMesh* CVIBuffer_NavMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CVIBuffer_NavMesh* pInstance = new CVIBuffer_NavMesh(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CVIBuffer_NavMesh");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CVIBuffer_NavMesh::Clone(void* pArg)
{
    CVIBuffer_NavMesh* pInstance = new CVIBuffer_NavMesh(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CVIBuffer_NavMesh");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVIBuffer_NavMesh::Free()
{
    __super::Free();
}
