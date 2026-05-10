#include "Level_GamePlay.h"
#include "GameInstance.h"
#include "Camera_Follow.h"
#include "Player.h"
#include "Layer.h"
#include "NavMeshObject.h"
#include "NavMesh.h"
#include "Cell.h"
#include "SceneSerializer.h"

namespace
{
	static constexpr _int PLAYER_START_CELL_INDEX = { 40 };

	static const _tchar* SCENEDATA_PATH = TEXT("../../Resources/Scenes/ThroneRoom.scene");
	static const _tchar* DEFAULT_NAVDATA_PATH = TEXT("../../Resources/NavMesh/ThroneRoom.navdata");

	_bool Apply_PlayerSpawnFromCell(CPlayer::PLAYER_DESC& Desc, CNavMesh* pNavMesh, _int iCellIndex)
	{
		if (nullptr == pNavMesh)
			return false;

		const CCell* pStartCell = pNavMesh->Get_Cell(iCellIndex);
		if (nullptr == pStartCell)
			return false;

		Desc.vPosition = pStartCell->Get_Center();
		Desc.vPosition.y = pNavMesh->Compute_Height(iCellIndex, Desc.vPosition);
		Desc.iStartCellIndex = iCellIndex;

		return true;
	}

	_bool Apply_PlayerSpawnPoint(CPlayer::PLAYER_DESC& Desc, CNavMesh* pNavMesh, const SPAWN_POINT* pSpawnPoint)
	{
		if (nullptr == pSpawnPoint)
			return false;

		Desc.vPosition = pSpawnPoint->vPosition;
		Desc.vRotationDeg = pSpawnPoint->vRotationDeg;
		Desc.iStartCellIndex = pSpawnPoint->iNavCellIndex;

		if (nullptr != pNavMesh)
		{
			if (NAVMESH_INVALID_INDEX == Desc.iStartCellIndex)
				Desc.iStartCellIndex = pNavMesh->Find_Cell(Desc.vPosition);

			if (NAVMESH_INVALID_INDEX == Desc.iStartCellIndex)
				return false;

			Desc.vPosition.y = pNavMesh->Compute_Height(Desc.iStartCellIndex, Desc.vPosition);
		}

		return true;
	}

