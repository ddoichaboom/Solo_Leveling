#include "VIBuffer_Terrain.h"

CVIBuffer_Terrain::CVIBuffer_Terrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer{ pDevice, pContext }
{
}

CVIBuffer_Terrain::CVIBuffer_Terrain(const CVIBuffer_Terrain& Prototype)
    : CVIBuffer { Prototype }
{
}

HRESULT CVIBuffer_Terrain::Initialize_Prototype(const _tchar* pHeightMapFilePath)
{
    // (1) BMP Height Map 읽기 
    _ulong              dwByte = { };
    HANDLE              hFile = CreateFile(pHeightMapFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == 0)
        return E_FAIL;

    BITMAPFILEHEADER    fh{};
    BITMAPINFOHEADER    ih{};
    _uint* pPixels = { nullptr };

    ReadFile(hFile, &fh, sizeof fh, &dwByte, nullptr);
    ReadFile(hFile, &ih, sizeof ih, &dwByte, nullptr);
    m_iNumVerticesX                         = ih.biWidth;
    m_iNumVerticesZ                         = ih.biHeight;
    m_iNumVertices                          = m_iNumVerticesX * m_iNumVerticesZ;

    pPixels = new _uint[m_iNumVertices];
    ZeroMemory(pPixels, sizeof(_uint) * m_iNumVertices);

    ReadFile(hFile, pPixels, sizeof(_uint) * m_iNumVertices, &dwByte, nullptr);

    CloseHandle(hFile);

    // (2) 버퍼 속성 설정
    m_iNumVertexBuffers                     = 1;
    m_iVertexStride                         = sizeof(VTXNORTEX);

    // 인덱스 수 = (X-1) * (Z-1) * 2삼각형 * 3정점
    m_iNumIndices                           = (m_iNumVerticesX - 1) * (m_iNumVerticesZ - 1) * 2 * 3;        
    m_iIndexStride                          = 4;
    m_eIndexFormat                          = DXGI_FORMAT_R32_UINT;

    m_ePrimitiveType                        = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // (3) 정점 버퍼 생성
    D3D11_BUFFER_DESC       VertexBufferDesc{};
    VertexBufferDesc.ByteWidth              = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage                  = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags              = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags         = 0;
    VertexBufferDesc.MiscFlags              = 0;
    VertexBufferDesc.StructureByteStride    = m_iVertexStride;

    VTXNORTEX*                 pVertices       = new VTXNORTEX[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXNORTEX) * m_iNumVertices);

    for (size_t i = 0; i < m_iNumVerticesZ; i++)
    {
        for (size_t j = 0; j < m_iNumVerticesX; j++)
        {
            size_t iIndex = i * m_iNumVerticesX + j;

            pVertices[iIndex].vPosition     = _float3(j, (pPixels[iIndex] & 0x000000ff) / 10.f, i);
            pVertices[iIndex].vNormal       = _float3(0.f, 0.f, 0.f);
            pVertices[iIndex].vTexcoord = _float2(j / (m_iNumVerticesX - 1.f), i / (m_iNumVerticesZ - 1.f));
        }
    }


    // (4) 인덱스 버퍼 생성 + 노멀 계산
    D3D11_BUFFER_DESC               IndexBufferDesc{};
    IndexBufferDesc.ByteWidth               = m_iIndexStride * m_iNumIndices;
    IndexBufferDesc.Usage                   = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags               = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags          = 0;
    IndexBufferDesc.MiscFlags               = 0;
    IndexBufferDesc.StructureByteStride     = m_iIndexStride;

    _uint* pIndices                         = new _uint[m_iNumIndices];
    ZeroMemory(pIndices, sizeof(_uint) * m_iNumIndices);

    _uint iNumIndices                       = {};

    for (size_t i = 0; i < m_iNumVerticesZ -1 ; i++)
    {
        for (size_t j = 0; j < m_iNumVerticesX - 1; j++)
        {
            size_t iIndex = i * m_iNumVerticesX + j;

            // 현재 셀의 4개 정점 인덱스 
            // 좌상 -> 우상 -> 우하 -> 좌하
            _uint iIndices[4] = {
                iIndex + m_iNumVerticesX,
                iIndex + m_iNumVerticesX + 1,
                iIndex + 1,
                iIndex
            };

            _vector vSrc{}, vDst{}, vNor{};

            // 첫번째 삼각형 (우상단 - 대각선 기준)
            pIndices[iNumIndices++] = iIndices[0];
            pIndices[iNumIndices++] = iIndices[1];
            pIndices[iNumIndices++] = iIndices[2];

            // 면 노멀 = Cross(edge1, edge2), 정규화
            vSrc = XMLoadFloat3(&pVertices[iIndices[1]].vPosition) - XMLoadFloat3(&pVertices[iIndices[0]].vPosition);
            vDst = XMLoadFloat3(&pVertices[iIndices[2]].vPosition) - XMLoadFloat3(&pVertices[iIndices[0]].vPosition);
            vNor = XMVector3Normalize(XMVector3Cross(vSrc, vDst));

            // 삼각형(면)을 구성하는 3개의 정점에 해당 노멀 값 누적
            XMStoreFloat3(&pVertices[iIndices[0]].vNormal, XMLoadFloat3(&pVertices[iIndices[0]].vNormal) + vNor);
            XMStoreFloat3(&pVertices[iIndices[1]].vNormal, XMLoadFloat3(&pVertices[iIndices[1]].vNormal) + vNor);
            XMStoreFloat3(&pVertices[iIndices[2]].vNormal, XMLoadFloat3(&pVertices[iIndices[2]].vNormal) + vNor);
                       
            // 두 번째 삼각형 (좌하단 - 대각선 기준)
            pIndices[iNumIndices++] = iIndices[0];
            pIndices[iNumIndices++] = iIndices[2];
            pIndices[iNumIndices++] = iIndices[3];           

            // 면 노멀 = Cross(edge1, edge2), 정규화
            vSrc = XMLoadFloat3(&pVertices[iIndices[2]].vPosition) - XMLoadFloat3(&pVertices[iIndices[0]].vPosition);
            vDst = XMLoadFloat3(&pVertices[iIndices[3]].vPosition) - XMLoadFloat3(&pVertices[iIndices[0]].vPosition);
            vNor = XMVector3Normalize(XMVector3Cross(vSrc, vDst));

            // 삼각형(면)을 구성하는 3개의 정점에 해당 노멀 값 누적
            XMStoreFloat3(&pVertices[iIndices[0]].vNormal, XMLoadFloat3(&pVertices[iIndices[0]].vNormal) + vNor);
            XMStoreFloat3(&pVertices[iIndices[2]].vNormal, XMLoadFloat3(&pVertices[iIndices[2]].vNormal) + vNor);
            XMStoreFloat3(&pVertices[iIndices[3]].vNormal, XMLoadFloat3(&pVertices[iIndices[3]].vNormal) + vNor);
        }
    }

    // (5) 모든 정점의 노멀 정규화 (누적된 면 노멀들의 평균 방향)
    for (size_t i = 0; i < m_iNumVertices; i++)
        XMStoreFloat3(&pVertices[i].vNormal, XMVector3Normalize(XMLoadFloat3(&pVertices[i].vNormal)));

    // (6) VB 생성 (노멀 계산 완료 후)
    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
        return E_FAIL;

    // (7) IB 생성 
    D3D11_SUBRESOURCE_DATA          IndexInitialData{};
    IndexInitialData.pSysMem                = pIndices;

    if (FAILED(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB)))
        return E_FAIL;

    // PICK_DATA 생성
    m_pPickData = new PICK_DATA{};
    m_pPickData->iNumVertices = m_iNumVertices;
    m_pPickData->iNumIndices = m_iNumIndices;

    for (size_t i = 0; i < m_iNumVertices; i++)
        m_pPickData->pVerticesPos[i] = pVertices[i].vPosition;

    m_pPickData->pIndices = new _uint[m_iNumIndices];
    memcpy(m_pPickData->pIndices, pIndices, sizeof(_uint)* m_iNumIndices);

    m_pPickData->pVerticesPos = new _float3[m_iNumVertices];
    

    Safe_Delete_Array(pVertices);
    Safe_Delete_Array(pIndices);
    Safe_Delete_Array(pPixels);

    return S_OK;
}

HRESULT CVIBuffer_Terrain::Initialize(void* pArg)
{
    return S_OK;
}

CVIBuffer_Terrain* CVIBuffer_Terrain::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pHeightMapFilePath)
{
    CVIBuffer_Terrain* pInstance = new CVIBuffer_Terrain(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(pHeightMapFilePath)))
    {
        MSG_BOX("Failed to Created : CVIBuffer_Terrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CVIBuffer_Terrain::Clone(void* pArg)
{
    CVIBuffer_Terrain* pInstance = new CVIBuffer_Terrain(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CVIBuffer_Terrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVIBuffer_Terrain::Free()
{
    __super::Free();
}
