#pragma once
#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CAnimController final : public CComponent
{
public:
	typedef struct tagAnimClipDesc
	{
		const _char*	pAnimationName	= { nullptr };
		_bool			bRestartOnEnter = { true };
	}ANIM_CLIP_DESC;

private:
	typedef struct tagAnimClip
	{
		_uint			iAnimationIndex = {};
		_bool			bRestartOnEnter = { true };
	}ANIM_CLIP;

private:
	CAnimController(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CAnimController(const CAnimController& Prototype);
	virtual ~CAnimController() = default;

public:
	HRESULT							Bind_Model(class CModel* pModel);

	HRESULT							Register_Clip(_uint64 iKey, const ANIM_CLIP_DESC& Desc);
	HRESULT							Play(_uint64 iKey);
	_bool							Update(_float fTimeDelta);
	void							Restart();

public:
	_bool							Has_CurrentClip() const { return m_bHasCurrentClip; }
	_uint64							Get_CurrentKey() const { return m_iCurrentKey; }
	_bool							Is_Finished() const { return m_bFinished; }

private:
	class CModel*					m_pModel = { nullptr };

	unordered_map<_uint64, ANIM_CLIP> m_Clips;

	_uint64							m_iCurrentKey = {};
	_bool							m_bHasCurrentClip = { false };
	_bool							m_bFinished = { false };

public:
	static CAnimController*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*				Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END