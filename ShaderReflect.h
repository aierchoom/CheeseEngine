#pragma once
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include <xstring>
#include <vector>

#include "../../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;
using namespace std;

enum ShaderFlag {
  PixelShader    = 0x1,
  VertexShader   = 0x2,
  GeometryShader = 0x4,
  HullShader     = 0x8,
  DomainShader   = 0x10,
  ComputeShader  = 0x20,
};

using Byte = uint8_t;

struct CBufferData {
  bool isDirty = false;
  ComPtr<ID3D12Resource> uploadBuffer;
  ComPtr<ID3D12DescriptorHeap> cbvHeap;
  vector<Byte> data;
  string name;
  uint32_t startSlot = 0;

  CBufferData() = default;
  CBufferData(const std::string& name, uint32_t startSlot, uint32_t ByteWidth, Byte* initData = nullptr) : data(ByteWidth), name(name), startSlot(startSlot)
  {
    if (initData) memcpy_s(data.data(), ByteWidth, initData, ByteWidth);
  }

  HRESULT CreateBuffer(ID3D12Device* device)
  {
    if (uploadBuffer != nullptr) return S_OK;

    CD3DX12_HEAP_PROPERTIES heapProps  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(data.size());

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = uploadBuffer->GetGPUVirtualAddress();

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask       = 0;
    device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes    = data.size();
    device->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
    return S_OK;
  }

  void UpdateBuffer() { uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(data.data())); }
};

class ShaderReflect
{
 public:
  ShaderReflect(ID3DBlob* shaderByteCode, ID3D12Device* device)
  {
    ComPtr<ID3D12ShaderReflection> shaderReflection;

    HRESULT hr = D3DReflect(shaderByteCode->GetBufferPointer(), shaderByteCode->GetBufferSize(), __uuidof(ID3D12ShaderReflection),
                            reinterpret_cast<void**>(shaderReflection.GetAddressOf()));

    D3D12_SHADER_DESC shaderDesc;
    shaderReflection->GetDesc(&shaderDesc);
    ShaderFlag shaderFlag = static_cast<ShaderFlag>(1 << D3D12_SHVER_GET_TYPE(shaderDesc.Version));

    if (FAILED(hr)) {
      MessageBox(NULL, L"error", L"Error", MB_OK);
    }

    for (uint32_t i = 0;; ++i) {
      D3D12_SHADER_INPUT_BIND_DESC shaderInputDesc;
      hr = shaderReflection->GetResourceBindingDesc(i, &shaderInputDesc);

      // It will fail after reading, but it's not a failed call.
      if (FAILED(hr)) break;

      // process construct buffer build
      if (shaderInputDesc.Type == D3D_SIT_CBUFFER) {
        ID3D12ShaderReflectionConstantBuffer* srcCBuffer = shaderReflection->GetConstantBufferByName(shaderInputDesc.Name);

        // Get the variable info in the cbuffer and create the mapping.
        D3D12_SHADER_BUFFER_DESC cbufferDescs{};
        hr = srcCBuffer->GetDesc(&cbufferDescs);
        if (FAILED(hr)) MessageBox(NULL, L"fail", L"error", MB_OK);

        bool isParam = !strcmp(shaderInputDesc.Name, "$Params");

        if (!isParam) {
          auto iter = mCBuffers.find(shaderInputDesc.BindPoint);

          // If it doesn't exist, create it.
          if (iter == mCBuffers.end()) {
            mCBuffers.emplace(make_pair(shaderInputDesc.BindPoint, CBufferData(shaderInputDesc.Name, shaderInputDesc.BindPoint, cbufferDescs.Size, nullptr)));
            mCBuffers[shaderInputDesc.BindPoint].CreateBuffer(device);
          }

          for (uint32_t j = 0; j < cbufferDescs.Variables; ++j) {
            ID3D12ShaderReflectionVariable* srVar = srcCBuffer->GetVariableByIndex(j);
            D3D12_SHADER_VARIABLE_DESC svDesc;
            HRESULT hr = srVar->GetDesc(&svDesc);
            if (FAILED(hr)) MessageBox(NULL, L"fail", L"error", MB_OK);
            mBufferOffset[svDesc.Name] = svDesc.StartOffset;
          }
        }
      }
    }
  }

 public:
  unordered_map<uint32_t, CBufferData> mCBuffers;
  unordered_map<string, uint32_t> mBufferOffset;
};
