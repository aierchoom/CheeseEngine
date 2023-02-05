#ifndef GRAPHICS_SHADER_REFLECT_H
#define GRAPHICS_SHADER_REFLECT_H

#include "Common/TypeDef.h"
#include "ShaderHelper.h"
#include "Model/Mesh.h"

enum class ShaderFlag : uint8 {
  PixelShader    = 0x1,
  VertexShader   = 0x2,
  GeometryShader = 0x4,
  HullShader     = 0x8,
  DomainShader   = 0x10,
  ComputeShader  = 0x20,
};

class ConstantBuffer
{
 public:
  ConstantBuffer() : mCbInfo() {}
  ConstantBuffer(const CBufferInfo& cbInfo) : mCbInfo(cbInfo) {}
  ConstantBuffer(const ConstantBuffer& rhs) : mCbInfo(rhs.mCbInfo), mData(rhs.mData), mUploadBuffer(rhs.mUploadBuffer) {}
  ConstantBuffer(ConstantBuffer&& rhs) noexcept;

  const ConstantBuffer& operator=(ConstantBuffer&& rhs);

  HRESULT CreateGPUResource(ID3D12Device* device);

  void SetRawData(const CheString& varName, const Byte* data, uint32 size);

  inline const CBufferInfo& GetCBufferInfo() const { return mCbInfo; }

  // private:
 public:
  CBufferInfo mCbInfo;

  Byte* mData = nullptr;
  ComPtr<ID3D12Resource> mUploadBuffer;
};

class ShaderInfo
{
 public:
  ShaderInfo() : mCBuffers() {}

  void Create(ID3D12Device* device, const ShaderSettings& settings);
  void SetFloat(const CheString& varName, float value);
  void SetFloat3(const CheString& varName, const DirectX::XMFLOAT3& value);
  void SetMatrix(const CheString& varName, const DirectX::XMMATRIX& matrix);
  void SetMaterialDesc(const CheString& varName, const MaterialDesc& material);
  void SetLight(const CheString& varName, const Light& light);

  inline const std::unordered_map<CheString, ConstantBuffer>& GetCBuffers() const { return mCBuffers; }

 private:
  std::unordered_map<CheString, ConstantBuffer> mCBuffers;
};

#endif  // GRAPHICS_SHADER_REFLECT_H