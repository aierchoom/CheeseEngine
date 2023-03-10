
#include "LightingUtil.hlsl"

Texture2D gAlbedoMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gShadowMap : register(t2);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

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

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
  // Uncompress each component from [0,1] to [-1,1].
  float3 normalT = 2.0f * normalMapSample - 1.0f;

  // Build orthonormal basis.
  float3 N = unitNormalW;
  float3 T = normalize(tangentW - dot(tangentW, N) * N);
  float3 B = cross(N, T);

  float3x3 TBN = float3x3(T, B, N);

  // Transform from tangent space to world space.
  float3 bumpedNormalW = mul(normalT, TBN);

  return bumpedNormalW;
}

//---------------------------------------------------------------------------------------
// PCF for shadow mapping.
//---------------------------------------------------------------------------------------

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
    percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow, shadowPosH.xy + offsets[i], depth).r;
  }

  return percentLit / 9.0f;
}
