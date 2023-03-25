#include "RenderData.h"

RenderItem::RenderItem(const Model* model, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
    : mPerObjectCBManagers(), mDrawArgs(model->GetMeshes().size())
{
  BuildDrawArgs(model);
  BuildMeshUploadResource(model, device, cmdList);
}

void RenderItem::BuildPerObjectCBuffer(ID3D12Device* device, const CheString& shaderName,
                                       std::unordered_map<CheString, CBufferInfo> cbSettings)
{
  for (auto pair : cbSettings) {
    auto cbName = pair.first;
    auto cbInfo = pair.second;
    // RenderItem just save tag:PEROBJECT data.
    if (CBufferManager::CBufferConfig[cbName] == CBufferType::PEROBJECT) {
      mPerObjectCBManagers[shaderName].AddCBuffer(device, cbName, cbInfo);
    }
  }
}

D3D12_INDEX_BUFFER_VIEW RenderItem::GetIndexBufferView16() const
{
  D3D12_INDEX_BUFFER_VIEW ibv;
  if (mIndexBufferGPU16 == nullptr) {
    ZeroMemory(&ibv, sizeof(D3D12_INDEX_BUFFER_VIEW));
    return ibv;
  }
  ibv.BufferLocation = mIndexBufferGPU16->GetGPUVirtualAddress();
  ibv.Format         = DXGI_FORMAT_R16_UINT;
  ibv.SizeInBytes    = static_cast<UINT>(mTotalIndexCount16 * sizeof(uint16));
  return ibv;
}

D3D12_INDEX_BUFFER_VIEW RenderItem::GetIndexBufferView32() const
{
  D3D12_INDEX_BUFFER_VIEW ibv;
  if (mIndexBufferGPU32 == nullptr) {
    ZeroMemory(&ibv, sizeof(D3D12_INDEX_BUFFER_VIEW));
    return ibv;
  }
  ibv.BufferLocation = mIndexBufferGPU32->GetGPUVirtualAddress();
  ibv.Format         = DXGI_FORMAT_R32_UINT;
  ibv.SizeInBytes    = static_cast<UINT>(mTotalIndexCount16 * sizeof(uint32));
  return ibv;
}

D3D12_VERTEX_BUFFER_VIEW RenderItem::GetVertexBufferView() const
{
  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
  vbv.StrideInBytes  = sizeof(Vertex);
  vbv.SizeInBytes    = static_cast<UINT>(mTotalVertexCount * sizeof(Vertex));
  return vbv;
}

void RenderItem::BuildDrawArgs(const Model* model)
{
  mTotalVertexCount        = 0;
  mTotalIndexCount16       = 0;
  mTotalIndexCount32       = 0;
  mTotalSrvDescriptorCount = 0;

  // Record total mesh info.
  for (uint32 i = 0; i < model->GetMeshes().size(); i++) {
    const IMesh* mesh = model->GetMeshes()[i];

    mDrawArgs[i].IsBlend = mesh->GetBlend();

    // Record render desc.
    mDrawArgs[i].BaseVertexLocation = mTotalVertexCount;
    mDrawArgs[i].IndexCount         = mesh->GetIndexCount();
    if (mesh->GetIndexFormat() == DXGI_FORMAT_R16_UINT) {
      mDrawArgs[i].IndexFormat        = DXGI_FORMAT_R16_UINT;
      mDrawArgs[i].StartIndexLocation = mTotalIndexCount16;
      mTotalIndexCount16 += mesh->GetIndexCount();
    } else {
      mDrawArgs[i].IndexFormat        = DXGI_FORMAT_R32_UINT;
      mDrawArgs[i].StartIndexLocation = mTotalIndexCount32;
      mTotalIndexCount32 += mesh->GetIndexCount();
    }

    mTotalVertexCount += mesh->GetVertexCount();

    Material& material = mesh->GetMaterial();
    for (auto pair : material.Textures) {
      auto texName = pair.first;
      auto texture = pair.second;

      mDrawArgs[i].DrawSrvs[texName].SrvIndex  = mTotalSrvDescriptorCount;
      mDrawArgs[i].DrawSrvs[texName].Dimension = texture.Dimension;
      mDrawArgs[i].DrawSrvs[texName].Resource  = texture.Resource;
      mTotalSrvDescriptorCount++;
    }
  }
}

void RenderItem::BuildMeshUploadResource(const Model* model, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
  std::vector<Vertex> totalVertices(mTotalVertexCount);
  std::vector<uint16> totalIndices16(mTotalIndexCount16);
  std::vector<uint32> totalIndices32(mTotalIndexCount32);

  // Copy vertices&indices data.
  uint32 copyVertexOffset  = 0;
  uint32 copyIndexOffset16 = 0;
  uint32 copyIndexOffset32 = 0;

  for (uint32 i = 0; i < model->GetMeshes().size(); i++) {
    const IMesh* mesh     = model->GetMeshes()[i];
    const uint32 copySize = mesh->GetVertexByteSize();
    Byte* copyTarget      = reinterpret_cast<Byte*>(totalVertices.data());
    memcpy_s(copyTarget + copyVertexOffset, copySize, mesh->GetVertexByteData(), copySize);
    copyVertexOffset += mesh->GetVertexByteSize();

    if (mesh->GetIndexFormat() == DXGI_FORMAT_R16_UINT) {
      const uint32 copySize = mesh->GetIndexByteSize();
      Byte* copyTarget      = reinterpret_cast<Byte*>(totalIndices16.data());
      memcpy_s(copyTarget + copyIndexOffset16, copySize, mesh->GetIndexByteData(), copySize);
      copyIndexOffset16 += mesh->GetIndexByteSize();
    } else {
      const uint32 copySize = mesh->GetIndexByteSize();
      Byte* copyTarget      = reinterpret_cast<Byte*>(totalIndices32.data());
      memcpy_s(copyTarget + copyIndexOffset32, copySize, mesh->GetIndexByteData(), copySize);
      copyIndexOffset32 += mesh->GetIndexByteSize();
    }
  }

  const uint32 vbByteSize   = static_cast<uint32>(totalVertices.size() * sizeof(Vertex));
  const uint32 ibByteSize16 = static_cast<uint32>(totalIndices16.size() * sizeof(uint16));
  const uint32 ibByteSize32 = static_cast<uint32>(totalIndices32.size() * sizeof(uint32));

  if (ibByteSize16 != 0) {
    mIndexBufferGPU16 = D3DUtil::CreateDefaultBuffer(device, cmdList, totalIndices16.data(), ibByteSize16, mIndexBufferUploader16);
  }
  if (ibByteSize32 != 0) {
    mIndexBufferGPU32 = D3DUtil::CreateDefaultBuffer(device, cmdList, totalIndices32.data(), ibByteSize32, mIndexBufferUploader32);
  }
  mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, cmdList, totalVertices.data(), vbByteSize, mVertexBufferUploader);
}

