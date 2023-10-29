#include "../Basic.hlsli"
#include "../LightHelper.hlsli"

Texture2D gAlbedoMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gORMMap : register(t2);
Texture2D gShadowMap : register(t3);

struct VertexIn {
  float3 PosL : POSITION;
  float3 NormalL : NORMAL;
  float3 Tangent : TANGENT;
  float2 Texcoord : TEXCOORD;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float3 PosW : POSITION0;
  float4 CurPosition: POSITION1;
  float4 PrevPosition: POSITION2;
  float4 ShadowPosH : ShadowPosition;
  float3 NormalW : NORMAL;
  float3 TangentW : TANGENT;
  float2 Texcoord : TEXCOORD;
};

float CalcShadowFactor(float4 shadowPosH)
{
  // Complete projection by doing division by w.
  shadowPosH.xyz /= shadowPosH.w;

  // Depth in NDC sapce.
  const float bias = 0.0001f;
  float depth      = shadowPosH.z - bias;

  uint width, height, numMips;
  gShadowMap.GetDimensions(0, width, height, numMips);

  // Texel size.
  float dx = 1.0f / (float)width;

  float percentLit        = 0.0f;
  const float2 offsets[9] = {float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),   float2(-dx, 0.0f), float2(0.0f, 0.0f),
                             float2(dx, 0.0f), float2(-dx, +dx),  float2(0.0f, +dx), float2(dx, +dx)};

  [unroll] for (int i = 0; i < 9; i++) { percentLit += gShadowMap.SampleCmpLevelZero(gSamShadow, shadowPosH.xy + offsets[i], depth).r; }

  return percentLit / 9.0f;
}