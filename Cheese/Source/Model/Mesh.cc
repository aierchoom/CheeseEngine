#include "Mesh.h"

#include <d3dcompiler.h>
#include "Graphics/D3DUtil.h"

using namespace std;

std::vector<D3D12_INPUT_ELEMENT_DESC> Vertex::InputLayout = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

IMesh::IMesh() : IMesh(std::vector<Vertex>()) {}

IMesh::IMesh(const std::vector<Vertex>& vertices) : mVertices(vertices) {}
IMesh::IMesh(std::vector<Vertex>&& vertices) : mVertices(std::move(vertices)) {}

void IMesh::CreateGPUResource(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                              const unordered_map<CheString, SRVInfo>& srvSettings)
{
  const uint32 vbByteSize = GetVertexByteSize();
  const uint32 ibByteSize = GetIndexByteSize();

  TIFF(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
  CopyMemory(mVertexBufferCPU->GetBufferPointer(), mVertices.data(), vbByteSize);

  TIFF(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
  CopyMemory(mIndexBufferCPU->GetBufferPointer(), GetIndexByteData(), ibByteSize);

  mIndexBufferGPU  = D3DUtil::CreateDefaultBuffer(device, cmdList, GetIndexByteData(), ibByteSize, mIndexBufferUploader);
  mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, cmdList, mVertices.data(), vbByteSize, mVertexBufferUploader);

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  srvHeapDesc.NumDescriptors             = srvSettings.size();
  srvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  TIFF(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

  for (auto pair : srvSettings) {
    auto srvName    = pair.first;
    auto srvSetting = pair.second;
    if (mMaterial.Textures.find(srvName) == mMaterial.Textures.end()) break;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    uint32 srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    srvDescriptor.Offset(srvSetting.GetSlot(), srvDescriptorSize);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format                          = mMaterial.Textures[srvName].Resource->GetDesc().Format;
    srvDesc.ViewDimension                   = srvSetting.GetDimension();
    srvDesc.Texture2D.MostDetailedMip       = 0;
    srvDesc.Texture2D.MipLevels             = mMaterial.Textures[srvName].Resource->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
    device->CreateShaderResourceView(mMaterial.Textures[srvName].Resource.Get(), &srvDesc, srvDescriptor);
  }
}

D3D12_INDEX_BUFFER_VIEW IMesh::GetIndexBufferView() const
{
  D3D12_INDEX_BUFFER_VIEW ibv;
  ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
  ibv.Format         = GetIndexFormat();
  ibv.SizeInBytes    = GetIndexByteSize();
  return ibv;
}

D3D12_VERTEX_BUFFER_VIEW IMesh::GetVertexBufferView() const
{
  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
  vbv.StrideInBytes  = sizeof(Vertex);
  vbv.SizeInBytes    = GetVertexByteSize();
  return vbv;
}
