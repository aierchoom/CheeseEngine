#include "ConstantBuffer.h"

#include "Graphics/D3DUtil.h"
#include "Utils/Log/Logger.h"
#include "d3dx12.h"

using namespace std;

std::unordered_map<CheString, CBufferType> CBufferManager::CBufferConfig{
    {CTEXT("cbPerObject"), CBufferType::PEROBJECT},
    {CTEXT("cbPass"), CBufferType::PASS},
};

HRESULT ConstantBuffer::CreateGPUResource(ID3D12Device* device) {
  if (mUploadBuffer != nullptr) return S_OK;
  if (mCbInfo.GetSlot() == UNINIT_SLOT_VALUE) {
    logger.Error(CTEXT("Uninit cbuffer info"));
    return E_FAIL;
  }

  CD3DX12_HEAP_PROPERTIES heapProps =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC resDesc =
      CD3DX12_RESOURCE_DESC::Buffer(mCbInfo.GetByteSize());

  TIFF(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                       &resDesc,
                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                       nullptr, IID_PPV_ARGS(&mUploadBuffer)));

  mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mData));
  return S_OK;
}

void ConstantBuffer::SetRawData(const CheString& varName, const Byte* data,
                                uint32 size) {
  if (size > mCbInfo.GetByteSize()) {
    return;
  }
  auto& variableOffsets = mCbInfo.GetVariableOffsets();
  // if varName incorrect,do not set.
  if (variableOffsets.find(varName) != variableOffsets.end()) {
    uint32 offset = variableOffsets[varName];
    Byte* dataStart = mData.get() + offset;
    memcpy_s(dataStart, size, data, size);
  }
}