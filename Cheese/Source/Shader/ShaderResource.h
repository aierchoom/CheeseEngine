#ifndef SHADER_SHADER_RESOURCE_H
#define SHADER_SHADER_RESOURCE_H
#include <DirectXMath.h>

struct PointLight {
  DirectX::XMFLOAT3 strength;
  float falloffStart;
  DirectX::XMFLOAT3 direction;
  float falloffEnd;
  DirectX::XMFLOAT3 position;
  float SpotPower;
};

struct DirLight {
  DirectX::XMFLOAT3 strength;
  float pad;
  DirectX::XMFLOAT3 direction;
  float pad1;
};

struct MaterialDesc {
  DirectX::XMFLOAT4 DiffuseAlbedo;
  DirectX::XMFLOAT3 FresnelR0;
  float Roughness;
};

#endif  // SHADER_SHADER_RESOURCE_H