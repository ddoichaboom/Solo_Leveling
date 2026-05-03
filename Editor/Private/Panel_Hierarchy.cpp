#include "Panel_Hierarchy.h"
#include "Panel_Manager.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameObject.h"

namespace
{
	constexpr const char* DND_PAYLOAD_TYPE = "DND_HIER_OBJ";
	constexpr const char* DND_PROTOTYPE_TYPE = "DND_PROTOTYPE";

	struct DND_OBJECT
	{
		CGameObject*	pObject; 
		const _wstring* pSrcLayer;
	};

	struct DND_PROTOTYPE
	{
		_uint			iLevel;
		wchar_t			szTag[MAX_PATH];
	};
}

CPanel_Hierarchy::CPanel_Hierarchy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Hierarchy::Initialize()
{
	strcpy_s(m_szName, "Hierarchy");
	return S_OK;
}

void CPanel_Hierarchy::Update(_float fTimeDelta)
{
	_int iCurLevel = m_pGameInstance->Get_CurrentLevelIndex();
	if (iCurLevel != m_iPrevLevelIndex)
	{
		m_pPanel_Manager->Clear_Selection();
		m_iPrevLevelIndex = iCurLevel;
	}
}

void CPanel_Hierarchy::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	_int iLevelIndex = m_pGameInstance->Get_CurrentLevelIndex();
	if (iLevelIndex < 0)
	{
		ImGui::TextDisabled("No Level Loaded");
		ImGui::End();
		return;
	}

	const auto* pLayers = m_pGameInstance->Get_Layers(static_cast<_uint>(iLevelIndex));
	if (nullptr == pLayers)
	{
		ImGui::TextDisabled("No Layers");
		ImGui::End();
		return;
	}

	CGameObject* pSelected = m_pPanel_Manager->Get_SelectedObject();
	const _bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	for (const auto& Pair : *pLayers)
	{
		const _wstring& strLayerKey = Pair.first;
		const string strLayerName = WTOA(strLayerKey);
		const auto& ObjList = Pair.second->Get_GameObjects();
		const _uint iObjectCount = static_cast<_uint>(ObjList.size());

		_char szLabel[MAX_PATH] = {};
		sprintf_s(szLabel, "%s (%u)", strLayerName.c_str(), iObjectCount);

		const _bool bOpen = ImGui::TreeNode(szLabel);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(DND_PAYLOAD_TYPE))
			{
				const DND_OBJECT* pSrc = static_cast<const DND_OBJECT*>(pPayload->Data);
				if (nullptr != pSrc && nullptr != pSrc->pObject && nullptr != pSrc->pSrcLayer && *pSrc->pSrcLayer != strLayerKey)
				{
					m_PendingCmd.eType = CMD_TYPE::MOVE;
					m_PendingCmd.pObject = pSrc->pObject;
					m_PendingCmd.strSrcLayer = *pSrc->pSrcLayer;
					m_PendingCmd.strDstLayer = strLayerKey;
					m_PendingCmd.iInsertIndex = UINT_MAX;
				}
			}

			if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(DND_PROTOTYPE_TYPE))
			{
				const DND_PROTOTYPE* pSrc = static_cast<const DND_PROTOTYPE*>(pPayload->Data);
				if (nullptr != pSrc && L'\0' != pSrc->szTag[0])
				{
					m_PendingCmd.eType = CMD_TYPE::SPAWN;
					m_PendingCmd.strDstLayer = strLayerKey;
					m_PendingCmd.iSpawnProtoLevel = pSrc->iLevel;
					m_PendingCmd.strSpawnProtoTag = pSrc->szTag;     // wchar_t* → wstring 자동 변환
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (bOpen)
		{
			_uint iIndex = 0;
			for (auto& pObject : ObjList)
			{
				ImGui::PushID(static_cast<int>(iIndex));

				string strDisplay;
				if (!pObject->Get_Name().empty())
					strDisplay = WTOA(pObject->Get_Name());
				else if (!pObject->Get_Tag().empty())
					strDisplay = WTOA(pObject->Get_Tag());
				else
					strDisplay = "(Unnamed)";

				const _bool bIsSelected = (pObject == pSelected);

				if (ImGui::Selectable(strDisplay.c_str(), bIsSelected))
					m_pPanel_Manager->Set_SelectedObject(pObject);

				// DnD source
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					DND_OBJECT Payload{ pObject, &strLayerKey };
					ImGui::SetDragDropPayload(DND_PAYLOAD_TYPE, &Payload, sizeof(Payload));
					ImGui::Text("%s", strDisplay.c_str());
					ImGui::EndDragDropSource();
				}

				// DnD target on item: "이 아이템 *앞에* 삽입"
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(DND_PAYLOAD_TYPE))
					{
						const DND_OBJECT* pSrc = static_cast<const DND_OBJECT*>(pPayload->Data);
						if (nullptr != pSrc && nullptr != pSrc->pObject && pSrc->pObject != pObject
							&& nullptr != pSrc->pSrcLayer)
						{
							const _bool bSameLayer = (*pSrc->pSrcLayer == strLayerKey);
							m_PendingCmd.eType = bSameLayer ? CMD_TYPE::REORDER : CMD_TYPE::MOVE;
							m_PendingCmd.pObject = pSrc->pObject;
							m_PendingCmd.strSrcLayer = *pSrc->pSrcLayer;
							m_PendingCmd.strDstLayer = strLayerKey;
							m_PendingCmd.iInsertIndex = iIndex;
						}
					}
					ImGui::EndDragDropTarget();
				}

				// 우클릭 컨텍스트 메뉴
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete"))
					{
						m_PendingCmd.eType = CMD_TYPE::REMOVE;
						m_PendingCmd.pObject = pObject;
						m_PendingCmd.strSrcLayer = strLayerKey;
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
				++iIndex;
			}
			ImGui::TreePop();
		}
	}

	// Delete 키 — 창 포커스 + 선택 객체 존재 시
	if (bWindowFocused && nullptr != pSelected
		&& ImGui::IsKeyPressed(ImGuiKey_Delete, false)
		&& m_PendingCmd.eType == CMD_TYPE::NONE)
	{
		// 선택 객체의 소속 레이어를 역탐색 (보유 정보가 객체 포인터뿐이므로)
		for (const auto& Pair : *pLayers)
		{
			const auto& List = Pair.second->Get_GameObjects();
			if (std::find(List.begin(), List.end(), pSelected) != List.end())
			{
				m_PendingCmd.eType = CMD_TYPE::REMOVE;
				m_PendingCmd.pObject = pSelected;
				m_PendingCmd.strSrcLayer = Pair.first;
				break;
			}
		}
	}

	ImGui::End();

	// 이번 프레임의 mutation 을 ImGui Render 종료 후 Flush
	Flush_PendingCommand(static_cast<_uint>(iLevelIndex));
}

