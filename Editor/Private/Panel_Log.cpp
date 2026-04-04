#include "Panel_Log.h"

CPanel_Log::CPanel_Log(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Log::Initialize()
{
	strcpy_s(m_szName, "Log");

	return S_OK;
}

void CPanel_Log::Update(_float fTimeDelta)
{
}

void CPanel_Log::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

    // Info 버튼 (녹색 계열)
    if (m_bShowInfo)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

    if (ImGui::SmallButton("Info"))
        m_bShowInfo = !m_bShowInfo;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Warning 버튼 (노란색 계열)
    if (m_bShowWarning)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.8f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

    if (ImGui::SmallButton("Warning"))
        m_bShowWarning = !m_bShowWarning;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Error 버튼 (빨간색 계열)
    if (m_bShowError)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

    if (ImGui::SmallButton("Error"))
        m_bShowError = !m_bShowError;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    if (ImGui::SmallButton("Clear"))
        Clear_Log();

    ImGui::Separator();

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    const auto& LogBuffer = Get_LogBuffer();

    for (const auto& Entry : LogBuffer)
    {
        // 필터 체크
        if (Entry.eLevel == LOG_LEVEL::INFO && !m_bShowInfo)          continue;
        if (Entry.eLevel == LOG_LEVEL::WARNING && !m_bShowWarning)    continue;
        if (Entry.eLevel == LOG_LEVEL::ERROR_ && !m_bShowError)       continue;

        // 레벨별 색상 + 접두사
        ImVec4 vColor;
        const char* szPrefix = "";

        switch (Entry.eLevel)
        {
        case LOG_LEVEL::INFO:
            vColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
            szPrefix = "[Info]    ";
            break;
        case LOG_LEVEL::WARNING:
            vColor = ImVec4(1.0f, 0.9f, 0.3f, 1.0f);
            szPrefix = "[Warning] ";
            break;
        case LOG_LEVEL::ERROR_:
            vColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            szPrefix = "[Error]   ";
            break;
        }

        ImGui::TextColored(vColor, "%s%s", szPrefix, Entry.strMessage.c_str());
    }

    // ── 자동 스크롤 ──
      // 사용자가 맨 아래에 있을 때만 자동 스크롤 유지
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        m_bAutoScroll = true;
    else
        m_bAutoScroll = false;

    if (m_bAutoScroll)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::End();
}

CPanel_Log* CPanel_Log::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Log* pInstance = new CPanel_Log(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Log");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Log::Free()
{
	__super::Free();
}
