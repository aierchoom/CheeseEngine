#include "Mesh.h"

#include <d3dcompiler.h>
#include "Graphics/D3DUtil.h"

using namespace std;

std::vector<D3D12_INPUT_ELEMENT_DESC> Vertex::InputLayout = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

IMesh::IMesh() : IMesh(std::vector<Vertex>()) {}

IMesh::IMesh(const std::vector<Vertex>& vertices) : mVertices(vertices) {}
IMesh::IMesh(std::vector<Vertex>&& vertices) : mVertices(std::move(vertices)) {}