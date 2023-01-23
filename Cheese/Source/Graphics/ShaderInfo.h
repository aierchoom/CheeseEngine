#ifndef GRAPHICS_SHADER_REFLECT_H
#define GRAPHICS_SHADER_REFLECT_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <array>

#include <DirectXMath.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include "Core/CoreMinimal.h"
#include "Graphics/D3DUtil.h"
#include "Graphics/IGraphics.h"
#include "Model/Texture2D.h"

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

  void Generate(ID3DBlob* shaderByteCode, ID3D12Device* device);
  void CreateConstantBuffer(ID3D12Device* device, D3D12_SHADER_INPUT_BIND_DESC bindDesc,
                            ID3D12ShaderReflectionConstantBuffer* cbReflection);
  void CreateRootSignature(ID3D12Device* device);

  void SetFloat(const CheString& varName, float value);
  void SetFloat3(const CheString& varName, const DirectX::XMFLOAT3& value);
  void SetMatrix(const CheString& varName, const DirectX::XMMATRIX& matrix);
  void SetMaterial(const CheString& varName, const Material& material);
  void SetLight(const CheString& varName, const Light& light);

  std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers()
  {
    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,                                 // shaderRegister
                                                D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,                                  // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,                                 // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,                                  // shaderRegister
                                                  D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4,                                // shaderRegister
                                                      D3D12_FILTER_ANISOTROPIC,         // filter
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                                                      0.0f,                             // mipLODBias
                                                      8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,                                 // shaderRegister
                                                       D3D12_FILTER_ANISOTROPIC,          // filter
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
                                                       0.0f,                              // mipLODBias
                                                       8);                                // maxAnisotropy

    return {pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp};
  }

  ID3D12DescriptorHeap* GetCbvHeap() const { return mCbvHeap.Get(); }
  ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }

  std::unordered_map<CheString, ConstantBuffer> mCBuffers;
  std::unordered_map<CheString, Texture2D> mTextures;
  std::unordered_map<CheString, CD3DX12_STATIC_SAMPLER_DESC> mSamplers;

 private:
  ComPtr<ID3D12DescriptorHeap> mCbvHeap      = nullptr;
  ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
};

#endif  // GRAPHICS_SHADER_REFLECT_H