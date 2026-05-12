#pragma once

#include "Client_Defines.h"
#include "UI_Image.h"

NS_BEGIN(Client)

class CLIENT_DLL CUI_SpriteAnim : public CUI_Image
{
public:
    typedef struct tagUI_SpriteAnimDesc : public CUI_Image::UI_IMAGE_DESC
    {
        _uint   iAtlasCols = { 1 };
        _uint   iAtlasRows = { 1 };
        _float  fFrameDuration = { 0.1f };     // 한 프레임 표시 시간(초)
        _bool   bLoop = { true };
    }UI_SPRITEANIM_DESC;

protected:
    CUI_SpriteAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_SpriteAnim(const CUI_SpriteAnim& Prototype);
    virtual ~CUI_SpriteAnim() = default;

public:
    virtual HRESULT     Initialize_Prototype() override;
    virtual HRESULT     Initialize(void* pArg) override;
    virtual void        Update(_float fTimeDelta) override;
    virtual HRESULT     Render() override;

public:
    void                Set_Frame(_uint iFrame) 
    { 
        m_iCurFrame = iFrame % max(1u, m_iAtlasCols * m_iAtlasRows); 
        m_fFrameTimer = 0.f; 
    }

    _uint               Get_Frame() const { return m_iCurFrame; }

private:
    _uint       m_iAtlasCols = { 1 };
    _uint       m_iAtlasRows = { 1 };
    _float      m_fFrameDuration = { 0.1f };
    _bool       m_bLoop = { true };

    _uint       m_iCurFrame = { 0 };
    _float      m_fFrameTimer = { 0.f };
    _bool       m_bFinished = { false };

private:
    HRESULT     Bind_SpriteAnim_Resources();
    void        Compute_UVOffsetScale(_float4* pOut) const;

public:
    static CUI_SpriteAnim* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void            Free() override;
};

NS_END

