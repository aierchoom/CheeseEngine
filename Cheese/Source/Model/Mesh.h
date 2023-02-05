#ifndef MODEL_MESH_H
#define MODEL_MESH_H
#include "Common/TypeDef.h"

#include <vector>
#include <unordered_map>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Shader/ShaderHelper.h"
#include "Utils/Log/Logger.h"
#include "Model/Texture2D.h"

struct Vertex {
  DirectX::XMFLOAT3 Position;
  DirectX::XMFLOAT3 Normal;
  DirectX::XMFLOAT4 Tangent;
  DirectX::XMFLOAT2 TexCoord;
  static std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
};

struct Material {
  std::unordered_map<CheString, Texture2D> Textures;
};

// Key points of design
// 1.Mesh要存储所有的GPU资源
// 2.思考是否应该存储CPU资源
// 3.尽量保持操作一致性原则
class IMesh
{
 public:
  IMesh();
  IMesh(const std::vector<Vertex>& vertices);
  IMesh(std::vector<Vertex>&& vertices);
  inline virtual ~IMesh() {}

  inline Material GetMaterial() const { return mMaterial; }
  inline void SetMaterial(const Material& material) { mMaterial = material; }

  inline uint32 GetVertexByteSize() const { return mVertices.size() * sizeof(Vertex); }

  inline virtual uint32 GetIndexCount() const       = 0;
  inline virtual uint32 GetIndexByteSize() const    = 0;
  inline virtual DXGI_FORMAT GetIndexFormat() const = 0;

  void CreateGPUResource(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                         const std::unordered_map<CheString, SRVInfo>& srvSettings);

  ID3D12DescriptorHeap* GetSrvDescriptor() const { return mSrvDescriptorHeap.Get(); }

  D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
  D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

 protected:
  inline virtual Byte* GetIndexByteData() const = 0;

 protected:
  ComPtr<ID3DBlob> mIndexBufferCPU       = nullptr;
  ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

  std::vector<Vertex> mVertices;
  ComPtr<ID3DBlob> mVertexBufferCPU       = nullptr;
  ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;

  ComPtr<ID3D12Resource> mIndexBufferUploader  = nullptr;
  ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;

  ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

  Material mMaterial;
};

// The Mesh class supports different index lengths.
template <typename IndexType>
class Mesh : public IMesh
{
 public:
  Mesh() : IMesh(), mIndices(0) {}
  Mesh(const std::vector<Vertex>& vertices, const std::vector<IndexType>& indices) : IMesh(vertices), mIndices(indices) {}
  Mesh(std::vector<Vertex>&& vertices, std::vector<IndexType>&& indices)
      : IMesh(std::move(vertices)), mIndices(std::move(indices))
  {
  }

  inline virtual uint32 GetIndexCount() const override { return mIndices.size(); }
  inline virtual Byte* GetIndexByteData() const override { return (Byte*)mIndices.data(); }
  inline virtual DXGI_FORMAT GetIndexFormat() const override
  {
    if (sizeof(IndexType) == 2) {
      return DXGI_FORMAT_R16_UINT;
    } else {
      return DXGI_FORMAT_R32_UINT;
    }
  }

 private:
  inline virtual uint32 GetIndexByteSize() const override { return mIndices.size() * sizeof(IndexType); }

 private:
  std::vector<IndexType> mIndices;
};

#endif  // MODEL_MESH_H
