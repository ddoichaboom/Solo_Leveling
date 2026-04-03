  ---
  Panel_ContentBrowser.h

  #pragma once

  #include "Editor_Defines.h"
  #include "Panel.h"
  #include <filesystem>

  NS_BEGIN(Editor)

  class CPanel_ContentBrowser final : public CPanel
  {
  private:
      CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
      virtual ~CPanel_ContentBrowser() = default;

  public:
      virtual HRESULT         Initialize() override;
      virtual void            Update(_float fTimeDelta) override;
      virtual void            Render() override;

  private:
      /* 디렉토리 내용 캐싱 (매 프레임 filesystem 순회 방지) */
      void                    Refresh();

      /* UI 헬퍼 */
      void                    Render_Breadcrumb();
      void                    Render_Contents();
      const _char*            Get_FileIcon(const std::filesystem::path& ext) const;

  private:
      std::filesystem::path                       m_RootPath;         // Resources/ 절대 경로
      std::filesystem::path                       m_CurrentPath;      // 현재 탐색 경로

      /* 캐싱된 디렉토리 엔트리 */
      std::vector<std::filesystem::directory_entry>    m_Directories;
      std::vector<std::filesystem::directory_entry>    m_Files;

      /* 선택 상태 */
      std::filesystem::path                       m_SelectedPath;

      _bool                                       m_bNeedRefresh = { true };

  public:
      static CPanel_ContentBrowser* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
      virtual void            Free() override;
  };

  NS_END

  ---
  Panel_ContentBrowser.cpp

  #include "Panel_ContentBrowser.h"

  namespace fs = std::filesystem;

  CPanel_ContentBrowser::CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
      : CPanel{ pDevice, pContext }
  {
  }

  HRESULT CPanel_ContentBrowser::Initialize()
  {
      strcpy_s(m_szName, "Content Browser");

      /* Resources/ 경로 설정 — Editor.exe 기준 상대 경로 */
      m_RootPath = fs::absolute("../../Resources");

      if (!fs::exists(m_RootPath))
      {
          MSG_BOX("Content Browser: Resources/ 경로를 찾을 수 없습니다.");
          return E_FAIL;
      }

      m_RootPath = fs::canonical(m_RootPath);
      m_CurrentPath = m_RootPath;
      m_bNeedRefresh = true;

      return S_OK;
  }

  void CPanel_ContentBrowser::Update(_float fTimeDelta)
  {
  }

  void CPanel_ContentBrowser::Render()
  {
      ImGui::Begin(m_szName, &m_bOpen);

      /* 캐시 갱신 */
      if (m_bNeedRefresh)
          Refresh();

      /* 상단: Breadcrumb 경로 */
      Render_Breadcrumb();

      ImGui::Separator();

      /* 내용물: 폴더 + 파일 */
      Render_Contents();

      ImGui::End();
  }

  /* ================================================================ */
  /*                          Private                                  */
  /* ================================================================ */

  void CPanel_ContentBrowser::Refresh()
  {
      m_Directories.clear();
      m_Files.clear();

      if (!fs::exists(m_CurrentPath))
      {
          m_bNeedRefresh = false;
          return;
      }

      for (const auto& entry : fs::directory_iterator(m_CurrentPath))
      {
          if (entry.is_directory())
              m_Directories.push_back(entry);
          else
              m_Files.push_back(entry);
      }

      /* 이름순 정렬 */
      auto sortByName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
          return a.path().filename().string() < b.path().filename().string();
      };

      std::sort(m_Directories.begin(), m_Directories.end(), sortByName);
      std::sort(m_Files.begin(), m_Files.end(), sortByName);

      m_bNeedRefresh = false;
  }

  void CPanel_ContentBrowser::Render_Breadcrumb()
  {
      /* Root로 돌아가기 버튼 */
      if (ImGui::Button("Resources"))
      {
          m_CurrentPath = m_RootPath;
          m_bNeedRefresh = true;
      }

      /* Root 이후의 상대 경로를 / 단위로 분할하여 클릭 가능한 breadcrumb 생성 */
      fs::path relativePath = fs::relative(m_CurrentPath, m_RootPath);

      if (!relativePath.empty() && relativePath != ".")
      {
          fs::path accumulated = m_RootPath;
          for (const auto& part : relativePath)
          {
              accumulated /= part;

              ImGui::SameLine();
              ImGui::Text(">");
              ImGui::SameLine();

              /* 각 경로 조각을 버튼으로 */
              if (ImGui::Button(part.string().c_str()))
              {
                  m_CurrentPath = accumulated;
                  m_bNeedRefresh = true;
              }
          }
      }

      /* 뒤로가기: Root가 아닐 때만 */
      if (m_CurrentPath != m_RootPath)
      {
          ImGui::SameLine();
          if (ImGui::Button("<-"))
          {
              m_CurrentPath = m_CurrentPath.parent_path();
              m_bNeedRefresh = true;
          }
      }
  }

  void CPanel_ContentBrowser::Render_Contents()
  {
      /* 폴더 먼저 */
      for (const auto& dir : m_Directories)
      {
          std::string label = std::string("[D] ") + dir.path().filename().string();

          if (ImGui::Selectable(label.c_str(), m_SelectedPath == dir.path()))
              m_SelectedPath = dir.path();

          /* 더블클릭 → 폴더 진입 */
          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          {
              m_CurrentPath = dir.path();
              m_SelectedPath = fs::path();
              m_bNeedRefresh = true;
          }
      }

      /* 파일 */
      for (const auto& file : m_Files)
      {
          const _char* icon = Get_FileIcon(file.path().extension());
          std::string label = std::string(icon) + " " + file.path().filename().string();

          if (ImGui::Selectable(label.c_str(), m_SelectedPath == file.path()))
              m_SelectedPath = file.path();
      }
  }

  const _char* CPanel_ContentBrowser::Get_FileIcon(const std::filesystem::path& ext) const
  {
      std::string strExt = ext.string();

      /* 소문자 변환 */
      for (auto& c : strExt)
          c = static_cast<_char>(::tolower(c));

      if (strExt == ".hlsl" || strExt == ".fx")       return "[S]";   // Shader
      if (strExt == ".dds" || strExt == ".jpg" ||
          strExt == ".jpeg" || strExt == ".png" ||
          strExt == ".bmp" || strExt == ".tga")       return "[T]";   // Texture
      if (strExt == ".fbx" || strExt == ".obj")       return "[M]";   // Model
      if (strExt == ".model")                         return "[B]";   // Binary Model

      return "[?]";
  }

  CPanel_ContentBrowser* CPanel_ContentBrowser::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
  {
      CPanel_ContentBrowser* pInstance = new CPanel_ContentBrowser(pDevice, pContext);

      if (FAILED(pInstance->Initialize()))
      {
          MSG_BOX("Failed to Create : CPanel_ContentBrowser");
          Safe_Release(pInstance);
      }

      return pInstance;
  }

  void CPanel_ContentBrowser::Free()
  {
      __super::Free();
  }

  ---
  기능 요약

  ┌────────────────┬─────────────────────────────────────────────────┐
  │      기능      │                      구현                       │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ 디렉토리 순회  │ std::filesystem::directory_iterator + 캐싱      │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ Breadcrumb     │ 경로 조각별 클릭 버튼 + <- 뒤로가기             │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ 폴더 진입      │ 더블클릭                                        │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ 파일 타입 구분 │ [D]폴더 [T]텍스처 [S]셰이더 [M]모델 [B]바이너리 │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ 선택 상태      │ Selectable 하이라이트                           │
  ├────────────────┼─────────────────────────────────────────────────┤
  │ Root 제한      │ Resources/ 바깥으로 못 나감                     │
  └────────────────┴─────────────────────────────────────────────────┘

  2개 파일(Panel_ContentBrowser.h, .cpp)만 변경하면 됩니다. 적용 후 빌드해보세요.