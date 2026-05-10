#pragma once
#include "Engine_Defines.h"
#include <stdio.h>

NS_BEGIN(Engine)

class ENGINE_DLL CBinaryWriter final
{
public:
    CBinaryWriter() = default;
    ~CBinaryWriter();

    CBinaryWriter(const CBinaryWriter&) = delete;
    CBinaryWriter& operator=(const CBinaryWriter&) = delete;

public:
    HRESULT Open(const _tchar* pFilePath);
    void Close();

    _bool Is_Open() const { return nullptr != m_pFile; }

    HRESULT WriteBytes(const void* pData, size_t iByteSize);
    HRESULT WriteArray(const void* pData, size_t iStride, _uint iCount);
    HRESULT WriteMagic(const char* pMagic, size_t iLength);
    HRESULT WriteVersion(_uint iVersion);

    template<typename T>
    HRESULT Write(const T& Value)
    {
        return WriteBytes(&Value, sizeof(T));
    }

private:
    FILE* m_pFile = { nullptr };
};

NS_END

