SamplerState gPointWrap : register(s0);
SamplerState gPointClamp : register(s1);
SamplerState gLinearWrap : register(s2);
SamplerState gLinearClamp : register(s3);
SamplerState gAnisotropicWrap : register(s4);
SamplerState gAnisotropicClamp : register(s5);
SamplerComparisonState gSamShadow : register(s6);

struct PointLight {
  float3 Strength;
  float FalloffStart;
  float3 Direction;
  float FalloffEnd;
  float3 Position;
  float SpotPower;
};

struct DirLight {
  float3 Strength;
  float pad;
  float3 Direction;
  float pad1;
};

struct MaterialDesc {
  float4 DiffuseAlbedo;
  float3 FresnelR0;
  float Roughness;
};

float3 NormalSampleToWorldSpace(float3 normal_map_sample, float3 unit_normalW, float3 tangentW)
{
  float3 normalT = 2.0f * normal_map_sample - 1.0f;
  float3 N       = unit_normalW;
  float3 T       = normalize(tangentW - dot(tangentW, N) * N);
  float3 B       = cross(N, T);

  float3x3 TBN        = float3x3(T, B, N);
  float3 bumpedNormal = mul(normalT, TBN);
  return bumpedNormal;
}