#ifndef SHADER_CONSTANT_BUFFER_H
#define SHADER_CONSTANT_BUFFER_H

#include <strsafe.h>

#include <memory>

#include "Common/TypeDef.h"
#include "Model/Mesh.h"
#include "ShaderHelper.h"
#include "ShaderResource.h"

enum class CBufferType : uint8 {
  PEROBJECT = 0,
  PASS      = 1,
};

class ConstantBuffer
{
 public:
  ConstantBuffer() : mCbInfo() {}
  ConstantBuffer(const CBufferInfo& cbInfo) : mCbInfo(cbInfo) {}

  HRESULT CreateGPUResource(ID3D12Device* device);

  inline const CBufferInfo& GetCBufferInfo() const { return mCbInfo; }
  inline ID3D12Resource* GetResource() const { return mUploadBuffer.Get(); }

  friend class CBufferManager;

 private:
  void SetRawData(const CheString& varName, const Byte* data, uint32 size);

 public:
  CBufferInfo mCbInfo;

  std::shared_ptr<Byte> mData = nullptr;
  ComPtr<ID3D12Resource> mUploadBuffer;
};

class CBufferManager
{
 public:
  CBufferManager() : mCBuffers() {}

  inline void AddCBuffer(ID3D12Device* device, const CheString& cbName, const CBufferInfo& cbInfo)
  {
    if (mCBuffers.find(cbName) == mCBuffers.end()) {
      mCBuffers[cbName] = ConstantBuffer(cbInfo);
      mCBuffers[cbName].CreateGPUResource(device);
    }
  }

  template <typename ValueType>
  inline void SetValue(const CheString& varName, const ValueType& value)
  {
    uint64 splitPoint = varName.find(CheChar('.'));
    CheChar cbufferName[64];
    ZeroMemory(cbufferName, 64);
    StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

    if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
      CheChar cbufferVarName[64];
      lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
      const Byte* byteData  = (Byte*)&value;
      const uint32 dataSize = sizeof(value);
      mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
    }
  }

  inline const std::unordered_map<CheString, ConstantBuffer>& GetCBuffers() const { return mCBuffers; }

  static std::unordered_map<CheString, CBufferType> CBufferConfig;

 private:
  std::unordered_map<CheString, ConstantBuffer> mCBuffers;
};

#endif  // SHADER_CONSTANT_BUFFER_H