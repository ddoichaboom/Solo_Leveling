#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_Viewport final : public CPanel
{
private:
    CPanel_Viewport(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Viewport() = default;

public:
    virtual HRESULT             Initialize() override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Render() override;

#pragma region RENDER_TARGET
public:
    HRESULT                     Begin_RT(); 
    HRESULT                     End_RT();

    // RT 접근자
    ID3D11ShaderResourceView*   Get_SRV() const { return m_pSRV; }
    _uint                       Get_RTWidth() const { return m_iRTWidth; }
	_uint                       Get_RTHeight() const { return m_iRTHeight; }

private:
    HRESULT                     Create_RenderTarget(_uint iWidth, _uint iHeight);
	void                        Release_RenderTarget();

private:
    // Render Target 리소스
	ID3D11Texture2D*            m_pRTTexture = { nullptr };
	ID3D11RenderTargetView*     m_pRTV = { nullptr };
	ID3D11ShaderResourceView*   m_pSRV = { nullptr };

	ID3D11Texture2D*            m_pDSTexture = { nullptr };
    ID3D11DepthStencilView*     m_pDSV = { nullptr };

    D3D11_VIEWPORT              m_Viewport = {};

    _uint                       m_iRTWidth = { 0 };
	_uint                       m_iRTHeight = { 0 };

#pragma endregion

public:
    static CPanel_Viewport*     Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                Free() override;

};

NS_END