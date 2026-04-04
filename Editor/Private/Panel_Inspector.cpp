#include "Panel_Inspector.h"
#include "Panel_Manager.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Transform.h"


CPanel_Inspector::CPanel_Inspector(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Inspector::Initialize()
{
	strcpy_s(m_szName, "Inspector");

	return S_OK;
}

void CPanel_Inspector::Update(_float fTimeDelta)
{
}

void CPanel_Inspector::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	CGameObject* pSelected = m_pPanel_Manager->Get_SelectedObject();
	if (nullptr == pSelected)
	{
		ImGui::TextDisabled("No Object Selected");
		ImGui::End();
		return;
	}

	Render_GameObject(pSelected);

	CTransform* pTransform = pSelected->Get_Transform();
	if (nullptr == pTransform)
		Render_Transform(pTransform);

	ImGui::End();
}

void CPanel_Inspector::Render_GameObject(CGameObject* pObject)
{
	if (!ImGui::CollapsingHeader("GameObject", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	rttr::type type = rttr::type::get<CGameObject>();

	for (auto& prop : type.get_properties())
	{
		Render_Property(prop, *pObject);
	}
}

void CPanel_Inspector::Render_Transform(CTransform* pTransform)
{
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	rttr::type type = rttr::type::get<CTransform>();

	for (auto& prop : type.get_properties())
	{
		Render_Property(prop, *pTransform);
	}
}

void CPanel_Inspector::Render_Property(rttr::property prop, rttr::instance instance)
{
	std::string strName = prop.get_name().to_string();
	rttr::variant value = prop.get_value(instance);

	if (!value.is_valid())
		return;

	// _float3 -> DragFloat3
	if (value.is_type<_float3>())
	{
		_float3 v = value.get_value<_float3>();

		if (ImGui::DragFloat3(strName.c_str(), &v.x, 0.1f))
		{
			prop.set_value(instance, v);
		}
	}
	// _float -> DragFloat
	else if (value.is_type<_float>())
	{
		_float f = value.get_value<_float>();

		if (ImGui::DragFloat(strName.c_str(), &f, 0.1f))
		{
			prop.set_value(instance, f);
		}
	}
	// _bool -> CheckBox
	else if (value.is_type<_bool>())
	{
		_bool b = value.get_value<_bool>();

		if (ImGui::Checkbox(strName.c_str(), &b))
		{
			prop.set_value(instance, b);
		}
	}
	// _int -> DragInt
	else if (value.is_type<_int>())
	{
		_int i = value.get_value<_int>();

		if (ImGui::DragInt(strName.c_str(), &i))
		{
			prop.set_value(instance, i);
		}
	}
	// wstring -> InputText
	else if (value.is_type<_wstring>())
	{
		_wstring wstr = value.get_value<_wstring>();
		std::string str = WTOA(wstr);

		char szBuff[256] = {};
		strcpy_s(szBuff, str.c_str());

		if (ImGui::InputText(strName.c_str(), szBuff, sizeof(szBuff),
			ImGuiInputTextFlags_EnterReturnsTrue))
		{
			prop.set_value(instance, ATOW(std::string(szBuff)));
		}
	}
	// 미지원 타입 (필요시 확장)
	else
	{
		ImGui::TextDisabled("%s: (Unsupported Type)", strName.c_str());
	}
}

CPanel_Inspector* CPanel_Inspector::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Inspector* pInstance = new CPanel_Inspector(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Inspector");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Inspector::Free()
{
	__super::Free();
}
