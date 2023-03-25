static const float PI = 3.14159265359;

float DistributionGGX(float3 normal, float3 half_vec, float roughness)
{
  float a      = roughness * roughness;
  float a2     = a * a;
  float ndoth  = max(dot(normal, half_vec), 0.0f);
  float ndoth2 = ndoth * ndoth;

  float nom   = a2;
  float denom = (ndoth2 * (a2 - 1.0f) + 1.0f);
  denom       = PI * denom * denom;

  return nom / denom;
}

float GeometrySchlickGGX(float ndotv, float roughness)
{
  float r = (roughness + 1.0f);
  float k = (r * r) / 8.0f;

  float nom   = ndotv;
  float denom = ndotv * (1.0f - k) + k;

  return nom / denom;
}

float GeometrySmith(float3 normal, float3 view_dir, float3 light_dir, float roughness)
{
  float ndotv = max(dot(normal, view_dir), 0.0f);
  float ndotl = max(dot(normal, light_dir), 0.0f);
  float ggx2  = GeometrySchlickGGX(ndotv, roughness);
  float ggx1  = GeometrySchlickGGX(ndotl, roughness);

  return ggx1 * ggx2;
}

float3 SchlickFresnel(float cos_theta, float3 F0) { return F0 + (1.0f - F0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f); }