	CNavMesh* Find_GamePlayNavMesh()
	{
		CGameInstance* pGameInstance = CGameInstance::GetInstance();
		if (nullptr == pGameInstance)
			return nullptr;

		const auto* pLayers = pGameInstance->Get_Layers(ETOUI(LEVEL::GAMEPLAY));
		if (nullptr == pLayers)
			return nullptr;

		auto iterLayer = pLayers->find(TEXT("Layer_NavMesh"));
		if (iterLayer == pLayers->end() || nullptr == iterLayer->second)
			return nullptr;

		const list<CGameObject*>& NavMeshObjects = iterLayer->second->Get_GameObjects();

		for (CGameObject* pObject : NavMeshObjects)
		{
			CNavMeshObject* pNavMeshObject = dynamic_cast<CNavMeshObject*>(pObject);
			if (nullptr == pNavMeshObject)
				continue;

			CNavMesh* pNavMesh = pNavMeshObject->Get_NavMesh();
			if (nullptr != pNavMesh)
				return pNavMesh;
		}

		return nullptr;
	}
}

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CLevel{ pDevice, pContext }
{

}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(Ready_SceneData()))
		return E_FAIL;

	if (FAILED(Ready_Lights()))
		return E_FAIL;

	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_NavMesh(TEXT("Layer_NavMesh"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
		return E_FAIL;

	m_pGameInstance->Set_CursorLocked(true);

	return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{

}

HRESULT CLevel_GamePlay::Render()
{
#ifdef _DEBUG
	SetWindowText(m_pGameInstance->Get_hWnd(), TEXT("게임 플레이 레벨 입니다."));
#endif

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_SceneData()
{
	m_SceneData = SCENE_DATA{};
	m_bSceneDataLoaded = false;

	if (SUCCEEDED(CSceneSerializer::Load(SCENEDATA_PATH, &m_SceneData)))
		m_bSceneDataLoaded = true;

	if (0 == m_SceneData.szNavDataPath[0])
		wcscpy_s(m_SceneData.szNavDataPath, DEFAULT_NAVDATA_PATH);

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Lights()
{
	LIGHT_DESC		LightDesc{};

	LightDesc.eType = LIGHT::DIRECTIONAL;
	LightDesc.vDiffuse = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vAmbient = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);

	if (FAILED(m_pGameInstance->Add_Light(LightDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _wstring& strLayerTag)
{
	const _float4x4* pTargetWorld = { nullptr };

	const auto* pLayers = m_pGameInstance->Get_Layers(ETOUI(LEVEL::GAMEPLAY));

	if (nullptr != pLayers)
	{
		auto iter = pLayers->find(TEXT("Layer_Player"));
		if (iter != pLayers->end() && nullptr != iter->second)
		{
			const list<CGameObject*>& PlayerObjects = iter->second->Get_GameObjects();
			if (false == PlayerObjects.empty())
			{
				CGameObject* pPlayer = PlayerObjects.front();
				if (nullptr != pPlayer && nullptr != pPlayer->Get_Transform())
					pTargetWorld = pPlayer->Get_Transform()->Get_WorldMatrixPtr();
			}
		}
	}

	CCamera_Follow::CAMERA_FOLLOW_DESC  CameraDesc{};
	CameraDesc.vEye = _float3(0.f, 3.f, -5.f);
	CameraDesc.vAt = _float3(0.f, 1.5f, 0.f);
	CameraDesc.fFovy = XMConvertToRadians(60.f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;
	CameraDesc.strTargetLayerTag = TEXT("Layer_Player");
	CameraDesc.pTargetWorldMatrix = pTargetWorld;
	CameraDesc.vHeightOffset = { 0.f, 1.5f, 0.f };
	CameraDesc.fIdealDistance = 5.f;
	CameraDesc.fInitialYaw = 0.f;
	CameraDesc.fInitialPitch = -0.3f;
	CameraDesc.fPitchMin = -1.1f;
	CameraDesc.fPitchMax = 1.0f;
	CameraDesc.fMouseSensor = 0.003f;
	CameraDesc.fArmLerpSpeed = 8.f;

	if (FAILED(m_pGameInstance->Add_GameObject(
		ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Follow"),
		ETOUI(LEVEL::GAMEPLAY), strLayerTag, &CameraDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
	// Prototype_GameObject_MapObject
	if (FAILED(m_pGameInstance->Add_GameObject(
		ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapObject"),
		ETOUI(LEVEL::GAMEPLAY), strLayerTag)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_NavMesh(const _wstring& strLayerTag)
{
	NAVMESH_SNAPSHOT Snapshot{};

	if (FAILED(CNavMesh::Load_NavDataSnapshot(
		m_SceneData.szNavDataPath,
		&Snapshot)))
		return E_FAIL;

	CNavMeshObject::NAVMESHOBJECT_DESC Desc{};
	Desc.pInitialSnapshot = &Snapshot;

	if (FAILED(m_pGameInstance->Add_GameObject(
		ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_NavMeshObject"),
		ETOUI(LEVEL::GAMEPLAY), strLayerTag, &Desc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Monster(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(
		ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster"),
		ETOUI(LEVEL::GAMEPLAY), strLayerTag)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _wstring& strLayerTag)
{
	CPlayer::PLAYER_DESC Desc{};

	CNavMesh* pNavMesh = Find_GamePlayNavMesh();

	Desc.pNavMesh = pNavMesh;
	Desc.iStartCellIndex = NAVMESH_INVALID_INDEX;

	Desc.vPosition = _float3(0.f, 1.f, 0.f);
	Desc.vRotationDeg = _float3(0.f, 0.f, 0.f);
	Desc.vScale = _float3(1.f, 1.f, 1.f);
	Desc.fSpeedPerSec = 5.f;
	Desc.fRotationPerSec = XMConvertToRadians(1440.f);

	const SPAWN_POINT* pPlayerSpawnPoint = nullptr;

	if (m_bSceneDataLoaded)
		pPlayerSpawnPoint = CSceneSerializer::Find_FirstSpawnPoint(m_SceneData, SPAWN_TYPE::PLAYER);

	if (false == Apply_PlayerSpawnPoint(Desc, pNavMesh, pPlayerSpawnPoint))
		Apply_PlayerSpawnFromCell(Desc, pNavMesh, PLAYER_START_CELL_INDEX);

	if (FAILED(m_pGameInstance->Add_GameObject(
		ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
		ETOUI(LEVEL::GAMEPLAY), strLayerTag, &Desc)))
		return E_FAIL;

	return S_OK;
}

CLevel_GamePlay* CLevel_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_GamePlay* pInstance = new CLevel_GamePlay(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_GamePlay");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLevel_GamePlay::Free()
{
	m_pGameInstance->Set_CursorLocked(false);

	__super::Free();
}