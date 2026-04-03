#include "Panel_ContentBrowser.h"

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
		std::string label = std::string("[D] ") + dir.path().filename().string();

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
		std::string label = std::string(icon) + " " + file.path().filename().string();

		if (ImGui::Selectable(label.c_str(), m_SelectedPath == file.path()))
			m_SelectedPath = file.path();
	}
}

const _char* CPanel_ContentBrowser::Get_FileIcon(const fs::path& ext) const
{
	string strExt = ext.string();

	// 소문자 변환 
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
