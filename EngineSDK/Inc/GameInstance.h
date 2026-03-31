#pragma once

#include "Prototype_Manager.h"

NS_BEGIN(Engine)

class ENGINE_DLL CGameInstance final : public CBase
{
	DECLARE_SINGLETON(CGameInstance)

private:
	CGameInstance();
	virtual ~CGameInstance() = default;

#pragma region ENGINE
public:
	HRESULT						Initialize_Engine(const ENGINE_DESC& EngineDesc,
													ID3D11Device** ppDevice,
													ID3D11DeviceContext** ppContext);
	void						Update_Engine(_float fTimeDelta);
	HRESULT						Begin_Draw();
	HRESULT						Draw();
	HRESULT						End_Draw();
	void 						Clear_Resources(_int iLevelIndex);

	_float						Random(_float fMin, _float fMax);
#pragma endregion

#pragma region TIMER_MANAGER
public:
	_float						Get_TimeDelta(const _wstring& strTimerTag);
	HRESULT						Add_Timer(const _wstring& strTimerTag);
	void						Compute_Timer(const _wstring& strTimerTag);
#pragma endregion

#pragma region LEVEL_MANAGER
	HRESULT 					Change_Level(_int iNewLevelIndex, class CLevel* pNewLevel);
#pragma endregion

#pragma region PROTOTYPE_MANAGER
	HRESULT						Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype);
	CBase*						Clone_Prototype(PROTOTYPE eType, _uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg = nullptr); 
#pragma endregion

#pragma region OBJECT_MANAGER
	HRESULT						Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag,
												_uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg = nullptr);
#pragma endregion

#pragma region RENDERER
	void						Add_RenderGroup(RENDERID eGroupID, class CGameObject* pGameObject);
#pragma endregion

#pragma region PIPELINE
	const _float4x4*			Get_Transform(D3DTS eState) const;
	const _float4x4*			Get_Transform_Inverse(D3DTS eState) const;
	const _float4*				Get_CamPosition() const;
	void						Set_Transform(D3DTS eState, _fmatrix StateMatrix);
#pragma endregion

#pragma region INPUT_DEVICE
	_byte						Get_KeyState(_ubyte byKeyID);
	_byte						Get_MouseBtnState(MOUSEBTN eBtn);
	_long						Get_MouseDelta(MOUSEAXIS eAxis);
	static void					Process_RawInput(LPARAM lParam);
#pragma endregion

#pragma region LIGHT_MANAGER
	const LIGHT_DESC*			Get_LightDesc(_uint iIndex);
	HRESULT						Add_Light(const LIGHT_DESC& LightDesc);
#pragma endregion

private:
	class CGraphic_Device*		m_pGraphic_Device = { nullptr };
	class CTimer_Manager*		m_pTimer_Manager = { nullptr };
	class CLevel_Manager*		m_pLevel_Manager = { nullptr };
	class CPrototype_Manager*	m_pPrototype_Manager = { nullptr };
	class CObject_Manager*		m_pObject_Manager = { nullptr };
	class CRenderer*			m_pRenderer = { nullptr };
	class CPipeLine*			m_pPipeLine = { nullptr };
	class CInput_Device*		m_pInput_Device = { nullptr };
	class CLight_Manager*		m_pLight_Manager = { nullptr };
	
public:
	void						Release_Engine();
	virtual void				Free() override;
};

NS_END