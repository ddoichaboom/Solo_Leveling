#include "GameInstance.h"
#include "Graphic_Device.h"
#include "Timer_Manager.h"
#include "Level_Manager.h"
#include "Object_Manager.h"
#include "Renderer.h"
#include "PipeLine.h"
#include "Input_Device.h"
#include "Light_Manager.h"

IMPLEMENT_SINGLETON(CGameInstance)

CGameInstance::CGameInstance()
{
}

#pragma region ENGINE

HRESULT CGameInstance::Initialize_Engine(const ENGINE_DESC& EngineDesc, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	m_pGraphic_Device = CGraphic_Device::Create(EngineDesc.hWnd, EngineDesc.eWinMode, EngineDesc.iViewportWidth, EngineDesc.iViewportHeight, ppDevice, ppContext);
	if (nullptr == m_pGraphic_Device)
		return E_FAIL;

	m_pTimer_Manager = CTimer_Manager::Create();
	if (nullptr == m_pTimer_Manager)
		return E_FAIL;

	m_pLevel_Manager = CLevel_Manager::Create();
	if (nullptr == m_pLevel_Manager)
		return E_FAIL;

	m_pPrototype_Manager = CPrototype_Manager::Create(EngineDesc.iNumLevels);
	if (nullptr == m_pPrototype_Manager)
		return E_FAIL;

	m_pObject_Manager = CObject_Manager::Create(EngineDesc.iNumLevels);
	if (nullptr == m_pObject_Manager)
		return E_FAIL;

	m_pRenderer = CRenderer::Create(*ppDevice, *ppContext);
	if (nullptr == m_pRenderer)
		return E_FAIL;

	m_pPipeLine = CPipeLine::Create();
	if (nullptr == m_pPipeLine)
		return E_FAIL;

	m_pInput_Device = CInput_Device::Create(EngineDesc.hWnd);
	if (nullptr == m_pInput_Device)
		return E_FAIL;

	m_pLight_Manager = CLight_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pLight_Manager)
		return E_FAIL;

	return S_OK;
}

void CGameInstance::Update_Engine(_float fTimeDelta)
{
	m_pInput_Device->Update();							// (1) 누적 Raw Input -> 프레임 데이터 복사

	m_pObject_Manager->Priority_Update(fTimeDelta);		// (2) 카메라 이동 처리
	m_pObject_Manager->Update(fTimeDelta);				// (3) 카메라 -> PipeLine 세팅

	m_pPipeLine->Update();								// (4) 역행렬 계산, 카메라 위치 추출

	m_pObject_Manager->Late_Update(fTimeDelta);			// (5) 렌더 등록

	m_pLevel_Manager->Update(fTimeDelta);
}

