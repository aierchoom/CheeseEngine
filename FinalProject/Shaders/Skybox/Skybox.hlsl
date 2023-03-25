#include "../Basic.hlsli"

TextureCube gCubeMap : register(t0);

cbuffer cbPerObject : register(b0) { float4x4 gWorld; };

cbuffer cbPass : register(b1)
{
  matrix gViewProj;
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
  float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
  VertexOut vout;

  vout.PosL = vin.PosL;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

  posW.xyz += gEyePosW;

  vout.PosH = mul(posW, gViewProj).xyww;

  return vout;
}

float4 PS(VertexOut pin) : SV_Target { return gCubeMap.Sample(gLinearWrap, pin.PosL); }