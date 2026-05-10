#include "BinaryReader.h"
#include <cstring>

CBinaryReader::~CBinaryReader()
{
    Close();
}

HRESULT CBinaryReader::Open(const _tchar* pFilePath)
{
    Close();

    if (nullptr == pFilePath || 0 == pFilePath[0])
        return E_FAIL;

    if (0 != _wfopen_s(&m_pFile, pFilePath, L"rb") || nullptr == m_pFile)
        return E_FAIL;

    return S_OK;
}

void CBinaryReader::Close()
{
    if (nullptr != m_pFile)
    {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
}

HRESULT CBinaryReader::ReadBytes(void* pOutData, size_t iByteSize)
{
    if (0 == iByteSize)
        return S_OK;

    if (nullptr == m_pFile || nullptr == pOutData)
        return E_FAIL;

    return iByteSize == fread(pOutData, 1, iByteSize, m_pFile) ? S_OK : E_FAIL;
}

HRESULT CBinaryReader::ReadArray(void* pOutData, size_t iStride, _uint iCount)
{
    if (0 == iCount)
        return S_OK;

    if (0 == iStride)
        return E_FAIL;

    return ReadBytes(pOutData, iStride * iCount);
}

HRESULT CBinaryReader::ReadMagic(const char* pExpectedMagic, size_t iLength)
{
    if (nullptr == pExpectedMagic || 0 == iLength)
        return E_FAIL;

    vector<char> Magic(iLength);

    if (FAILED(ReadBytes(Magic.data(), iLength)))
        return E_FAIL;

    return 0 == memcmp(Magic.data(), pExpectedMagic, iLength) ? S_OK : E_FAIL;
}

HRESULT CBinaryReader::ReadVersion(_uint* pOutVersion, _uint iMinVersion, _uint iMaxVersion)
{
    if (nullptr == pOutVersion || iMinVersion > iMaxVersion)
        return E_FAIL;

    if (FAILED(Read(pOutVersion)))
        return E_FAIL;

    if (*pOutVersion < iMinVersion || *pOutVersion > iMaxVersion)
        return E_FAIL;

    return S_OK;
}