void CPanel_Hierarchy::Flush_PendingCommand(_uint iLevel)
{
	if (m_PendingCmd.eType == CMD_TYPE::NONE)
		return;

	switch (m_PendingCmd.eType)
	{
	case CMD_TYPE::MOVE:
		// 1) Move (끝에 append 됨)
		if (SUCCEEDED(m_pGameInstance->Move_GameObject(
			iLevel, m_PendingCmd.strSrcLayer,
			iLevel, m_PendingCmd.strDstLayer,
			m_PendingCmd.pObject)))
		{
			// 2) 인덱스가 지정됐다면 해당 위치로 Reorder
			if (m_PendingCmd.iInsertIndex != UINT_MAX)
			{
				m_pGameInstance->Reorder_GameObject(
					iLevel, m_PendingCmd.strDstLayer,
					m_PendingCmd.pObject, m_PendingCmd.iInsertIndex);
			}
		}
		break;

	case CMD_TYPE::SPAWN:
	{
		HRESULT hr = m_pGameInstance->Add_GameObject(
			m_PendingCmd.iSpawnProtoLevel,
			m_PendingCmd.strSpawnProtoTag,
			iLevel,
			m_PendingCmd.strDstLayer,
			nullptr);

		if (FAILED(hr))
		{
			Log_Error(WTOA(L"[Hierarchy] Spawn failed: " + m_PendingCmd.strSpawnProtoTag));
		}
		else
		{
			Log_Info(WTOA(L"[Hierarchy] Spawned: " + m_PendingCmd.strSpawnProtoTag));
		}
	}
	break;

	case CMD_TYPE::REORDER:
		m_pGameInstance->Reorder_GameObject(
			iLevel, m_PendingCmd.strSrcLayer,
			m_PendingCmd.pObject, m_PendingCmd.iInsertIndex);
		break;

	case CMD_TYPE::REMOVE:
		if (m_pPanel_Manager->Get_SelectedObject() == m_PendingCmd.pObject)
			m_pPanel_Manager->Clear_Selection();
		m_pGameInstance->Remove_GameObject(
			iLevel, m_PendingCmd.strSrcLayer, m_PendingCmd.pObject);
		break;

	default: break;
	}

	m_PendingCmd = PENDING_CMD{};
}

CPanel_Hierarchy* CPanel_Hierarchy::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Hierarchy* pInstance = new CPanel_Hierarchy(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Hierarchy");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Hierarchy::Free()
{
	__super::Free();
}
