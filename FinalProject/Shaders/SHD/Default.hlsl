#include "Common.hlsl"

struct VertexIn {
  float3 PosL : POSITION;
  float3 NormalL : NORMAL;
  float2 TexC : TEXCOORD;
  float3 TangentU : TANGENT;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float4 ShadowPosH : POSITION0;
  float3 PosW : POSITION1;
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
  vout.PosH     = mul(posW, gViewProj);
  vout.TexC     = vin.TexC;

  vout.ShadowPosH = mul(posW, gShadowTransform);

  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  // Fetch the material data.
  Material matData     = gMaterial;
  float4 diffuseAlbedo = matData.DiffuseAlbedo;
  float3 fresnelR0     = matData.FresnelR0;
  float roughness      = matData.Roughness;

  diffuseAlbedo *= gAlbedoMap.Sample(gsamAnisotropicWrap, pin.TexC);

  pin.NormalW = normalize(pin.NormalW);

  float4 normalMapSample = gNormalMap.Sample(gsamAnisotropicWrap, pin.TexC);
  float3 bumpedNormalW   = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

  float3 toEyeW = normalize(gEyePosW - pin.PosW);

  // Light terms.
  float4 ambient = 0.3f * diffuseAlbedo;

  float shadowFactor = CalcShadowFactor(pin.ShadowPosH);

  const float shininess = (1.0f - roughness) * normalMapSample.a;
  Material mat          = {diffuseAlbedo, fresnelR0, shininess};
  float4 directLight    = ComputeLighting(gLight, mat, pin.PosW, bumpedNormalW, toEyeW, shadowFactor);

  float4 litColor = ambient + directLight;

  litColor.a = diffuseAlbedo.a;

  return litColor;
}
