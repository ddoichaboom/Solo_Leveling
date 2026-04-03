#include "Editor_Defines.h"

// Engine Headers
#include "Transform.h"
#include "Transform_3D.h"
#include "Transform_2D.h"
#include "GameObject.h"
#include "Camera.h"

// RTTR
#include <rttr/registration.h>

using namespace Engine;

// RTTR 등록
// RTTR_REGISTRATION 매크로는 전역 정적 변수를 생성하여 main() 전에 자동 실행된다.
// 이 파일이 Editor.exe에 링크되는 순간 등록이 완료된다.

RTTR_REGISTRATION
{
	rttr::registration::class_<_float3>("_float3")
		.property("x", &_float3::x)
		.property("y", &_float3::y)
		.property("z", &_float3::z);

// CTransform (기본 클래스)
	rttr::registration::class_<CTransform>("CTransform")
		.property("Position",
			&CTransform::Get_Position,
			&CTransform::Set_Position)
		.property("Scale",
			&CTransform::Get_Scale,
			static_cast<void(CTransform::*)(_float3)>(&CTransform::Set_Scale))
		.property("Rotation",
			&CTransform::Get_Rotation,
			&CTransform::Set_Rotation);

	// CGameObject
	rttr::registration::class_<CGameObject>("CGameObject")
		.property("Name", &CGameObject::Get_Name, &CGameObject::Set_Name)
		.property("Tag", &CGameObject::Get_Tag, &CGameObject::Set_Tag);
}