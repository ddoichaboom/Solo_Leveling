#include "Loader.h"

// ENGINE
#include "GameInstance.h"
#include "Model.h"

// LOGO
#include "BackGround.h"

// GAMEOBJECT
#include "Camera_Free.h"
#include "Player.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "MapObject.h"
#include "MapStaticObject.h"
#include "Camera_Follow.h"


CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	if (FAILED(pLoader->Loading()))
		return -1;

	return 0;
}

HRESULT CLoader::Initialize(LEVEL eNextLevelID)
{
	m_eNextLevelID = eNextLevelID;
	InitializeCriticalSection(&m_CriticalSection);

	// eNextLevelID 에 필요한 자원을 로딩하는 작업을 수행한다.
	// 누가? 스레드가 수행한다.
	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr));
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}

HRESULT CLoader::Loading()
{
	EnterCriticalSection(&m_CriticalSection);

	CoInitializeEx(nullptr, 0);

	HRESULT			hr = {};

	switch (m_eNextLevelID)
	{
	case LEVEL::LOGO:
		hr = Ready_Resources_For_Logo();
		break;
	case LEVEL::GAMEPLAY:
		hr = Ready_Resources_For_GamePlay();
		break;
	}

	CoUninitialize();

	LeaveCriticalSection(&m_CriticalSection);

	if (FAILED(hr))
		return E_FAIL;

	return S_OK;
}

#ifdef _DEBUG
void CLoader::Show()
{
	SetWindowText(m_pGameInstance->Get_hWnd(), m_szLoadingText);
}
#endif

HRESULT CLoader::Ready_Resources_For_Logo()
{
	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중"));

	// Prototype_Component_Texture_BackGround
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_Component_Texture_BackGround"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../Resources/Textures/Default%d.jpg"), 2))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("셰이더 로딩 중"));


	lstrcpy(m_szLoadingText, TEXT("정점, 인덱스 버퍼 로딩 중"));

	lstrcpy(m_szLoadingText, TEXT("객체원형 로딩 중"));

	// Prototype_GameObject_BackGround
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_GameObject_BackGround"), CBackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));

	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Ready_Resources_For_GamePlay()
{
	const LEVEL eLevel = LEVEL::GAMEPLAY;

	lstrcpy(m_szLoadingText, TEXT("셰이더 로드 중"));

	// Prototype_Component_Shader_VtxMesh
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Shader_VtxMesh"),
		CShader::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/ShaderFiles/Shader_VtxMesh.hlsl"),
			VTXMESH::Elements, VTXMESH::iNumElements))))
		return E_FAIL;

	// Prototype_Component_Shader_VtxAnimMesh 
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		CShader::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/ShaderFiles/Shader_VtxAnimMesh.hlsl"),
			VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("모델 로드 중"));

	// Prototype_Component_Model_SungJinWoo
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Model_SungJinWoo_OverDrive"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/hunter/SungJinWoo/Without_CamAnim/SungJinWoo_OverDrive.bin")))))
		return E_FAIL;

	// Prototype_Component_Model_Weapon_KnightKiller
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Model_Weapon_KnightKiller"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/weapons/KnightKiller/Weapon_Dualwield (merge).bin")))))
		return E_FAIL;

	// Prototype_Component_Model_Weapon_KasakaVenomFang
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Model_Weapon_KasakaVenomFang"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/weapons/KasakaVenomFang/Default/Weapon_Dualwield_02.bin")))))
		return E_FAIL;

	// Prototype_Component_Model_Map_ThroneRoom
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_Component_Model_Map_ThroneRoom"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/map/ThroneRoom/ThroneRoom.bin")))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("객체원형 로드 중"));

	// Prototype_GameObject_Camera_Follow
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_Camera_Follow"),
		CCamera_Follow::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_Camera_Free 
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_Camera_Free"),
		CCamera_Free::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_Player
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_Player"),
		CPlayer::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_Body_Player
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_Body_Player"),
		CBody_Player::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_Weapon
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_Weapon"),
		CWeapon::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_MapObject
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_MapObject"),
		CMapObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_MapStaticObject
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_GameObject_MapStaticObject"),
		CMapStaticObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));

	m_isFinished = true;

	return S_OK;
}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLoader* pInstance = new CLoader(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX("Failed to Created : CLoader");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLoader::Free()
{
	__super::Free();

	WaitForSingleObject(m_hThread, INFINITE);

	DeleteCriticalSection(&m_CriticalSection);

	CloseHandle(m_hThread);

	Safe_Release(m_pGameInstance);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}