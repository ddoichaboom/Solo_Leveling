#include "FadeOverlay_Helper.h"
#include "GameInstance.h"
#include "UI_Image.h"
#include "Layer.h"

CUI_Image* CFadeOverlay_Helper::Find()
{
    CGameInstance* pInstance = CGameInstance::GetInstance();
    if (nullptr == pInstance)
        return nullptr;

    const auto* pLayers = pInstance->Get_Layers(ETOUI(LEVEL::STATIC));
    if (nullptr == pLayers)
        return nullptr;

    auto it = pLayers->find(TEXT("Layer_Global_UI"));
    if (it == pLayers->end() || nullptr == it->second)
        return nullptr;

    for (CGameObject* pObj : it->second->Get_GameObjects())
    {
        CUIObject* pUI = dynamic_cast<CUIObject*>(pObj);
        if (!pUI) 
            continue;
        if (pUI->Get_ObjectName() == TEXT("FadeOverlay"))
            return dynamic_cast<CUI_Image*>(pUI);
    }

    return nullptr;
}