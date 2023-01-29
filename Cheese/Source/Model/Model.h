#ifndef MODEL_MODEL_H
#define MODEL_MODEL_H
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

#include "Common/TypeDef.h"
#include "Vertex.h"
using namespace DirectX;

struct MeshData {
  std::vector<VertexPosNormalTangentTex> vertexVec;
  std::vector<uint16> indexVec;
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
