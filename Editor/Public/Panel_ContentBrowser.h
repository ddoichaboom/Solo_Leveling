#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

namespace fs = std::filesystem;

class CPanel_ContentBrowser final : public CPanel
{
private:
    CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_ContentBrowser() = default;

public:
    virtual HRESULT                 Initialize() override;
    virtual void                    Update(_float fTimeDelta) override;
    virtual void                    Render() override;

private:
    // 디렉토리 내용 캐싱 (매 프레임 filesystem 순회 방지)
    void                            Refresh();

    // UI 헬퍼 
    void                            Render_Breadcrumb();
    void                            Render_Contents();
    const _char*                    Get_FileIcon(const fs::path& ext) const;

    void                            Render_ConvertPopup();

private:
    fs::path                        m_RootPath;        // Resources/ 절대 경로
    fs::path                        m_CurrentPath;      // 현재 탐색 경로


    // 캐싱된 디렉토리 엔트리
    vector<fs::directory_entry>     m_Directories;
    vector<fs::directory_entry>     m_Files;

    // 선택 상태
    fs::path                        m_SelectedPath;

    _bool                           m_bNeedRefresh = { true };

    // FBX 변환 팝업 상태
    fs::path                        m_FbxToConvertPath;
    _int                            m_iModelType = { ETOI(MODEL::NONANIM) };
    _bool                           m_bOpenConvertPopup = { false };

    _float3                         m_vPreScale         = { 1.f, 1.f, 1.f };
    _float3                         m_vPreRotationDeg   = { 0.f, 0.f, 0.f };

public:
    static CPanel_ContentBrowser*   Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                    Free() override;

};

NS_END