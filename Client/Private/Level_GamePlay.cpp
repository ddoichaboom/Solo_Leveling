#include "Level_GamePlay.h"
#include "GameInstance.h"
#include "Camera_Follow.h"
#include "Player.h"
#include "Layer.h"

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CLevel{ pDevice, pContext }
{

}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(Ready_Lights()))
		return E_FAIL;

	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
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

	CameraDesc.pTargetWorldMatrix = pTargetWorld;
	CameraDesc.vHeightOffset = { 0.f, 1.5f, 0.f };
	CameraDesc.fIdealDistance = 5.f;
	CameraDesc.fInitialYaw = 0.f;
	CameraDesc.fInitialPitch = -0.3f;
	CameraDesc.fPitchMin = -0.9f;
	CameraDesc.fPitchMax = 0.6f;
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
	Desc.vPosition			= _float3(0.f, 1.f, 0.f);
	Desc.vRotationDeg		= _float3(0.f, 0.f, 0.f);
	Desc.vScale				= _float3(1.f, 1.f, 1.f);
	Desc.fSpeedPerSec		= 10.f;
	Desc.fRotationPerSec	= XMConvertToRadians(180.f);

	// Prototype_GameObject_Player
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