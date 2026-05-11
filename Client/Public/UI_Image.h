#pragma once
#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Engine)
class CShader;
class CVIBuffer;
class CTexture;
NS_END

NS_BEGIN(Client)

class CUI_Image : public CUIObject
{
public:
	typedef struct tagUI_ImageDesc : public CUIObject::UIOBJECT_DESC
	{
		const _tchar*	pTexturePath = { nullptr };
		_uint			iZOrder = { 0 };
	}UI_IMAGE_DESC;

private:
	CUI_Image(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUI_Image(const CUI_Image& Prototype);
	virtual ~CUI_Image() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

private:
	CShader*					m_pShaderCom = { nullptr };
	CVIBuffer*					m_pVIBufferCom = { nullptr };
	CTexture*					m_pTextureCom = { nullptr };
	_uint						m_iZOrder = { 0 };
	
private:
	HRESULT						Ready_Components(const _tchar* pTexturePath);
	HRESULT						Bind_ShaderResources();

public:
	static	CUI_Image*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END