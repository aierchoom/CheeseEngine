#include "Geometry.h"
#include <DirectXMath.h>

using namespace DirectX;

IMesh* Geometry::GenerateBox(uint32 width, uint32 height, uint32 depth)
{
  std::vector<Vertex> vertices(24);
  float w2 = width / 2.0f, h2 = height / 2.0f, d2 = depth / 2.0f;

  // right(+X face)
  vertices[0].Position = XMFLOAT3(w2, -h2, -d2);
  vertices[1].Position = XMFLOAT3(w2, h2, -d2);
  vertices[2].Position = XMFLOAT3(w2, h2, d2);
  vertices[3].Position = XMFLOAT3(w2, -h2, d2);
  // left(-X face)
  vertices[4].Position = XMFLOAT3(-w2, -h2, d2);
  vertices[5].Position = XMFLOAT3(-w2, h2, d2);
  vertices[6].Position = XMFLOAT3(-w2, h2, -d2);
  vertices[7].Position = XMFLOAT3(-w2, -h2, -d2);
  // top(+Y face)
  vertices[8].Position  = XMFLOAT3(-w2, h2, -d2);
  vertices[9].Position  = XMFLOAT3(-w2, h2, d2);
  vertices[10].Position = XMFLOAT3(w2, h2, d2);
  vertices[11].Position = XMFLOAT3(w2, h2, -d2);
  // bottom(-Y face)
  vertices[12].Position = XMFLOAT3(w2, -h2, -d2);
  vertices[13].Position = XMFLOAT3(w2, -h2, d2);
  vertices[14].Position = XMFLOAT3(-w2, -h2, d2);
  vertices[15].Position = XMFLOAT3(-w2, -h2, -d2);
  // back(+Z face)
  vertices[16].Position = XMFLOAT3(w2, -h2, d2);
  vertices[17].Position = XMFLOAT3(w2, h2, d2);
  vertices[18].Position = XMFLOAT3(-w2, h2, d2);
  vertices[19].Position = XMFLOAT3(-w2, -h2, d2);
  // front(-Z face)
  vertices[20].Position = XMFLOAT3(-w2, -h2, -d2);
  vertices[21].Position = XMFLOAT3(-w2, h2, -d2);
  vertices[22].Position = XMFLOAT3(w2, h2, -d2);
  vertices[23].Position = XMFLOAT3(w2, -h2, -d2);

  for (UINT i = 0; i < 4; ++i) {
    // right(+X face)
    vertices[i].Normal  = XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertices[i].Tangent = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    // left(-X face)
    vertices[i + 4].Normal  = XMFLOAT3(-1.0f, 0.0f, 0.0f);
    vertices[i + 4].Tangent = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
    // top(+Y face)
    vertices[i + 8].Normal  = XMFLOAT3(0.0f, 1.0f, 0.0f);
    vertices[i + 8].Tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    // bottom(-Y face)
    vertices[i + 12].Normal  = XMFLOAT3(0.0f, -1.0f, 0.0f);
    vertices[i + 12].Tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
    // back(+Z face)
    vertices[i + 16].Normal  = XMFLOAT3(0.0f, 0.0f, 1.0f);
    vertices[i + 16].Tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
    // front(-Z face)
    vertices[i + 20].Normal  = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertices[i + 20].Tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
  }

  for (UINT i = 0; i < 6; ++i) {
    vertices[i * 4].TexCoord     = XMFLOAT2(0.0f, 1.0f);
    vertices[i * 4 + 1].TexCoord = XMFLOAT2(0.0f, 0.0f);
    vertices[i * 4 + 2].TexCoord = XMFLOAT2(1.0f, 0.0f);
    vertices[i * 4 + 3].TexCoord = XMFLOAT2(1.0f, 1.0f);
  }

  std::vector<uint16> indices = {
      0,  1,  2,  2,  3,  0,   // right(+X face)
      4,  5,  6,  6,  7,  4,   // left(-X face)
      8,  9,  10, 10, 11, 8,   // top(+Y face)
      12, 13, 14, 14, 15, 12,  // bottom(-Y face)
      16, 17, 18, 18, 19, 16,  // back(+Z face)
      20, 21, 22, 22, 23, 20   // front(-Z face)
  };

  Mesh<uint16>* mesh = new Mesh<uint16>(std::move(vertices), std::move(indices));
  return mesh;
}