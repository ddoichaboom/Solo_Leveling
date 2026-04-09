#include "Panel_ContentBrowser.h"
#include "Model_Converter.h"

CPanel_ContentBrowser::CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_ContentBrowser::Initialize()
{
	strcpy_s(m_szName, "Content Browser");

	// Resources 경로 설정
	m_RootPath = fs::absolute("../../Resources");

	if (!fs::exists(m_RootPath))
	{
		MSG_BOX("Content Browser: Resources/ 경로를 찾을 수 없습니다.");
		return E_FAIL;
	}

	m_RootPath = fs::canonical(m_RootPath);
	m_CurrentPath = m_RootPath;
	m_bNeedRefresh = { true };

	return S_OK;
}

void CPanel_ContentBrowser::Update(_float fTimeDelta)
{
}

void CPanel_ContentBrowser::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	// 캐시 갱신
	if (m_bNeedRefresh)
		Refresh();

	// 상단 Breadcrumb 경로
	Render_Breadcrumb();
	
	ImGui::Separator();

	// 내용 : 폴더 + 파일
	Render_Contents();

	// 팝업 오픈 트리거
	if (m_bOpenConvertPopup)
	{
		ImGui::OpenPopup("FBX Convert");
		m_bOpenConvertPopup = false;
	}
	Render_ConvertPopup();

	ImGui::End();
}

void CPanel_ContentBrowser::Refresh()
{
	m_Directories.clear();
	m_Files.clear();

	if (!fs::exists(m_CurrentPath))
	{
		m_bNeedRefresh = { false };
		return;
	}

	for (const auto& entry : fs::directory_iterator(m_CurrentPath))
	{
		if (entry.is_directory())
			m_Directories.push_back(entry);
		else
			m_Files.push_back(entry);
	}

	// 이름순 정렬
	auto SortByName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
		return a.path().filename().string() < b.path().filename().string();
		};

	sort(m_Directories.begin(), m_Directories.end(), SortByName);
	sort(m_Files.begin(), m_Files.end(), SortByName);

	m_bNeedRefresh = { false };
}

void CPanel_ContentBrowser::Render_Breadcrumb()
{
	// Root로 돌아가기
	if (ImGui::Button("Resources"))
	{
		m_CurrentPath = m_RootPath;
		m_bNeedRefresh = { true };
	}

	// Root 이후의 상대 경로를 / 단위로 분할하여 클릭 가능한 breadcrumb 생성
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

			// 각 경로 조각을 버튼으로 
			if (ImGui::Button(part.string().c_str()))
			{
				m_CurrentPath = accumulated;
				m_bNeedRefresh = true;
			}
		}
	}

	// 뒤로가기: Root가 아닐 때만 
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
	// 폴더 먼저 
	for (const auto& dir : m_Directories)
	{
		string label = "[D] " + dir.path().filename().string();

		if (ImGui::Selectable(label.c_str(), m_SelectedPath == dir.path()))
			m_SelectedPath = dir.path();

		// 더블클릭 → 폴더 진입 
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			m_CurrentPath = dir.path();
			m_SelectedPath = fs::path();
			m_bNeedRefresh = true;
		}
	}

	// 파일 
	for (const auto& file : m_Files)
	{
		const _char* icon = Get_FileIcon(file.path().extension());
		string label = string(icon) + " " + file.path().filename().string();

		if (ImGui::Selectable(label.c_str(), m_SelectedPath == file.path()))
			m_SelectedPath = file.path();

		// .fbx 파일 우클릭 컨텍스트 메뉴
		string extLower = file.path().extension().string();
		for (auto& c : extLower)
			c = static_cast<_char>(::tolower(c));

		if (extLower == ".fbx")
		{
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Convert to .bin"))
				{
					m_FbxToConvertPath = file.path();
					m_bOpenConvertPopup = true;
				}
				ImGui::EndPopup();
			}
		}
	}
}

const _char* CPanel_ContentBrowser::Get_FileIcon(const fs::path& ext) const
{
	string strExt = ext.string();

	// 소문자 변환 
	for (auto& c : strExt)
		c = static_cast<_char>(::tolower(c));

	if (strExt == ".hlsl" || strExt == ".fx")       
		return "[S]";   // Shader

	if (strExt == ".dds" || strExt == ".jpg" ||
		strExt == ".jpeg" || strExt == ".png" ||
		strExt == ".bmp" || strExt == ".tga")       
		return "[T]";   // Texture

	if (strExt == ".fbx" || strExt == ".obj")       
		return "[M]";   // Model

	if (strExt == ".bin")                         
		return "[B]";   // Binary Model


	return "[?]";
}

void CPanel_ContentBrowser::Render_ConvertPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("FBX Convert", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	// 경로 표시
	ImGui::TextDisabled("FBX");
	ImGui::SameLine();
	ImGui::TextUnformatted(m_FbxToConvertPath.filename().string().c_str());

	fs::path binPath = m_FbxToConvertPath;
	binPath.replace_extension(".bin");

	ImGui::TextDisabled("BIN");
	ImGui::SameLine();
	ImGui::TextUnformatted(binPath.filename().string().c_str());

	ImGui::Separator();

	// 모델 타입 선택
	ImGui::Text("Model Type");
	ImGui::RadioButton("NONANIM", &m_iModelType, 0);
	ImGui::SameLine();
	ImGui::RadioButton("ANIM", &m_iModelType, 1);

	ImGui::Separator();

	// Convert / Cancel
	if (ImGui::Button("Convert", ImVec2(120.f, 0.f)))
	{
		MODEL eType = (0 == m_iModelType) ? MODEL::NONANIM : MODEL::ANIM;

		wstring fbxW = m_FbxToConvertPath.wstring();
		wstring binW = binPath.wstring();

		if (SUCCEEDED(CModel_Converter::Convert(fbxW.c_str(), binW.c_str(), eType)))
		{
			Log_Info(WTOA(L"[Converter] 성공: " + binPath.filename().wstring()));
			m_bNeedRefresh = true;  // .bin 파일이 생겼으므로 목록 갱신
		}
		else
		{
			Log_Error(WTOA(L"[Converter] 실패: " + m_FbxToConvertPath.filename().wstring()));
		}

		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
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
