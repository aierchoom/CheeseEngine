#ifndef GRAPHICS_RENDER_DATA_H
#define GRAPHICS_RENDER_DATA_H
#include "Common/TypeDef.h"
#include <unordered_map>
#include <vector>
#include <d3d12.h>
#include "Graphics/D3DUtil.h"
#include "Model/Model.h"
#include "Shader/ConstantBuffer.h"

struct DrawMaterial {
  uint32 SrvIndex;
  D3D12_SRV_DIMENSION Dimension;
  ComPtr<ID3D12Resource> Resource;
};

struct DrawArg {
  uint32 IndexCount;
  uint32 StartIndexLocation;
  DXGI_FORMAT IndexFormat;

  uint32 BaseVertexLocation;

  bool IsBlend;

  std::unordered_map<CheString, DrawMaterial> DrawSrvs;
};

class RenderItem
{
 public:
  RenderItem()                       = default;
  RenderItem(const RenderItem&)      = default;
  RenderItem(RenderItem&&) noexcept  = default;
  RenderItem& operator=(RenderItem&) = default;

  RenderItem(const Model* model, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

  void BuildPerObjectCBuffer(ID3D12Device* device, const CheString& shaderName, std::unordered_map<CheString, CBufferInfo> cbSettings);

  D3D12_INDEX_BUFFER_VIEW GetIndexBufferView16() const;
  D3D12_INDEX_BUFFER_VIEW GetIndexBufferView32() const;
  D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

  inline const std::vector<DrawArg>& GetDrawArgs() const { return mDrawArgs; }

  inline void SetPosition(float x, float y, float z) { mTransform.SetPosition(x, y, z); }
  inline void SetScale(float x, float y, float z) { mTransform.SetScale(x, y, z); }
  inline void SetRotation(float x, float y, float z) { mTransform.SetRotation(x, y, z); }
  inline DirectX::XMMATRIX GetTransMatrix() { return mTransform.GetLocalToWorldMatrixXM(); }

  inline CBufferManager& GetPerObjectCBuffer(const CheString& shaderName) { return mPerObjectCBManagers[shaderName]; }
  inline uint32 GetSrvDescriptorOffset() const { return mSrvDescriptorOffset; }
  inline uint32 GetSrvDescriptorCount() const { return mTotalSrvDescriptorCount; }

  inline void SetSrvDescriptorOffset(uint32 offset) { mSrvDescriptorOffset = offset; }

 private:
  inline void BuildDrawArgs(const Model* model);
  inline void BuildMeshUploadResource(const Model* model, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

 private:
  uint32 mTotalVertexCount        = 0;
  uint32 mTotalIndexCount16       = 0;
  uint32 mTotalIndexCount32       = 0;
  uint32 mTotalSrvDescriptorCount = 0;
  uint32 mSrvDescriptorOffset     = 0;

  Transform mTransform;

  // Shader name: cbuffer manager.
  std::unordered_map<CheString, CBufferManager> mPerObjectCBManagers;
  std::vector<DrawArg> mDrawArgs;

  ComPtr<ID3D12Resource> mIndexBufferGPU16 = nullptr;
  ComPtr<ID3D12Resource> mIndexBufferGPU32 = nullptr;
  ComPtr<ID3D12Resource> mVertexBufferGPU  = nullptr;

  ComPtr<ID3D12Resource> mIndexBufferUploader16 = nullptr;
  ComPtr<ID3D12Resource> mIndexBufferUploader32 = nullptr;
  ComPtr<ID3D12Resource> mVertexBufferUploader  = nullptr;
};

class RenderData
{
 public:
  RenderData(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList);

  void AddShader(Shader* shader) { mShaders.push_back(shader); }
  void AddRenderItem(const CheString& name, Model* model);

  uint32 GetTotalDescriptorCount();

  void BuildRenderData();

  inline RenderItem& GetItem(const CheString& itemName) { return mRenderItems[itemName]; }
  inline std::unordered_map<CheString, RenderItem>& GetRenderItems() { return mRenderItems; }
  inline ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return mSrvDescriptorHeap.Get(); }
  inline uint32 GetNullSrvIndex() const { return mNullSrvIndex; }

  inline CBufferManager& GetItemPerObjectCB(const CheString& itemName, const CheString& shaderName)
  {
    return mRenderItems[itemName].GetPerObjectCBuffer(shaderName);
  }

  inline void SetCBValueWithItemTrans(const CheString& itemName, const CheString& shaderName, const CheString& varName)
  {
    mRenderItems[itemName].GetPerObjectCBuffer(shaderName).SetValue(varName, XMMatrixTranspose(mRenderItems[itemName].GetTransMatrix()));
  }

  inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetShadowMapHandleCPU() const
  {
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(mShadowMapSrvIndex, mSrvDescriptorSize);
    return handle;
  }

  inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetShadowMapHandleGPU() const
  {
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    handle.Offset(mShadowMapSrvIndex, mSrvDescriptorSize);
    return handle;
  }

 public:
  static const uint32 NULL_SRV_WIDTH  = 4;
  static const uint32 NULL_SRV_HEIGHT = 4;

 private:
  void BuildNullSrvResource();

 private:
  ComPtr<ID3D12Device> mDevice;
  ComPtr<ID3D12GraphicsCommandList> mCmdList;

  std::vector<Shader*> mShaders;
  std::unordered_map<CheString, RenderItem> mRenderItems;

  ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
  ComPtr<ID3D12Resource> mNullResource            = nullptr;

  uint32 mSrvDescriptorSize  = 0;
  uint32 mSrvDescriptorCount = 0;

  const uint32 mNullSrvIndex      = 0;
  const uint32 mShadowMapSrvIndex = 1;

  // Shadow Map Srv + NullSrv
  const uint32 mDescriptorOffset = 2;
};
#endif  // GRAPHICS_RENDER_DATA_H