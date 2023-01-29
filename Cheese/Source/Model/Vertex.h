#ifndef MODEL_VERTEX_H
#define MODEL_VERTEX_H
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
using namespace DirectX;

struct VertexPosNormalTangentTex {
  XMFLOAT3 pos;
  XMFLOAT3 normal;
  XMFLOAT3 tangent;
  XMFLOAT2 tex;
  static std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
};

#endif  // MODEL_VERTEX_H
