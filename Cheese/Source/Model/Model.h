#ifndef MODEL_MODEL_H
#define MODEL_MODEL_H
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

#include "Common/TypeDef.h"

using namespace DirectX;

struct VertexPosNormalTex {
  XMFLOAT3 pos;
  XMFLOAT3 normal;
  XMFLOAT2 tex;
  static std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
};

std::vector<D3D12_INPUT_ELEMENT_DESC> VertexPosNormalTex::inputLayout = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

template <class VertexType = VertexPosNormalTex, class IndexType = uint16>
struct MeshData {
  std::vector<VertexType> vertexVec;
  std::vector<IndexType> indexVec;
  MeshData()
  {
    static_assert(sizeof(IndexType) == 2 || sizeof(IndexType) == 4, "The size of IndexType must be 2 bytes or 4 bytes!");
    static_assert(std::is_unsigned<IndexType>::value, "IndexType must be unsigned integer!");
  }
};

// 设计作为模型总类的接口，与d3d12进行对接，暴露资源给d3d12。
class Model
{
 public:
  Model() = default;

  virtual ID3D12Resource* GetVertexBufferGPU() = 0;
  virtual ID3D12Resource* GetIndexBufferGPU()  = 0;

 protected:
  ComPtr<ID3DBlob> mVertexBufferCPU;
  ComPtr<ID3DBlob> mIndexBufferCPU;

  ComPtr<ID3D12Resource> mVertexBufferGPU;
  ComPtr<ID3D12Resource> mIndexBufferGPU;
};
#endif  // MODEL_MODEL_H
