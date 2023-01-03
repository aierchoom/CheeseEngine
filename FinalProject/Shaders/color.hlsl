struct Light {
  float3 Strength;
  float FalloffStart;
  float3 Direction;
  float FalloffEnd;
  float3 Position;
  float SpotPower;
};

struct Material {
  float4 DiffuseAlbedo;
  float3 FresnelR0;
  float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
  return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
  float cosIncidentAngle = saturate(dot(normal, lightVec));

  float f0              = 1.0f - cosIncidentAngle;
  float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

  return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
  const float m  = mat.Shininess * 256.0f;
  float3 halfVec = normalize(toEye + lightVec);

  float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
  float3 fresnelFactor  = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

  float3 specAlbedo = fresnelFactor * roughnessFactor;

  // Our spec formula goes outside [0,1] range, but we are
  // doing LDR rendering.  So scale it down a bit.
  specAlbedo = specAlbedo / (specAlbedo + 1.0f);

  return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
  float3 lightVec = L.Position - pos;

  float d = length(lightVec);

  if (d > L.FalloffEnd) return 0.0f;

  lightVec /= d;

  float ndotl          = max(dot(lightVec, normal), 0.0f);
  float3 lightStrength = L.Strength * ndotl;

  float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
  lightStrength *= att;

  return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

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
  // float2 Tex : TEXTCOORD;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float3 PosW : POSITION;
  float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
  VertexOut vout = (VertexOut)0.0f;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW   = posW.xyz;

  vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

  vout.PosH = mul(posW, gCameraTrans);

  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  pin.NormalW = normalize(pin.NormalW);

  float3 toEyeW = normalize(gEyePosW - pin.PosW);

  float4 ambient = float4(0.2f, 0.2f, 0.2f, 1.0f) * gMaterial.DiffuseAlbedo;

  const float shininess = gMaterial.Shininess;
  float3 shadowFactor   = 1.0f;
  float3 directLight    = ComputePointLight(gLight, gMaterial, pin.PosW, pin.NormalW, toEyeW);

  float3 litColor = ambient.xyz + directLight;

  return float4(litColor, gMaterial.DiffuseAlbedo.a);
}