#ifndef MODEL_BOX_H
#define MODEL_BOX_H
#include <map>
#include "Model.h"

struct VertexData {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT4 tangent;
  DirectX::XMFLOAT4 color;
  DirectX::XMFLOAT2 tex;
};

template <typename VertexType>
class Box : public Model
{
 public:
  Box(uint32 width, uint32 height, uint32 depth)
  {
    mMesh.vertexVec.resize(24);
    VertexData vertexDataArr[24];
    float w2 = width / 2, h2 = height / 2, d2 = depth / 2;

    XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

    // right(+X face)
    vertexDataArr[0].pos = XMFLOAT3(w2, -h2, -d2);
    vertexDataArr[1].pos = XMFLOAT3(w2, h2, -d2);
    vertexDataArr[2].pos = XMFLOAT3(w2, h2, d2);
    vertexDataArr[3].pos = XMFLOAT3(w2, -h2, d2);
    // left(-X face)
    vertexDataArr[4].pos = XMFLOAT3(-w2, -h2, d2);
    vertexDataArr[5].pos = XMFLOAT3(-w2, h2, d2);
    vertexDataArr[6].pos = XMFLOAT3(-w2, h2, -d2);
    vertexDataArr[7].pos = XMFLOAT3(-w2, -h2, -d2);
    // top(+Y face)
    vertexDataArr[8].pos  = XMFLOAT3(-w2, h2, -d2);
    vertexDataArr[9].pos  = XMFLOAT3(-w2, h2, d2);
    vertexDataArr[10].pos = XMFLOAT3(w2, h2, d2);
    vertexDataArr[11].pos = XMFLOAT3(w2, h2, -d2);
    // bottom(-Y face)
    vertexDataArr[12].pos = XMFLOAT3(w2, -h2, -d2);
    vertexDataArr[13].pos = XMFLOAT3(w2, -h2, d2);
    vertexDataArr[14].pos = XMFLOAT3(-w2, -h2, d2);
    vertexDataArr[15].pos = XMFLOAT3(-w2, -h2, -d2);
    // back(+Z face)
    vertexDataArr[16].pos = XMFLOAT3(w2, -h2, d2);
    vertexDataArr[17].pos = XMFLOAT3(w2, h2, d2);
    vertexDataArr[18].pos = XMFLOAT3(-w2, h2, d2);
    vertexDataArr[19].pos = XMFLOAT3(-w2, -h2, d2);
    // front(-Z face)
    vertexDataArr[20].pos = XMFLOAT3(-w2, -h2, -d2);
    vertexDataArr[21].pos = XMFLOAT3(-w2, h2, -d2);
    vertexDataArr[22].pos = XMFLOAT3(w2, h2, -d2);
    vertexDataArr[23].pos = XMFLOAT3(w2, -h2, -d2);

    for (UINT i = 0; i < 4; ++i) {
      // 右面(+X面)
      vertexDataArr[i].normal  = XMFLOAT3(1.0f, 0.0f, 0.0f);
      vertexDataArr[i].tangent = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
      vertexDataArr[i].color   = color;
      // 左面(-X面)
      vertexDataArr[i + 4].normal  = XMFLOAT3(-1.0f, 0.0f, 0.0f);
      vertexDataArr[i + 4].tangent = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
      vertexDataArr[i + 4].color   = color;
      // 顶面(+Y面)
      vertexDataArr[i + 8].normal  = XMFLOAT3(0.0f, 1.0f, 0.0f);
      vertexDataArr[i + 8].tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
      vertexDataArr[i + 8].color   = color;
      // 底面(-Y面)
      vertexDataArr[i + 12].normal  = XMFLOAT3(0.0f, -1.0f, 0.0f);
      vertexDataArr[i + 12].tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
      vertexDataArr[i + 12].color   = color;
      // 背面(+Z面)
      vertexDataArr[i + 16].normal  = XMFLOAT3(0.0f, 0.0f, 1.0f);
      vertexDataArr[i + 16].tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
      vertexDataArr[i + 16].color   = color;
      // 正面(-Z面)
      vertexDataArr[i + 20].normal  = XMFLOAT3(0.0f, 0.0f, -1.0f);
      vertexDataArr[i + 20].tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
      vertexDataArr[i + 20].color   = color;
    }

    for (UINT i = 0; i < 6; ++i) {
      vertexDataArr[i * 4].tex     = XMFLOAT2(0.0f, 1.0f);
      vertexDataArr[i * 4 + 1].tex = XMFLOAT2(0.0f, 0.0f);
      vertexDataArr[i * 4 + 2].tex = XMFLOAT2(1.0f, 0.0f);
      vertexDataArr[i * 4 + 3].tex = XMFLOAT2(1.0f, 1.0f);
    }

    for (UINT i = 0; i < 24; ++i) {
      InsertVertexElement(mMesh.vertexVec[i], vertexDataArr[i]);
    }

    mMesh.indexVec = {
        0,  1,  2,  2,  3,  0,   // 右面(+X面)
        4,  5,  6,  6,  7,  4,   // 左面(-X面)
        8,  9,  10, 10, 11, 8,   // 顶面(+Y面)
        12, 13, 14, 14, 15, 12,  // 底面(-Y面)
        16, 17, 18, 18, 19, 16,  // 背面(+Z面)
        20, 21, 22, 22, 23, 20   // 正面(-Z面)
    };
  }

