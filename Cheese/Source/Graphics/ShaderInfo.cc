#include "ShaderInfo.h"
#include <strsafe.h>
#include "Core/CoreMinimal.h"

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

void ShaderInfo::Create(ID3DBlob* shader, ID3D12Device* device)
{
  ComPtr<ID3D12ShaderReflection> shaderReflection;
  HRESULT hr = D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), __uuidof(ID3D12ShaderReflection),
                          reinterpret_cast<void**>(shaderReflection.GetAddressOf()));

  D3D12_SHADER_DESC shaderDesc;
  shaderReflection->GetDesc(&shaderDesc);
  ShaderFlag shaderFlag = static_cast<ShaderFlag>(1 << D3D12_SHVER_GET_TYPE(shaderDesc.Version));

  if (FAILED(hr)) {
    MessageBox(NULL, CTEXT("error"), CTEXT("error"), MB_OK);
  }

  CreateRootSignature(device, shaderDesc.ConstantBuffers);
  CreateCbvHeapDescriptor(device, shaderDesc.ConstantBuffers);

  for (uint32 i = 0;; ++i) {
    D3D12_SHADER_INPUT_BIND_DESC shaderInputDesc;
    hr = shaderReflection->GetResourceBindingDesc(i, &shaderInputDesc);

    // It wiil fail after reading, but it's not a failed call.
    if (FAILED(hr)) break;

    // Process construct buffer build.
    if (shaderInputDesc.Type == D3D_SIT_CBUFFER) {
      ID3D12ShaderReflectionConstantBuffer* srcCBuffer = shaderReflection->GetConstantBufferByName(shaderInputDesc.Name);

      // Get the varible info in the cbuffer and create the mapping.
      D3D12_SHADER_BUFFER_DESC cbufferDescs{};
      hr = srcCBuffer->GetDesc(&cbufferDescs);
      if (FAILED(hr)) MessageBox(NULL, CTEXT("error"), CTEXT("error"), MB_OK);

      bool isParam = !strcmp(shaderInputDesc.Name, "$Params");

      if (!isParam) {
        CheString cbufferName = ConvertToCheString(shaderInputDesc.Name);

        auto iter = mCBuffers.find(cbufferName);

        // If it doesn't exist, cteate it.
        if (iter == mCBuffers.end()) {
          mCBuffers.emplace(
              std::make_pair(cbufferName, ConstantBuffer(cbufferName, shaderInputDesc.BindPoint, cbufferDescs.Size)));
          mCBuffers[cbufferName].Create(device);

          // Generate varible offsets
          for (uint32 j = 0; j < cbufferDescs.Variables; ++j) {
            ID3D12ShaderReflectionVariable* cbufferVar = srcCBuffer->GetVariableByIndex(j);
            D3D12_SHADER_VARIABLE_DESC varibleDesc;
            TIFF(cbufferVar->GetDesc(&varibleDesc));
            mCBuffers[cbufferName].SetVariableOffset(ConvertToCheString(varibleDesc.Name), varibleDesc.StartOffset);
          }
        }
      }
    }
  }
}

void ShaderInfo::CreateCbvHeapDescriptor(ID3D12Device* device, uint32 descriptorNo)
{
  if (descriptorNo == 0) {
    return;
  }
  D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
  cbvHeapDesc.NumDescriptors = descriptorNo;
  cbvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cbvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbvHeapDesc.NodeMask       = 0;
  device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
}

void ShaderInfo::CreateRootSignature(ID3D12Device* device, uint32 cbufferSize)
{
  // vector<CD3DX12_ROOT_PARAMETER> slotRootParamter(cbufferSize);
  // vector<CD3DX12_DESCRIPTOR_RANGE> cbvTable(cbufferSize);
  // for (uint32 i = 0; i < cbufferSize; i++) {
  //   cbvTable[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i);
  //   slotRootParamter[i].InitAsDescriptorTable(1, &cbvTable[i]);
  // }

  // CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(cbufferSize, slotRootParamter.data(), 0, nullptr,
  //                                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  // ComPtr<ID3DBlob> serializedRootSig = nullptr;
  // ComPtr<ID3DBlob> errorBlob         = nullptr;
  // HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(),
  //                                          errorBlob.GetAddressOf());

  // if (errorBlob != nullptr) {
  //   ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
  // }
  // TIFF(hr);

  // TIFF(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
  //                                  IID_PPV_ARGS(&mRootSignature)))
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