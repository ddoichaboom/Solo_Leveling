#ifndef Editor_Function_h__
#define Editor_Function_h__

namespace Editor
{
    inline bool Get_MonitorResolution(_Out_ unsigned int* pWidth, _Out_ unsigned int* pHeight)
    {
        IDXGIFactory* pFactory = { nullptr };
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
            return false;

        IDXGIAdapter* pAdapter = { nullptr };
        if (FAILED(pFactory->EnumAdapters(0, &pAdapter)))
        {
            pFactory->Release();
            return false;
        }

        IDXGIOutput* pOutput = nullptr;
        if (FAILED(pAdapter->EnumOutputs(0, &pOutput)))
        {
            pAdapter->Release();
            pFactory->Release();
            return false;
        }

        DXGI_OUTPUT_DESC outputDesc{};
        pOutput->GetDesc(&outputDesc);

        *pWidth = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        *pHeight = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        pOutput->Release();
        pAdapter->Release();
        pFactory->Release();

        return true;
    }

    inline std::string WTOA(const std::wstring& wstr)
    {
        if (wstr.empty())
            return {};
        int iSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                        static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

        std::string str(iSize, 0);

        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
            static_cast<int>(wstr.size()), &str[0], iSize, nullptr, nullptr);

        return str;
    }

    inline std::wstring ATOW(const std::string& str)
    {
        if (str.empty())
            return {};

        int iSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                        static_cast<int>(str.size()), nullptr, 0);

        std::wstring wstr(iSize, 0);

        MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                        static_cast<int>(str.size()), &wstr[0], iSize);

        return wstr;
    }

    inline std::vector<LOG_DESC>& Get_LogBuffer()
    {
        static std::vector<LOG_DESC> s_LogBuffer;
        return s_LogBuffer;
    }

    inline void Log_Message(LOG_LEVEL eLevel, const std::string& strMsg)
    {
        Get_LogBuffer().push_back({ eLevel, strMsg });
    }

    inline void Log_Info(const std::string& strMsg)
    {
        Log_Message(LOG_LEVEL::INFO, strMsg);
    }

    inline void Log_Warning(const std::string& strMsg)
    {
        Log_Message(LOG_LEVEL::WARNING, strMsg);
    }

    inline void Log_Error(const std::string& strMsg)
    {
        Log_Message(LOG_LEVEL::ERROR_, strMsg);
    }

    inline void Clear_Log()
    {
        Get_LogBuffer().clear();
    }
}

#endif // Editor_Function.h
