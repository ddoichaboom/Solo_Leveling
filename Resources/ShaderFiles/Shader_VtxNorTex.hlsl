// HLSL 코드 내에서 사용할 전역 변수 선언

// 각종 변환 행렬 (월드, 뷰, 투영)
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

// 카메라의 월드 공간에서의 위치
vector g_vCamPosition;

// 빛
vector g_vLightDir;                 // 빛이 표면으로 들어오는 방향 (방향광)
vector g_vLightDiffuse;             // 빛의 난반사 색상
vector g_vLightAmbient;             // 빛의 환경광 색상
vector g_vLightSpecular;            // 빛의 정반사 색상

// 재질
texture2D g_DiffuseTexture;                                     // 텍스처 (재질의 난반사)
vector g_vMtrlAmbient    = vector(0.4f, 0.4f, 0.4f, 1.f);        // 재질의 환경광 반응도
vector g_vMtrlSpecular   = vector(1.f, 1.f, 1.f, 1.f);           // 재질의 정반사 반응도



// Sampler State : 필터링, 래핑 규칙 선언
sampler DefaultSampler = sampler_state
{
    // 텍스처 확대/축소 시 보간 방법 
    Filter = MIN_MAG_MIP_LINEAR;

    // 텍스처 좌표가 [0, 1] 범위를 벗어날 때의 처리 (wrap : 반복)
    AddressU = wrap;
    AddressV = wrap;
};

// 셰이더의 입력 구조체는 C++ 측 정점 구조체 VTXTEX와 1:1 대응 해야 함.
struct VS_IN
{
    float3 vPosition    : POSITION;
    float3 vNormal      : NORMAL;
    float2 vTexcoord    : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition    : SV_POSITION;
    float4 vNormal      : NORMAL;       // 월드 공간 노멀
    float2 vTexcoord    : TEXCOORD0;
    float4 vWorldPos    : TEXCOORD1;    // 월드 공간 위치
};

// 정점 셰이더
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    // WVP 변환
    float4 vPosition        = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    vPosition               = mul(vPosition, g_ViewMatrix);    
    vPosition               = mul(vPosition, g_ProjMatrix);
    
    Out.vPosition = vPosition;
    
    // 노멀을 월드 공간으로 변환
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    
    Out.vTexcoord = In.vTexcoord;
    
    // 월드 좌표 전달 (Specular 시선 벡터 계산용) -> 여기서는 2번대로
    // 1. 카메라 월드 공간 좌표를 로컬로 내려서 시선 벡터 구하기
    // 2. 정점의 로컬 좌표를 월드 공간으로 변환해서 시선 벡터 구하기 
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    
    return Out;
}

// w 나누기 연산 : 2차원 투영 스페이스로의 변환
// 뷰 포트로의 변환 (윈도우 좌표로의 변환)
// 레스터라이즈 (정점 정보를 기반으로 해서 픽셀의 정보를 생성)

struct PS_IN
{
    float4 vPosition    : SV_POSITION;
    float4 vNormal      : NORMAL; 
    float2 vTexcoord    : TEXCOORD0;
    float4 vWorldPos    : TEXCOORD1; 
};

struct PS_OUT
{
    float4 vColor       : SV_TARGET0;
};

// 픽셀 셰이더 - 픽셀의 최종적인 색을 결정해준다
// 따라서 반환 타입 구조체는 멤버로 float4  vColor를 갖는다.

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    // (1) 텍스처 샘플링 (재질의 Diffuse 색상)
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    // (2) Diffuse (Lambert) + Ambient
    vector vShade = max(dot(normalize(g_vLightDir) * -1.f, In.vNormal), 0.f) + (g_vLightAmbient * g_vMtrlAmbient);
    
    // (3) Specular (Phong Reflection)
    vector vReflect     = reflect(normalize(g_vLightDir), normalize(In.vNormal));
    vector vLook        = In.vWorldPos - g_vCamPosition;
    
    float fSpecular = pow(max(dot(normalize(vLook) * -1.f, normalize(vReflect)), 0.f), 50.f);
    
    // (4) 최종 색상 조합
    Out.vColor = g_vLightDiffuse * vMtrlDiffuse * saturate(vShade) + (g_vLightSpecular * g_vMtrlSpecular) * fSpecular;
    
    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}