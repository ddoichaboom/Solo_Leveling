#pragma once
#include "Engine_Defines.h"
#include <stdio.h>

NS_BEGIN(Engine)

class ENGINE_DLL CBinaryReader final
{
public:
    CBinaryReader() = default;
    ~CBinaryReader();

    CBinaryReader(const CBinaryReader&) = delete;
    CBinaryReader& operator=(const CBinaryReader&) = delete;

public:
    HRESULT Open(const _tchar* pFilePath);
    void Close();

    _bool Is_Open() const { return nullptr != m_pFile; }

    HRESULT ReadBytes(void* pOutData, size_t iByteSize);
    HRESULT ReadArray(void* pOutData, size_t iStride, _uint iCount);
    HRESULT ReadMagic(const char* pExpectedMagic, size_t iLength);
    HRESULT ReadVersion(_uint* pOutVersion, _uint iMinVersion, _uint iMaxVersion);

    template<typename T>
    HRESULT Read(T* pOutValue)
    {
        return ReadBytes(pOutValue, sizeof(T));
    }

private:
    FILE* m_pFile = { nullptr };
};

NS_END
