#include "ShaderInfo.h"
#include <strsafe.h>
#include "d3dx12.h"
#include "Graphics/D3DUtil.h"
#include "Utils/Log/Logger.h"

using namespace std;

ConstantBuffer::ConstantBuffer(ConstantBuffer&& rhs) noexcept
    : mCbInfo(rhs.mCbInfo), mData(rhs.mData), mUploadBuffer(rhs.mUploadBuffer)
{
  if (mData != nullptr) {
    rhs.mData = nullptr;
  }
  if (mUploadBuffer != nullptr) {
    rhs.mUploadBuffer = nullptr;
  }
}

const ConstantBuffer& ConstantBuffer::operator=(ConstantBuffer&& rhs)
{
  mCbInfo       = rhs.mCbInfo;
  mData         = rhs.mData;
  mUploadBuffer = rhs.mUploadBuffer;
  if (mData != nullptr) {
    rhs.mData = nullptr;
  }
  if (mUploadBuffer != nullptr) {
    rhs.mUploadBuffer = nullptr;
  }
  return (*this);
}

HRESULT ConstantBuffer::CreateGPUResource(ID3D12Device* device)
{
  if (mUploadBuffer != nullptr) return S_OK;
  if (mCbInfo.GetSlot() == UNINIT_SLOT_VALUE) {
    logger.Error(CTEXT("Uninit cbuffer info"));
    return E_FAIL;
  }

  CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC resDesc     = CD3DX12_RESOURCE_DESC::Buffer(mCbInfo.GetByteSize());

  TIFF(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&mUploadBuffer)));

  mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mData));
  return S_OK;
}

void ConstantBuffer::SetRawData(const CheString& varName, const Byte* data, uint32 size)
{
  if (size > mCbInfo.GetByteSize()) {
    return;
  }
  auto& variableOffsets = mCbInfo.GetVariableOffsets();
  // if varName incorrect,do not set.
  if (variableOffsets.find(varName) != variableOffsets.end()) {
    uint32 offset   = variableOffsets[varName];
    Byte* dataStart = mData + offset;
    memcpy_s(dataStart, size, data, size);
  }
}

void ShaderInfo::Create(ID3D12Device* device, const ShaderSettings& settings)
{
  for (auto pair : settings.GetCBSetting()) {
    auto cbName    = pair.first;
    auto cbSetting = pair.second;
    if (mCBuffers.find(cbName) == mCBuffers.end()) {
      ConstantBuffer cbuffer(cbSetting);
      cbuffer.CreateGPUResource(device);
      mCBuffers[cbName] = std::move(cbuffer);
    }
  }
}

void ShaderInfo::SetFloat(const CheString& varName, float value)
{
  uint64 splitPoint = varName.find(CheChar('.'));
  CheChar cbufferName[64];
  ZeroMemory(cbufferName, 64);
  StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

  if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
    CheChar cbufferVarName[64];
    lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
    const Byte* byteData = (Byte*)&value;
    uint32 dataSize      = sizeof(value);
    mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
  }
}

void ShaderInfo::SetFloat3(const CheString& varName, const DirectX::XMFLOAT3& value)
{
  uint64 splitPoint = varName.find(CheChar('.'));
  CheChar cbufferName[64];
  ZeroMemory(cbufferName, 64);
  StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

  if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
    CheChar cbufferVarName[64];
    lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
    const Byte* byteData = (Byte*)&value;
    uint32 dataSize      = sizeof(value);
    mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
  }
}

void ShaderInfo::SetMatrix(const CheString& varName, const DirectX::XMMATRIX& matrix)
{
  uint64 splitPoint = varName.find(CheChar('.'));
  CheChar cbufferName[64];
  ZeroMemory(cbufferName, 64);
  StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

  if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
    CheChar cbufferVarName[64];
    lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
    const Byte* byteData = (Byte*)&matrix;
    uint32 dataSize      = sizeof(matrix);
    mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
  }
}

void ShaderInfo::SetMaterialDesc(const CheString& varName, const MaterialDesc& material)
{
  uint64 splitPoint = varName.find(CheChar('.'));
  CheChar cbufferName[64];
  ZeroMemory(cbufferName, 64);
  StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

  if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
    CheChar cbufferVarName[64];
    lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
    const Byte* byteData = (Byte*)&material;
    uint32 dataSize      = sizeof(material);
    mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
  }
}

void ShaderInfo::SetLight(const CheString& varName, const Light& light)
{
  uint64 splitPoint = varName.find(CheChar('.'));
  CheChar cbufferName[64];
  ZeroMemory(cbufferName, 64);
  StringCchCopy(cbufferName, splitPoint + 1, varName.c_str());

  if (mCBuffers.find(cbufferName) != mCBuffers.end()) {
    CheChar cbufferVarName[64];
    lstrcpyW(cbufferVarName, varName.c_str() + splitPoint + 1);
    const Byte* byteData = (Byte*)&light;
    uint32 dataSize      = sizeof(light);
    mCBuffers[cbufferName].SetRawData(cbufferVarName, byteData, dataSize);
  }
}