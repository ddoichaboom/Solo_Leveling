// float2, float3, float4 == Vector
// float1x3, float2x3, float4x4 == Matrix

// HLSL 코드 내에서 사용할 전역 변수 선언
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
texture2D g_Texture;

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
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

// 정점 셰이더
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    // 1단계 - 월드 변환 : 로컬 -> 월드 (오브젝트를 세계에 배치)
    float4 vPosition = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    
    // 2단계 - 뷰 변환 : 월드 -> 뷰 (카메라 기준으로 변환)
    vPosition = mul(vPosition, g_ViewMatrix);
    
    // 3단계 - 투영 변환 : 뷰 -> 투영 (원근감 적용, 클리핑 준비
    vPosition = mul(vPosition, g_ProjMatrix);
    
    // 클립 좌표
    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}

// w 나누기 연산 : 2차원 투영 스페이스로의 변환
// 뷰 포트로의 변환 (윈도우 좌표로의 변환)
// 레스터라이즈 (정점 정보를 기반으로 해서 픽셀의 정보를 생성)

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

// 픽셀 셰이더 - 픽셀의 최종적인 색을 결정해준다
// 따라서 반환 타입 구조체는 멤버로 float4  vColor를 갖는다.

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    
    // Alpha Test 
    if (Out.vColor.a < 0.1f)
        discard;
    
    Out.vColor.gb = Out.vColor.r; // 회색처럼 표시 
    
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