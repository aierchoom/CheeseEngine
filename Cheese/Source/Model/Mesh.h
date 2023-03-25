#ifndef MODEL_MESH_H
#define MODEL_MESH_H
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>

#include <unordered_map>
#include <vector>

#include "Common/TypeDef.h"
#include "Model/Texture2D.h"
#include "Shader/ShaderHelper.h"
#include "Utils/Log/Logger.h"

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

class IMesh
{
 public:
  IMesh();
  IMesh(const std::vector<Vertex>& vertices);
  IMesh(std::vector<Vertex>&& vertices);
  inline virtual ~IMesh() {}

  inline Material GetMaterial() const { return mMaterial; }
  inline void SetMaterial(const Material& material) { mMaterial = material; }
  inline bool GetBlend() const { return mIsBlend; }
  inline void SetBlend(bool isBlend) { mIsBlend = isBlend; }

  inline const Byte* GetVertexByteData() const { return reinterpret_cast<const Byte*>(mVertices.data()); }
  inline uint32 GetVertexCount() const { return static_cast<uint32>(mVertices.size()); }
  inline uint32 GetVertexByteSize() const { return static_cast<uint32>(mVertices.size() * sizeof(Vertex)); }

  inline virtual Byte* GetIndexByteData() const     = 0;
  inline virtual uint32 GetIndexCount() const       = 0;
  inline virtual uint32 GetIndexByteSize() const    = 0;
  inline virtual DXGI_FORMAT GetIndexFormat() const = 0;

 protected:
  std::vector<Vertex> mVertices;

  Material mMaterial;

  bool mIsBlend = false;
};

// The Mesh class supports different index lengths.
template <typename IndexType>
class Mesh : public IMesh
{
 public:
  Mesh() : IMesh(), mIndices(0) {}
  Mesh(const std::vector<Vertex>& vertices, const std::vector<IndexType>& indices) : IMesh(vertices), mIndices(indices) {}
  Mesh(std::vector<Vertex>&& vertices, std::vector<IndexType>&& indices) : IMesh(std::move(vertices)), mIndices(std::move(indices)) {}

  inline virtual uint32 GetIndexCount() const override { return static_cast<uint32>(mIndices.size()); }
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
  inline virtual uint32 GetIndexByteSize() const override { return static_cast<uint32>(mIndices.size() * sizeof(IndexType)); }

 private:
  std::vector<IndexType> mIndices;
};

#endif  // MODEL_MESH_H