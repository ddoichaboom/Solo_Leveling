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
		const _tchar* pTextureProtoTag = { nullptr };
		_uint iTextureProtoLevel = { ETOUI(LEVEL::STATIC) };

		const _tchar*	pTexturePath = { nullptr };
		_uint			iZOrder = { 0 };
	}UI_IMAGE_DESC;

protected:
	CUI_Image(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUI_Image(const CUI_Image& Prototype);
	virtual ~CUI_Image() = default;

public:
	void						Set_Progress(_float fProgress);

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

protected:
	CShader*					m_pShaderCom = { nullptr };
	CVIBuffer*					m_pVIBufferCom = { nullptr };
	CTexture*					m_pTextureCom = { nullptr };
	_uint						m_iZOrder = { 0 };
	_float						m_fBaseCenterX = { 0.f };
	_float						m_fBaseSizeX = { 0.f };
	_bool						m_bBaseCached = { false };
	
protected:
	HRESULT						Ready_Components(const UI_IMAGE_DESC* pDesc);
	HRESULT						Bind_ShaderResources();

public:
	static	CUI_Image*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END