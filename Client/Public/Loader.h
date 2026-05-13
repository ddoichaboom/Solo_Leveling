#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CLoader final : public CBase
{
private:
	CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLoader() = default;

public:
	HRESULT					Initialize(LEVEL eNextLevelID);
	HRESULT					Loading();
	_bool					isFinished() const { return m_isFinished; }

public:
	_float					Get_Progress() const { return m_fProgress; }
	const _tchar*			Get_LoadingText() const { return m_szLoadingText; }

#ifdef _DEBUG
public:
	void					Show();
#endif

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };
	LEVEL					m_eNextLevelID = { LEVEL::END };

	HANDLE					m_hThread = { };
	CRITICAL_SECTION		m_CriticalSection = { };

private:
	_tchar					m_szLoadingText[MAX_PATH] = {};
	_bool					m_isFinished = {};
	_float					m_fProgress = { 0.f };
private:
	HRESULT					Ready_Resources_For_Logo();
	HRESULT					Ready_Resources_For_GamePlay();

public:
	static CLoader*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID);
	virtual void			Free() override;
};

NS_END
