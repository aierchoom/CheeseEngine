#include "DefaultSampler.hlsli"
#include "LightingUtil.hlsli"

Texture2D gAlbedoMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gShadowMap : register(t2);

cbuffer cbPerObject : register(b0)
{
  matrix gWorld;
  matrix gViewProj;
  matrix gShadowTransform;
};

cbuffer cbMaterial : register(b1)
{
  Material gMaterial;
  Light gLight;
  float3 gEyePosW;
};

struct VertexIn {
  float3 PosL : POSITION;
  float3 NormalL : NORMAL;
  float3 TangentU : TANGENT;
  float2 TexC : TEXCOORD;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float3 PosW : POSITION0;
  float4 ShadowPosH : POSITION1;
  float3 NormalW : NORMAL;
  float3 TangentW : TANGENT;
  float2 TexC : TEXCOORD;
};

float CalcShadowFactor(float4 shadowPosH)
{
  // Complete projection by doing division by w.
  shadowPosH.xyz /= shadowPosH.w;

  // Depth in NDC space.
  float depth = shadowPosH.z;

  uint width, height, numMips;
  gShadowMap.GetDimensions(0, width, height, numMips);

  // Texel size.
  float dx = 1.0f / (float)width;

  float percentLit        = 0.0f;
  const float2 offsets[9] = {float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),   float2(-dx, 0.0f), float2(0.0f, 0.0f),
                             float2(dx, 0.0f), float2(-dx, +dx),  float2(0.0f, +dx), float2(dx, +dx)};

  [unroll] for (int i = 0; i < 9; ++i)
  {
    percentLit += gShadowMap.SampleCmpLevelZero(gSamShadow, shadowPosH.xy + offsets[i], depth).r;
  }

  return percentLit / 9.0f;
}

VertexOut VS(VertexIn vin)
{
  VertexOut vout = (VertexOut)0.0f;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW   = posW.xyz;

  vout.NormalW  = mul(vin.NormalL, (float3x3)gWorld);
  vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);
  vout.PosH     = mul(posW, gViewProj);
  vout.TexC     = vin.TexC;

  // Generate projective tex-coords to project shadow map onto scene.
  vout.ShadowPosH = mul(posW, gShadowTransform);

  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  pin.NormalW = normalize(pin.NormalW);

  float3 toEyeW = normalize(gEyePosW - pin.PosW);

  float4 diffuseAlbedo = gAlbedoMap.Sample(gLinearWrap, pin.TexC);

  float3 normal       = gNormalMap.Sample(gLinearWrap, pin.TexC).rgb;
  float3 bumpedNormal = NormalSampleToWorldSpace(normal, pin.NormalW, pin.TangentW);

  float4 ambient = diffuseAlbedo * 0.3f;

  float shadowFactor = CalcShadowFactor(pin.ShadowPosH);

  Material material      = gMaterial;
  material.DiffuseAlbedo = diffuseAlbedo;
  float3 directLight     = ComputeDirectionalLight(gLight, material, bumpedNormal, toEyeW);

  float3 litColor = ambient.xyz + directLight * shadowFactor;

  return float4(litColor, gMaterial.DiffuseAlbedo.a);
}
