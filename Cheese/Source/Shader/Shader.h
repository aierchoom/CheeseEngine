#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H
#include "Common/TypeDef.h"
#include <array>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include "ShaderHelper.h"
#include "ShaderInfo.h"

enum class ShaderType : uint8 {
  VERTEX_SHADER   = 0,
  HULL_SHADER     = 1,
  DOMAIN_SHADER   = 2,
  GEOMETRY_SHADER = 3,
  PIXEL_SHADER    = 4,
};

class Shader
{
 public:
  Shader() {}
  void AddShader(const CheString& fileName, ShaderType type);
  ShaderSettings GetSettings() const { return mSettings; }

  ID3DBlob* GetVS() const { return mVsByteCode.Get(); }
  ID3DBlob* GetPS() const { return mPsByteCode.Get(); }
  ID3DBlob* GetGS() const { return mGsByteCode.Get(); }
  ID3DBlob* GetHS() const { return mHsByteCode.Get(); }
  ID3DBlob* GetDS() const { return mDsByteCode.Get(); }

  void CreateRootSignature(ID3D12Device* device);
  inline ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }

 private:
  void AddVS(ID3DBlob* shaderByteCode);
  void AddPS(ID3DBlob* shaderByteCode);
  void AddGS(ID3DBlob* shaderByteCode);
  void AddHS(ID3DBlob* shaderByteCode);
  void AddDS(ID3DBlob* shaderByteCode);

  void GenerateShaderSettings(ID3DBlob* shader);
  void GenerateCBSettings(D3D12_SHADER_INPUT_BIND_DESC bindDesc, ID3D12ShaderReflectionConstantBuffer* cbReflection);

  std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

 private:
  ComPtr<ID3DBlob> mVsByteCode = nullptr;
  ComPtr<ID3DBlob> mPsByteCode = nullptr;
  ComPtr<ID3DBlob> mGsByteCode = nullptr;
  ComPtr<ID3DBlob> mHsByteCode = nullptr;
  ComPtr<ID3DBlob> mDsByteCode = nullptr;

  ShaderSettings mSettings;

  ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
};

#endif  // GRAPHICS_SHADER_H
