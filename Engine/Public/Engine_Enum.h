#ifndef Engine_Enum_h__
#define Engine_Enum_h__

namespace Engine
{
	enum class WINMODE { FULL, WIN, END };

	enum class PROTOTYPE { GAMEOBJECT, COMPONENT, END };

	enum class TRANSFORMTYPE { TRANSFORM_2D, TRANSFORM_3D, END };

	enum class RENDERID { PRIORITY, NONBLEND, BLEND, UI, END };

	enum class STATE { RIGHT, UP, LOOK, POSITION, END };

	enum class D3DTS { VIEW, PROJ, END };

	enum class MOUSEBTN { LBUTTON, RBUTTON, MBUTTON, END };

	enum class MOUSEAXIS { X, Y, WHEEL, END };

	enum class LIGHT { DIRECTIONAL, POINT, END };

	enum class MODEL { NONANIM, ANIM, END };

    enum class TEXTURE_TYPE
    {
        DIFFUSE,
        SPECULAR,
        AMBIENT,
        EMISSIVE,
        HEIGHT,
        NORMAL,
        SHININESS,
        OPACITY,
        DISPLACEMENT,
        LIGHTMAP,
        REFLECTION,
        BASE_COLOR,
        NORMAL_CAMERA,
        EMISSION_COLOR,
        METALNESS,
        DIFFUSE_ROUGHNESS,
        AMBIENT_OCCLUSION,

        UNKNOWN,
        END
    };

    enum class NOTIFY_TYPE : unsigned int
    {
        ACTION_FINISHED,
        ANIM_EVENT,

        // 향후 확장

        END
    };

	//// Dynamic 컴포넌트 경우 매 프레임마다 갱신해야하는 컴포넌트 집단
	//enum COMPONENTID { ID_DYNAMIC, ID_STATIC, ID_END };

	//enum INFO {	INFO_RIGHT, INFO_UP, INFO_LOOK, INFO_POS, INFO_END };

	//enum ROTATION { ROT_X, ROT_Y, ROT_Z, ROT_END };

	//enum TEXTUREID { TEX_NORMAL, TEX_CUBE, TEX_END };

	//enum RENDERID { RENDER_PRIORITY, RENDER_NONALPHA, RENDER_ALPHA, RENDER_UI, RENDER_END };

}
#endif // Engine_Enum_h__