void RenderData::AddRenderItem(const CheString& name, Model* model)
{
  // Deal with the render item of the same name.
  if (mRenderItems.find(name) == mRenderItems.end()) {
    mRenderItems[name] = RenderItem(model, mDevice.Get(), mCmdList.Get());
    for (auto shader : mShaders) {
      mRenderItems[name].BuildPerObjectCBuffer(mDevice.Get(), shader->GetName(), shader->GetSettings().GetCBSetting());
    }
  }
}

uint32 RenderData::GetTotalDescriptorCount()
{
  uint32 totalCount = 0;
  for (auto pair : mRenderItems) {
    auto itemName    = pair.first;
    auto& renderItem = mRenderItems[itemName];
    totalCount += renderItem.GetSrvDescriptorCount();
  }
  return totalCount;
}

RenderData::RenderData(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList) : mDevice(device), mCmdList(cmdList)
{
  mSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  BuildNullSrvResource();
}

void RenderData::BuildNullSrvResource()
{
  D3D12_RESOURCE_DESC texDesc;
  ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
  texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texDesc.Alignment          = 0;
  texDesc.Width              = NULL_SRV_WIDTH;
  texDesc.Height             = NULL_SRV_HEIGHT;
  texDesc.DepthOrArraySize   = 1;
  texDesc.MipLevels          = 0;
  texDesc.Format             = DXGI_FORMAT_BC1_UNORM;
  texDesc.SampleDesc.Count   = 1;
  texDesc.SampleDesc.Quality = 0;
  texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  texDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

  TIFF(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &texDesc,
                                        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mNullResource)));
}

void RenderData::BuildRenderData()
{
  mSrvDescriptorCount = GetTotalDescriptorCount();

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  uint32 srvDescriptorCount              = mSrvDescriptorCount + mDescriptorOffset;
  srvHeapDesc.NumDescriptors             = srvDescriptorCount;
  srvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  TIFF(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format                          = mNullResource->GetDesc().Format;
  srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip       = 0;
  srvDesc.Texture2D.MipLevels             = mNullResource->GetDesc().MipLevels;
  srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;

  CD3DX12_CPU_DESCRIPTOR_HANDLE nullSrvHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mNullSrvIndex, mSrvDescriptorSize);
  mDevice->CreateShaderResourceView(mNullResource.Get(), &srvDesc, nullSrvHandle);

  uint32 renderItemSrvOffset = mDescriptorOffset;
  for (auto pair : mRenderItems) {
    auto itemName    = pair.first;
    auto& renderItem = mRenderItems[itemName];

    renderItem.SetSrvDescriptorOffset(renderItemSrvOffset);
    renderItemSrvOffset += renderItem.GetSrvDescriptorCount();

    for (const DrawArg& arg : renderItem.GetDrawArgs()) {
      for (auto pair : arg.DrawSrvs) {
        auto drawSrv = pair.second;

        uint32 srvDescriptorIndex = renderItemSrvOffset - renderItem.GetSrvDescriptorCount() + drawSrv.SrvIndex;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), srvDescriptorIndex,
                                                    mSrvDescriptorSize);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                          = drawSrv.Resource->GetDesc().Format;
        srvDesc.ViewDimension                   = drawSrv.Dimension;
        srvDesc.Texture2D.MostDetailedMip       = 0;
        srvDesc.Texture2D.MipLevels             = drawSrv.Resource->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
        mDevice->CreateShaderResourceView(drawSrv.Resource.Get(), &srvDesc, srvDescriptor);
      }
    }
  }
}