HRESULT CGameInstance::Begin_Draw()
{
	// 색상 : 파란색 설정
	_float4     vColor = _float4(0.f, 0.f, 1.f, 1.f);

	if (FAILED(m_pGraphic_Device->Clear_BackBuffer_View(&vColor)))
		return E_FAIL;

	if (FAILED(m_pGraphic_Device->Clear_DepthStencil_View()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGameInstance::Draw()
{
	if (FAILED(m_pRenderer->Draw()))
		return E_FAIL;

	if (FAILED(m_pLevel_Manager->Render()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGameInstance::End_Draw()
{
	return m_pGraphic_Device->Present();
}

void CGameInstance::Clear_Resources(_int iLevelIndex)
{
	if (-1 == iLevelIndex)
		return;

	/*iLevelIndex용 자원을 정리한다. */
	m_pObject_Manager->Clear(iLevelIndex);

	m_pPrototype_Manager->Clear(iLevelIndex);
}

HRESULT CGameInstance::OnResize(_uint iWinSizeX, _uint iWinSizeY)
{
	if (nullptr == m_pGraphic_Device)
		return S_OK;

	// 동일 크기면 스킵
	if (m_iWinSizeX == iWinSizeX && m_iWinSizeY == iWinSizeY)
		return S_OK;

	m_iWinSizeX = iWinSizeX;
	m_iWinSizeY = iWinSizeY;

	return m_pGraphic_Device->OnResize(iWinSizeX, iWinSizeY);
}

_float CGameInstance::Random(_float fMin, _float fMax)
{
	return fMin + static_cast<_float>(rand()) / RAND_MAX * (fMax - fMin);
}

#pragma endregion

#pragma region TIMER_MANAGER

_float CGameInstance::Get_TimeDelta(const _wstring& strTimerTag)
{
	return m_pTimer_Manager->Get_TimeDelta(strTimerTag);
}

HRESULT CGameInstance::Add_Timer(const _wstring& strTimerTag)
{
	return m_pTimer_Manager->Add_Timer(strTimerTag);
}

void CGameInstance::Compute_Timer(const _wstring& strTimerTag)
{
	m_pTimer_Manager->Compute_Timer(strTimerTag);
}

#pragma endregion

#pragma region LEVEL_MANAGER

HRESULT CGameInstance::Change_Level(_int iNewLevelIndex, CLevel* pNewLevel)
{
	return m_pLevel_Manager->Change_Level(iNewLevelIndex, pNewLevel);
}

#pragma endregion

#pragma region PROTOTYPE_MANAGER

HRESULT	CGameInstance::Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype)
{
	return m_pPrototype_Manager->Add_Prototype(iLevelIndex, strPrototypeTag, pPrototype);
}

CBase* CGameInstance::Clone_Prototype(PROTOTYPE eType, _uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
	return m_pPrototype_Manager->Clone_Prototype(eType, iLevelIndex, strPrototypeTag, pArg);
}

#pragma endregion

#pragma region OBJECT_MANAGER

HRESULT	CGameInstance::Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag,
	_uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg)
{
	return m_pObject_Manager->Add_GameObject(iPrototypeLevelIndex, strPrototypeTag, iLayerLevelIndex, strLayerTag, pArg);
}

#pragma endregion

#pragma region RENDERER

void CGameInstance::Add_RenderGroup(RENDERID eGroupID, class CGameObject* pGameObject)
{
	m_pRenderer->Add_RenderGroup(eGroupID, pGameObject);
}

#pragma endregion

#pragma region PIPELINE
const _float4x4* CGameInstance::Get_Transform(D3DTS eState) const
{
	return m_pPipeLine->Get_Transform(eState);
}

const _float4x4* CGameInstance::Get_Transform_Inverse(D3DTS eState) const
{
	return m_pPipeLine->Get_Transform_Inverse(eState);
}

const _float4* CGameInstance::Get_CamPosition() const
{
	return m_pPipeLine->Get_CamPosition();
}

void			CGameInstance::Set_Transform(D3DTS eState, _fmatrix StateMatrix)
{
	m_pPipeLine->Set_Transform(eState, StateMatrix);
}

#pragma endregion

#pragma region INPUT_DEVICE
_byte			CGameInstance::Get_KeyState(_ubyte byKeyID)
{
	return m_pInput_Device->Get_KeyState(byKeyID);
}

_byte			CGameInstance::Get_MouseBtnState(MOUSEBTN eBtn)
{
	return m_pInput_Device->Get_MouseBtnState(eBtn);
}

_long			CGameInstance::Get_MouseDelta(MOUSEAXIS eAxis)
{
	return m_pInput_Device->Get_MouseDelta(eAxis);
}

void		CGameInstance::Process_RawInput(LPARAM lParam)
{
	CInput_Device::Process_Input(lParam);
}

#pragma endregion

#pragma region LIGHT_MANAGER
const LIGHT_DESC* CGameInstance::Get_LightDesc(_uint iIndex)
{
	return m_pLight_Manager->Get_LightDesc(iIndex);
}

HRESULT	CGameInstance::Add_Light(const LIGHT_DESC& LightDesc)
{
	return m_pLight_Manager->Add_Light(LightDesc);
}

#pragma endregion

void CGameInstance::Release_Engine()
{
	Safe_Release(m_pLight_Manager);
	Safe_Release(m_pInput_Device);
	Safe_Release(m_pPipeLine);
	Safe_Release(m_pRenderer);
	Safe_Release(m_pObject_Manager);
	Safe_Release(m_pPrototype_Manager);
	Safe_Release(m_pLevel_Manager);
	Safe_Release(m_pTimer_Manager);
	Safe_Release(m_pGraphic_Device);

	DestroyInstance();
}

void CGameInstance::Free()
{
	__super::Free();
}