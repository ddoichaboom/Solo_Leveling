#include "Shader_Defines.hlsli"

// Shader_VtxAnimMesh.hlsl - Skinned Animation Model

float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
float4x4 g_BoneMatrices[512];
float g_fAlpha = 1.f;

vector g_vCamPosition;

vector g_vLightDir;
vector g_vLightDiffuse;
vector g_vLightAmbient;
vector g_vLightSpecular;

texture2D g_DiffuseTexture;
vector g_vMtrlAmbient   = vector(0.4f, 0.4f, 0.4f, 1.f);
vector g_vMtrlSpecular  = vector(1.f, 1.f, 1.f, 1.f);

struct VS_IN
{
    float3 vPosition        : POSITION;
    float3 vNormal          : NORMAL;
    float2 vTexcoord        : TEXCOORD;
    float3 vTangent         : TANGENT;
    float3 vBinormal        : BINORMAL;
    uint4 vBlendIndex       : BLENDINDEX;
    float4 vBlendWeight     : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition        : SV_POSITION;
    float4 vNormal          : NORMAL;
    float2 vTexcoord        : TEXCOORD0;
    float4 vWorldPos        : TEXCOORD1;
};

// Á¤Á¡ ¼ÎÀÌ´õ
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    float fWeightW = 1.f - (In.vBlendWeight.x + In.vBlendWeight.y + In.vBlendWeight.z);
    //float4 vPosition = float4(In.vPosition, 1.f);
    //float4 vNormal = float4(In.vNormal, 0.f);
    
    
    float4x4 BoneMatrix = g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
                        g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
                        g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
                        g_BoneMatrices[In.vBlendIndex.w] * fWeightW;
    
    float4 vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float4 vNormal = mul(float4(In.vNormal, 0.f), BoneMatrix);
    
    float4x4 matWV, matWVP;
    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);
    
    Out.vPosition = mul(vPosition, matWVP);
    Out.vNormal = normalize(mul(vNormal, g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    
    return Out;
}

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

// ÇÈ¼¿ ¼ÎÀÌ´õ
PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    
    //if (vMtrlDiffuse.a < 0.1f)
    //    discard;
    
    /* ºûÀ» ¸¹ÀÌ ¹ÞÀ¸¸é1, ¾È¹ÞÀ¸¸é0) */
    vector vShade = max(dot(normalize(g_vLightDir) * -1.f, In.vNormal), 0.f) + (g_vLightAmbient * g_vMtrlAmbient);
    
    
    vector vReflect = reflect(normalize(g_vLightDir), normalize(In.vNormal));
    vector vLook = In.vWorldPos - g_vCamPosition;
        
    float fSpecular = pow(max(dot(normalize(vLook) * -1.f, normalize(vReflect)), 0.f), 50.f);
    
    Out.vColor = g_vLightDiffuse * vMtrlDiffuse * saturate(vShade) +
        (g_vLightSpecular * g_vMtrlSpecular) * fSpecular;
    
    Out.vColor.a = g_fAlpha;
    
    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass BlendPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}

