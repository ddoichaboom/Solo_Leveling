#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Editor)

class CPanel_Hierarchy final : public CPanel
{
private:
    CPanel_Hierarchy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Hierarchy() = default;

public:
    virtual HRESULT             Initialize() override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Render() override;

private:    
    enum class                  CMD_TYPE { NONE, MOVE, REORDER, REMOVE, SPAWN, END };

    struct PENDING_CMD
    {
        CMD_TYPE            eType = { CMD_TYPE::NONE };
        CGameObject*        pObject = { nullptr };
        _wstring            strSrcLayer;
        _wstring            strDstLayer;
        _uint               iInsertIndex = { 0 };

        // SPAWN Àü¿ë
        _uint               iSpawnProtoLevel = { 0 };
        _wstring            strSpawnProtoTag;
    };

    _int                        m_iPrevLevelIndex = { -1 };
    PENDING_CMD                 m_PendingCmd;

    void                        Flush_PendingCommand(_uint iLevel);

public:
    static CPanel_Hierarchy*    Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                Free() override;

};

NS_END