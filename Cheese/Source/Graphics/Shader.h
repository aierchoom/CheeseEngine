#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H
#include "Common/TypeDef.h"

#include <d3d12.h>
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
  Shader() : mShaderInfo(new ShaderInfo()) {}
  void AddShader(const CheString& fileName, ShaderType type);
  void GenerateShaderInfo(ID3D12Device* device);
  ShaderInfo* GetShaderInfo() const { return mShaderInfo; }

  ID3DBlob* GetVS() const { return mVsByteCode.Get(); }
  ID3DBlob* GetPS() const { return mPsByteCode.Get(); }
  ID3DBlob* GetGS() const { return mGsByteCode.Get(); }
  ID3DBlob* GetHS() const { return mHsByteCode.Get(); }
  ID3DBlob* GetDS() const { return mDsByteCode.Get(); }

 private:
  void AddVS(ID3DBlob* shaderByteCode);
  void AddPS(ID3DBlob* shaderByteCode);
  void AddGS(ID3DBlob* shaderByteCode);
  void AddHS(ID3DBlob* shaderByteCode);
  void AddDS(ID3DBlob* shaderByteCode);

 private:
  ComPtr<ID3DBlob> mVsByteCode = nullptr;
  ComPtr<ID3DBlob> mPsByteCode = nullptr;
  ComPtr<ID3DBlob> mGsByteCode = nullptr;
  ComPtr<ID3DBlob> mHsByteCode = nullptr;
  ComPtr<ID3DBlob> mDsByteCode = nullptr;

  ShaderInfo* mShaderInfo;
};

#endif  // GRAPHICS_SHADER_H
