#include "BinaryWriter.h"

CBinaryWriter::~CBinaryWriter()
{
    Close();
}

HRESULT CBinaryWriter::Open(const _tchar* pFilePath)
{
    Close();

    if (nullptr == pFilePath || 0 == pFilePath[0])
        return E_FAIL;

    if (0 != _wfopen_s(&m_pFile, pFilePath, L"wb") || nullptr == m_pFile)
        return E_FAIL;

    return S_OK;
}

void CBinaryWriter::Close()
{
    if (nullptr != m_pFile)
    {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
}

HRESULT CBinaryWriter::WriteBytes(const void* pData, size_t iByteSize)
{
    if (0 == iByteSize)
        return S_OK;

    if (nullptr == m_pFile || nullptr == pData)
        return E_FAIL;

    return iByteSize == fwrite(pData, 1, iByteSize, m_pFile) ? S_OK : E_FAIL;
}

HRESULT CBinaryWriter::WriteArray(const void* pData, size_t iStride, _uint iCount)
{
    if (0 == iCount)
        return S_OK;

    if (0 == iStride)
        return E_FAIL;

    return WriteBytes(pData, iStride * iCount);
}

HRESULT CBinaryWriter::WriteMagic(const char* pMagic, size_t iLength)
{
    if (nullptr == pMagic || 0 == iLength)
        return E_FAIL;

    return WriteBytes(pMagic, iLength);
}

HRESULT CBinaryWriter::WriteVersion(_uint iVersion)
{
    return Write(iVersion);
}