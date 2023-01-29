#include "DefaultSampler.hlsli"
#include "LightingUtil.hlsli"

Texture2D gAlbedoMap : register(t0);
Texture2D gNormalMap : register(t1);

cbuffer cbPerObject : register(b0)
{
  matrix gWorld;
  matrix gCameraTrans;
  matrix gInvWorld;
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
  float3 PosW : POSITION;
  float3 NormalW : NORMAL;
  float3 TangentW : TANGENT;
  float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
  VertexOut vout = (VertexOut)0.0f;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW   = posW.xyz;

  vout.NormalW  = mul(vin.NormalL, (float3x3)gWorld);
  vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

  vout.PosH = mul(posW, gCameraTrans);
  vout.TexC = vin.TexC;

  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  pin.NormalW = normalize(pin.NormalW);

  float3 toEyeW = normalize(gEyePosW - pin.PosW);

  float4 diffuseAlbedo = gAlbedoMap.Sample(gLinearWrap, pin.TexC);

  float3 normal       = gNormalMap.Sample(gLinearWrap, pin.TexC).rgb;
  float3 bumpedNormal = NormalSampleToWorldSpace(normal, pin.NormalW, pin.TangentW);

  float4 ambient = diffuseAlbedo * gMaterial.DiffuseAlbedo;

  const float shininess = gMaterial.Shininess;

  Material material = gMaterial;

  material.DiffuseAlbedo = diffuseAlbedo;
  float3 directLight     = ComputePointLight(gLight, material, pin.PosW, bumpedNormal, toEyeW);

  float3 litColor = ambient.xyz + directLight;

  return float4(litColor, gMaterial.DiffuseAlbedo.a);
}
