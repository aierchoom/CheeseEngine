#ifndef GRAPHICS_SHADER_REFLECT_H
#define GRAPHICS_SHADER_REFLECT_H

#include <unordered_map>
#include <vector>
#include <memory>

#include <DirectXMath.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include "Core/CoreMinimal.h"
#include "Graphics/D3DUtil.h"
#include "Graphics/IGraphics.h"

#define CONSTANT_BUFFER_ALIGN_SIZE 256

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
  ConstantBuffer() = default;
  ConstantBuffer(const CheString& name, uint32 startSlot, uint32 byteWidth)
      : mName(name), mCBufferSlot(startSlot), mByteSize(byteWidth)
  {
    mByteSize = CalcAlignBufferByteSize(mByteSize);
  }

  HRESULT Create(ID3D12Device* device);

  inline void SetVariableOffset(const CheString& varName, uint32 offset) { mVariableOffsetMap[varName] = offset; }

  void SetRawData(const CheString& varName, const Byte* data, uint32 size);

  inline uint32 GetSlot() const { return mCBufferSlot; }

  inline static uint32 CalcAlignBufferByteSize(uint32 byteSize, uint32 alignSize = CONSTANT_BUFFER_ALIGN_SIZE)
  {
    // Addition carry,clear low bit
    const uint32 clearBit = alignSize - 1;
    return (byteSize + (clearBit - 1)) & ~(clearBit - 1);
  }

  // private:
 public:
  CheString mName;
  uint32 mCBufferSlot;
  ComPtr<ID3D12Resource> mUploadBuffer;
  std::unordered_map<CheString, uint32> mVariableOffsetMap;

  Byte* mData = nullptr;
  uint32 mByteSize;
};

class ShaderInfo
{
 public:
  ShaderInfo() = default;

  void Create(ID3DBlob* shaderByteCode, ID3D12Device* device);
  void CreateRootSignature(ID3D12Device* device, uint32 cbufferSize);
  void CreateCbvHeapDescriptor(ID3D12Device* device, uint32 descriptorNo);

  void SetFloat(const CheString& varName, float value);
  void SetFloat3(const CheString& varName, const DirectX::XMFLOAT3& value);
  void SetMatrix(const CheString& varName, const DirectX::XMMATRIX& matrix);
  void SetMaterial(const CheString& varName, const Material& material);
  void SetLight(const CheString& varName, const Light& light);

  ID3D12DescriptorHeap* GetCbvHeap() const { return mCbvHeap.Get(); }
  ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }

 public:
  ComPtr<ID3D12DescriptorHeap> mCbvHeap      = nullptr;
  ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
  std::unordered_map<CheString, ConstantBuffer> mCBuffers;
};

#endif  // GRAPHICS_SHADER_REFLECT_H