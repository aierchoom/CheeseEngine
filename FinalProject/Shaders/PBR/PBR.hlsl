#include "PBR.hlsli"

cbuffer cbPerObject : register(b0)
{
  matrix gWorld;
  matrix PrevWorldTransform;
  MaterialDesc gMatDesc;
};

cbuffer cbPass : register(b1)
{
  matrix gViewProj;
  matrix PrevViewProjectionMatrix;
  float2 PrevJitter;
  float2 CurrJitter;
  matrix gShadowTransform;
  PointLight gLight;
  float3 gEyePosW;
};

struct GBuffer {
  float4 colors : SV_Target0;
  float2 MotionVectors : SV_Target1;
};

VertexOut VS(VertexIn vin)
{
  VertexOut vout = (VertexOut)0.0f;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW   = posW.xyz;

  // transform normal and save tangent,texcoord data.
  vout.NormalW  = mul(vin.NormalL, (float3x3)gWorld);
  vout.TangentW = mul(vin.Tangent, (float3x3)gWorld);
  vout.Texcoord = vin.Texcoord;

  // multiple view & proj.
  vout.PosH = mul(float4(vout.PosW,1.0f), gViewProj);

  // for motion vector
  vout.CurPosition = vout.PosH;
  const float4 worldPrevPos = mul(float4(vin.PosL, 1.0f), PrevWorldTransform);
  vout.PrevPosition = mul(worldPrevPos, PrevViewProjectionMatrix);

  vout.ShadowPosH = mul(posW, gShadowTransform);
  return vout;
}

GBuffer PS(VertexOut pin)
{
  GBuffer output;
  float3 gamma = 2.2f;

  // normalize normal & tangent to calculate lighting.
  float3 normal  = normalize(pin.NormalW);
  float3 tangent = normalize(pin.TangentW);

  float4 diffuseAlbedo = gAlbedoMap.Sample(gLinearWrap, pin.Texcoord);
  float3 albedo        = pow(abs(diffuseAlbedo.rgb), gamma.x);

  float3 normalSample = gNormalMap.Sample(gLinearWrap, pin.Texcoord).xyz;
  float3 bumpedNormal = NormalSampleToWorldSpace(normalSample, normal, tangent);

  float3 orm = 0.0f;

  uint texWidth = 0, texHeight = 0;
  gORMMap.GetDimensions(texWidth, texHeight);
  [flatten] if (texWidth != 4 && texHeight != 4) { orm = gORMMap.Sample(gLinearWrap, pin.Texcoord).rgb; }
  else
  {
    orm.r = 0.3f;
    orm.g = gMatDesc.Roughness;
    orm.b = 0.02f;
  }

  float3 litColor = 0.0f;
  float ao        = orm.r;
  float roughness = orm.g;
  float metallic  = orm.b;

  float3 FresnelR0 = gMatDesc.FresnelR0;
  float3 F0        = lerp(FresnelR0, albedo, metallic);

  // calculate one dir light source.
  float3 Lo = 0.0f;

  float3 lightDir = normalize(-gLight.Direction);
  // float3 lightDir = normalize(pin.PosW-gLight.Position);
  float3 viewDir = normalize(gEyePosW - pin.PosW);

  // calculate per-light radiance
  float3 halfVec = normalize(viewDir + lightDir);
  // float distance = length(gLight.Position-pin.PosW);
  // float auuenuation = gLight.FalloffStart / (distance*distance)*gLight.FalloffEnd;
  // float3 radiance = gLight.Strength * attenuation;
  float3 radiance = gLight.Strength;

  // cook-torrance BRDF
  float ndf      = DistributionGGX(bumpedNormal, halfVec, roughness);
  float geo      = GeometrySmith(bumpedNormal, viewDir, lightDir, roughness);
  float3 fresnel = SchlickFresnel(max(dot(halfVec, viewDir), 0.0f), F0);

  float3 nominator = ndf * geo * fresnel;
  // +0.0001 to prevent divide by zero.
  float denominator = 4.0f * max(dot(bumpedNormal, viewDir), 0.0f) * max(dot(bumpedNormal, lightDir), 0.0f) + 0.0001f;
  float3 specular   = nominator / denominator;

  // ks is equal to fresnel.
  float3 ks = fresnel;
  float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;

  kd *= 1.0f - metallic;

  float ndotl = max(dot(bumpedNormal, lightDir), 0.0f);

  Lo += (kd * albedo / PI + specular) * radiance * ndotl;

  // end to calculate light.

  float shadowFactor = CalcShadowFactor(pin.ShadowPosH);
  // float3 ambient = albedo*0.25f;
  float3 ambient = albedo * ao * 0.25f;
  litColor       = ambient + Lo * shadowFactor;

  // hdr tonemapping.
  litColor = litColor / (litColor + float3(1.0f, 1.0f, 1.0f));

  // gamma correct
  litColor = pow(abs(litColor), 1.0f / gamma);

  output.colors =  float4(litColor, diffuseAlbedo.a);

  float2 cancelJitter = PrevJitter - CurrJitter;
  float2 MotionVectors = (pin.PrevPosition.xy / pin.PrevPosition.w) -
                            (pin.CurPosition.xy / pin.CurPosition.w);
  output.MotionVectors = MotionVectors - cancelJitter;

  // Transform motion vectors from NDC space to UV space (+Y is top-down).
  output.MotionVectors *= float2(0.5f, -0.5f);

  return output;
}