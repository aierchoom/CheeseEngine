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

// �����Ϊģ������Ľӿڣ���d3d12���жԽӣ���¶��Դ��d3d12��
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
