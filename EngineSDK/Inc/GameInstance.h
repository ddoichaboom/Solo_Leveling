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

	HWND						Get_hWnd() const { return m_hWnd; }
	_uint						Get_WinSizeX() const { return m_iWinSizeX; }
	_uint						Get_WinSizeY() const { return m_iWinSizeY; }

	HRESULT						OnResize(_uint iWinSizeX, _uint iWinSizeY);
	WINMODE						Get_WinMode() const { return m_eWinMode; }
	void						Set_WinMode(WINMODE eMode) { m_eWinMode = eMode; }

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

private:
	class CGraphic_Device*		m_pGraphic_Device = { nullptr };
	class CTimer_Manager*		m_pTimer_Manager = { nullptr };
	class CLevel_Manager*		m_pLevel_Manager = { nullptr };
	class CPrototype_Manager*	m_pPrototype_Manager = { nullptr };
	class CObject_Manager*		m_pObject_Manager = { nullptr };
	class CRenderer*			m_pRenderer = { nullptr };

private:
	HWND						m_hWnd;
	_uint						m_iWinSizeX;
	_uint						m_iWinSizeY;
	WINMODE						m_eWinMode = { WINMODE::WIN };
	
public:
	void						Release_Engine();
	virtual void				Free() override;
};

NS_END