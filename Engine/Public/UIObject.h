#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIObject abstract : public CGameObject
{
public:
	typedef struct tagUIObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float			fCenterX, fCenterY;
		_float			fSizeX, fSizeY;
		_uint			iZOrder = { 0 };
		const _tchar*	pObjectName = { nullptr };
		_bool			bVisible = { true };

		tagUIObjectDesc() { eTransformType = TRANSFORMTYPE::TRANSFORM_2D; }
	}UIOBJECT_DESC;

protected:
	CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUIObject(const CUIObject& Prototype);
	virtual ~CUIObject() = default;

public:
	const _wstring& Get_ObjectName() const { return m_strObjectName; }

	_bool					Is_Visible() const { return m_bVisible; }
	void					Set_Visible(_bool bVisible) { m_bVisible = bVisible; }
	_bool					Is_Hovered(_float fScreenX, _float fScreenY) const;
	
	_uint					Get_ZOrder() const { return m_iZOrder; }
	void					Set_ZOrder(_uint iZOrder) { m_iZOrder = iZOrder; }

public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);
	virtual void			Priority_Update(_float fTimeDelta);
	virtual void			Update(_float fTimeDelta);
	virtual void			Late_Update(_float fTimeDelta);
	virtual HRESULT			Render();

protected:
	_float					m_fCenterX{}, m_fCenterY{}, m_fSizeX{}, m_fSizeY{};
	_float					m_fViewWidth{}, m_fViewHeight{};
	_float4x4				m_TransformMatrices[ETOUI(D3DTS::END)] = {};
	_wstring				m_strObjectName;
	_bool					m_bVisible = { true };

	_uint					m_iZOrder = { 0 };

protected:
	void					Update_UIState();
	HRESULT					Bind_ShaderResource(class CShader* pShader, const _char* pContstantName, D3DTS eType);

public:
	virtual CGameObject*	Clone(void* pArg) PURE;
	virtual void			Free() override;

};

NS_END