  void CreateGPUInfo(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
  {
    const uint32 vbByteSize = mMesh.indexVec.size() * sizeof(VertexPosNormalTex);
    const uint32 ibByteSize = mMesh.indexVec.size() * sizeof(uint16);

    TIFF(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
    CopyMemory(mVertexBufferCPU->GetBufferPointer(), mMesh.vertexVec.data(), vbByteSize);

    TIFF(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
    CopyMemory(mIndexBufferCPU->GetBufferPointer(), mMesh.indexVec.data(), ibByteSize);

    mVertexBufferGPU =
        D3DUtil::CreateDefaultBuffer(device, commandList, mMesh.vertexVec.data(), vbByteSize, VertexBufferUploader);
    mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, mMesh.indexVec.data(), ibByteSize, IndexBufferUploader);

    VertexByteStride     = sizeof(VertexPosNormalTex);
    VertexBufferByteSize = vbByteSize;
    IndexFormat          = DXGI_FORMAT_R16_UINT;
    IndexBufferByteSize  = ibByteSize;

    mMaterial.diffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    mMaterial.fresnelR0     = XMFLOAT3(0.01f, 0.01f, 0.01f);
    mMaterial.roughness     = 0.125f;
  }

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView()
  {
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
    vbv.StrideInBytes  = VertexByteStride;
    vbv.SizeInBytes    = VertexBufferByteSize;
    return vbv;
  }

  D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
  {
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
    ibv.Format         = IndexFormat;
    ibv.SizeInBytes    = IndexBufferByteSize;
    return ibv;
  }

  virtual ID3D12Resource* GetVertexBufferGPU() { return nullptr; }
  virtual ID3D12Resource* GetIndexBufferGPU() { return nullptr; }

 public:
  MeshData<VertexPosNormalTex, uint16> mMesh;

  ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
  ComPtr<ID3D12Resource> IndexBufferUploader  = nullptr;

  uint32 VertexByteStride;
  uint32 VertexBufferByteSize;
  DXGI_FORMAT IndexFormat;
  uint32 IndexBufferByteSize;

  Material mMaterial;

  inline void InsertVertexElement(VertexType& vertexDst, const VertexData& vertexSrc)
  {
    static std::string semanticName;
    static const std::map<std::string, std::pair<size_t, size_t>> semanticSizeMap = {
        {"POSITION", std::pair<size_t, size_t>(0, 12)},
        {"NORMAL", std::pair<size_t, size_t>(12, 24)},
        {"TANGENT", std::pair<size_t, size_t>(24, 40)},
        {"COLOR", std::pair<size_t, size_t>(40, 56)},
        {"TEXCOORD", std::pair<size_t, size_t>(56, 64)}};

    for (size_t i = 0; i < VertexType::inputLayout.size(); i++) {
      semanticName      = VertexType::inputLayout[i].SemanticName;
      const auto& range = semanticSizeMap.at(semanticName);
      memcpy_s(reinterpret_cast<char*>(&vertexDst) + VertexType::inputLayout[i].AlignedByteOffset, range.second - range.first,
               reinterpret_cast<const char*>(&vertexSrc) + range.first, range.second - range.first);
    }
  }
};
#endif  // MODEL_BOX_H
