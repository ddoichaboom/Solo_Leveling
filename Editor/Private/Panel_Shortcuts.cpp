#include "Panel_Shortcuts.h"

CPanel_Shortcuts::CPanel_Shortcuts(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Shortcuts::Initialize()
{
	strcpy_s(m_szName, "Shortcuts");

	return S_OK;
}

void CPanel_Shortcuts::Update(_float fTimeDelta)
{

}

void CPanel_Shortcuts::Render()
{
    ImGui::Begin(m_szName, &m_bOpen);

    _bool bCameraMode = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    // ===== 활성 모드 (강조 표시) =====
    if (bCameraMode)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.85f, 0.3f, 1.f));
        ImGui::Text("[ Camera Control - ACTIVE ]");
        ImGui::PopStyleColor();
        ImGui::Separator();
        Render_CameraShortcuts();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.85f, 1.f, 1.f));
        ImGui::Text("[ Gizmo - ACTIVE ]");
        ImGui::PopStyleColor();
        ImGui::Separator();
        Render_GizmoShortcuts();
    }

    // ===== 비활성 모드 (회색 요약) =====
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
    if (bCameraMode)
    {
        ImGui::Text("Gizmo (release RMB):");
        ImGui::BulletText("W / E / R : Translate / Rotate / Scale");
        ImGui::BulletText("X : Local / World toggle");
        ImGui::BulletText("Ctrl + Gizmo Drag : Snap");
    }
    else
    {
        ImGui::Text("Camera (hold RMB):");
        ImGui::BulletText("W / A / S / D : Forward / Left / Back / Right");
        ImGui::BulletText("Q / E : Down / Up");
        ImGui::BulletText("Mouse : Look around");
    }
    ImGui::PopStyleColor();

    ImGui::End();
}

void CPanel_Shortcuts::Render_CameraShortcuts()
{
    ImGui::BulletText("W / S : Forward / Backward");
    ImGui::BulletText("A / D : Left / Right");
    ImGui::BulletText("Q / E : Down / Up");
    ImGui::BulletText("Mouse : Look around");
    ImGui::Spacing();
    ImGui::TextDisabled("Release RMB to exit");
}

void CPanel_Shortcuts::Render_GizmoShortcuts()
{
    ImGui::BulletText("W : Translate");
    ImGui::BulletText("E : Rotate");
    ImGui::BulletText("R : Scale");
    ImGui::BulletText("X : Local / World toggle");
    ImGui::BulletText("Ctrl : Snap Mode");

    ImGui::Spacing();
    ImGui::TextDisabled("Hold RMB to enter camera mode");
}

CPanel_Shortcuts* CPanel_Shortcuts::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPanel_Shortcuts* pInstance = new CPanel_Shortcuts(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Create : CPanel_Shortcuts");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPanel_Shortcuts::Free()
{
    __super::Free();
}
