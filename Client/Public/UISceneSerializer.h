#pragma once
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CBinaryReader;
class CBinaryWriter;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CUISceneSerializer final
{
public:
	static HRESULT Save(const _tchar* pUISceneDataPath, const UI_SCENE_DATA& UISceneData);
	static HRESULT Load(const _tchar* pUISceneDataPath, UI_SCENE_DATA* pOutUISceneData);

private:
	static HRESULT Write_Element(CBinaryWriter& Writer, const UI_ELEMENT& Element);
	static HRESULT Read_Element(CBinaryReader& Reader, UI_ELEMENT* pOutElement, _uint iVersion);
};

NS_END