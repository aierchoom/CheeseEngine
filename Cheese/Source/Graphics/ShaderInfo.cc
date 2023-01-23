#include "ShaderInfo.h"
#include <strsafe.h>
#include "Core/CoreMinimal.h"
#include "Utils/Log/Logger.h"
#include "Model/Texture2D.h"

using namespace std;

HRESULT ConstantBuffer::Create(ID3D12Device* device)
{
  if (mUploadBuffer != nullptr) return S_OK;

  CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC resDesc     = CD3DX12_RESOURCE_DESC::Buffer(mByteSize);

  TIFF(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&mUploadBuffer)));

  // D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mUploadeBuffer->GetGPUVirtualAddress();
  // auto handle                         = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
  // uint32 cbvSrvUavDescriptorSize      = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  // handle.Offset(mCBufferSlot, cbvSrvUavDescriptorSize);

  // D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
  // cbvDesc.BufferLocation = cbAddress;
  // cbvDesc.SizeInBytes    = mByteSize;
  // device->CreateConstantBufferView(&cbvDesc, handle);

  mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mData));
  return S_OK;
}

void ConstantBuffer::SetRawData(const CheString& varName, const Byte* data, uint32 size)
{
  if (size > mByteSize) {
    return;
  }
  // if varName incorrect,do not set.
  if (mVariableOffsetMap.find(varName) != mVariableOffsetMap.end()) {
    const uint32 offset = mVariableOffsetMap[varName];
    Byte* dataStart     = mData + offset;
    memcpy_s(dataStart, size, data, size);
  }
}

void ShaderInfo::Generate(ID3DBlob* shader, ID3D12Device* device)
{
  ComPtr<ID3D12ShaderReflection> shaderReflection;
  HRESULT hr = D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), __uuidof(ID3D12ShaderReflection),
                          reinterpret_cast<void**>(shaderReflection.GetAddressOf()));

  D3D12_SHADER_DESC shaderDesc;
  shaderReflection->GetDesc(&shaderDesc);
  ShaderFlag shaderFlag = static_cast<ShaderFlag>(1 << D3D12_SHVER_GET_TYPE(shaderDesc.Version));

  if (FAILED(hr)) {
    logger.Error(CTEXT("Generate shader info error"));
    TIFF(hr);
  }

  for (uint32 i = 0;; ++i) {
    D3D12_SHADER_INPUT_BIND_DESC shaderInputDesc;
    hr = shaderReflection->GetResourceBindingDesc(i, &shaderInputDesc);

    // It wiil fail after reading, but it's not a failed call.
    if (FAILED(hr)) break;

    // Process construct buffer build.
    if (shaderInputDesc.Type == D3D_SIT_CBUFFER) {
      CreateConstantBuffer(device, shaderInputDesc, shaderReflection->GetConstantBufferByName(shaderInputDesc.Name));
    }
    if (shaderInputDesc.Type == D3D_SIT_SAMPLER) {
      CD3DX12_STATIC_SAMPLER_DESC linearWrap(shaderInputDesc.BindPoint,         // shaderRegister
                                             D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
                                             D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
                                             D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
                                             D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW
      mSamplers[ConvertToCheString(shaderInputDesc.Name)] = linearWrap;
    }
    if (shaderInputDesc.Type == D3D_SIT_TEXTURE) {
      Texture2D texture = {shaderInputDesc.BindPoint};

      mTextures[ConvertToCheString(shaderInputDesc.Name)] = texture;
    }
  }
}

void ShaderInfo::CreateConstantBuffer(ID3D12Device* device, D3D12_SHADER_INPUT_BIND_DESC bindDesc,
                                      ID3D12ShaderReflectionConstantBuffer* cbReflection)
{
  // Get the varible info in the cbuffer and create the mapping.
  D3D12_SHADER_BUFFER_DESC cbufferDescs{};
  HRESULT hr = cbReflection->GetDesc(&cbufferDescs);
  if (FAILED(hr)) {
    logger.Error(CTEXT("Can't get shader info"));
    TIFF(hr);
  }

  CheString cbufferName = ConvertToCheString(bindDesc.Name);

  auto iter = mCBuffers.find(cbufferName);
  // If it doesn't exist, cteate it.
  if (iter == mCBuffers.end()) {
    mCBuffers.emplace(std::make_pair(cbufferName, ConstantBuffer(cbufferName, bindDesc.BindPoint, cbufferDescs.Size)));
    mCBuffers[cbufferName].Create(device);

    // Generate varible offsets
    for (uint32 j = 0; j < cbufferDescs.Variables; ++j) {
      ID3D12ShaderReflectionVariable* cbufferVar = cbReflection->GetVariableByIndex(j);
      D3D12_SHADER_VARIABLE_DESC varibleDesc;
      TIFF(cbufferVar->GetDesc(&varibleDesc));
      mCBuffers[cbufferName].SetVariableOffset(ConvertToCheString(varibleDesc.Name), varibleDesc.StartOffset);
    }
  }
}

void ShaderInfo::CreateRootSignature(ID3D12Device* device)
{
  const uint32 paramterNo = mTextures.size() + mCBuffers.size();
  vector<CD3DX12_ROOT_PARAMETER> slotRootParameter(paramterNo);

  uint32 paramterIndex = 0;

  CD3DX12_DESCRIPTOR_RANGE texTable[2];
  for (auto iter : mTextures) {
    auto texture = iter.second;
    texTable[paramterIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, texture.slot);
    slotRootParameter[paramterIndex].InitAsDescriptorTable(1, &texTable[paramterIndex], D3D12_SHADER_VISIBILITY_PIXEL);
    paramterIndex++;
  }

  for (uint32 i = 0; i < mCBuffers.size(); i++, paramterIndex++) {
    slotRootParameter[paramterIndex].InitAsConstantBufferView(i);
  }

  auto staticSampler = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(slotRootParameter.size(), slotRootParameter.data(), staticSampler.size(),
                                          staticSampler.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob         = nullptr;
  TIFF(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(),
                                   errorBlob.GetAddressOf()));

  if (errorBlob != nullptr) {
    logger.Error(ConvertToCheString((char*)errorBlob->GetBufferPointer()));
  }

  TIFF(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                   IID_PPV_ARGS(mRootSignature.GetAddressOf())));
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

void ShaderInfo::SetMaterial(const CheString& varName, const Material& material)
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