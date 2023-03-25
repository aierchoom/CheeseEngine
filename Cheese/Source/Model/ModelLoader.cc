#include "ModelLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define STBI_MSC_SECURE_CRT

#include <d3d12.h>
#include "d3dx12.h"
#include "Graphics/D3DUtil.h"
#include "Utils/Log/Logger.h"
#include "tinygltf/tiny_gltf.h"

void ModelLoader::LoadGLTF(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const CheString& fileName, Model& model)
{
  logger.Info(CTEXT("Loading model:") + fileName);
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  tinygltf::Model gltfModel;
  bool res = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, ConvertToMultiByte(fileName).c_str());
  if (!res) {
    logger.Error(CTEXT("Load") + fileName + CTEXT("Error"));
    return;
  }

  for (tinygltf::Node node : gltfModel.nodes) {
    tinygltf::Mesh drawMesh = gltfModel.meshes[node.mesh];
    for (tinygltf::Primitive primitive : drawMesh.primitives) {
      if (primitive.attributes.size() == 3) continue;

      // get indices gltf buffer&view.
      tinygltf::Accessor& indicesAccessor     = gltfModel.accessors[primitive.indices];
      tinygltf::BufferView& indicesBufferView = gltfModel.bufferViews[indicesAccessor.bufferView];
      tinygltf::Buffer& indicesData           = gltfModel.buffers[indicesBufferView.buffer];

      // get position gltf buffer&view.
      tinygltf::Accessor& posAccessor     = gltfModel.accessors[primitive.attributes.at("POSITION")];
      tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
      tinygltf::Buffer& posBuffer         = gltfModel.buffers[posBufferView.buffer];

      const float* posData = reinterpret_cast<const float*>(&(posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]));

      std::vector<Vertex> vertexData(posAccessor.count);

      // assemble vertex data.
      for (int i = 0; i < vertexData.size(); i++) {
        Vertex vertex;
        vertex.Position = DirectX::XMFLOAT3(&posData[i * 3]);
        // for normal
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          tinygltf::Accessor& normalAccessor     = gltfModel.accessors[primitive.attributes.at("NORMAL")];
          tinygltf::BufferView& normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
          tinygltf::Buffer& normalBuffer         = gltfModel.buffers[normalBufferView.buffer];

          const float* normalData =
              reinterpret_cast<const float*>(&(normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]));
          vertex.Normal = DirectX::XMFLOAT3(&normalData[i * 3]);
        }

        // for tangent
        if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
          tinygltf::Accessor& tangentAccessor     = gltfModel.accessors[primitive.attributes.at("TANGENT")];
          tinygltf::BufferView& tangentBufferView = gltfModel.bufferViews[tangentAccessor.bufferView];
          tinygltf::Buffer& tangentBuffer         = gltfModel.buffers[tangentBufferView.buffer];

          const float* tangentData =
              reinterpret_cast<const float*>(&(tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]));
          vertex.Tangent = DirectX::XMFLOAT4(&tangentData[i * 4]);
        }

        // for tex coord
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
          tinygltf::Accessor& texcoordAccessor     = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
          tinygltf::BufferView& texcoordBufferView = gltfModel.bufferViews[texcoordAccessor.bufferView];
          tinygltf::Buffer& texcoordBuffer         = gltfModel.buffers[texcoordBufferView.buffer];

          const float* texcoordData =
              reinterpret_cast<const float*>(&(texcoordBuffer.data[texcoordBufferView.byteOffset + texcoordAccessor.byteOffset]));
          vertex.TexCoord = DirectX::XMFLOAT2(&texcoordData[i * 2]);
        }
        vertexData[i] = vertex;
      }

      IMesh* mesh;

      if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        std::vector<uint16> indexData(indicesAccessor.count);
        const void* ibdata    = indicesData.data.data() + indicesBufferView.byteOffset + indicesAccessor.byteOffset;
        const uint32 byteSize = static_cast<uint32>(indexData.size() * sizeof(uint16));
        memcpy_s(indexData.data(), byteSize, ibdata, byteSize);
        Mesh<uint16>* tempMesh = new Mesh<uint16>(std::move(vertexData), std::move(indexData));
        mesh                   = tempMesh;
      } else {
        std::vector<uint32> indexData(indicesAccessor.count);
        const void* ibdata    = indicesData.data.data() + indicesBufferView.byteOffset + indicesAccessor.byteOffset;
        const uint32 byteSize = static_cast<uint32>(indexData.size() * sizeof(uint32));
        memcpy_s(indexData.data(), byteSize, ibdata, byteSize);
        Mesh<uint32>* tempMesh = new Mesh<uint32>(std::move(vertexData), std::move(indexData));
        mesh                   = tempMesh;
      }

      Material material;
      // process texture
      tinygltf::Material& gltfMaterial = gltfModel.materials[primitive.material];
      if (gltfMaterial.additionalValues["alphaMode"].string_value == "BLEND") {
        mesh->SetBlend(true);
      }

      tinygltf::Image& diffuseImage = gltfModel.images.at(gltfMaterial.values["baseColorTexture"].TextureIndex());
      CreateTexture2D(device, cmdList, material.Textures[CTEXT("gAlbedoMap")], diffuseImage);

      tinygltf::Image& normalImage = gltfModel.images.at(gltfMaterial.additionalValues["normalTexture"].TextureIndex());
      CreateTexture2D(device, cmdList, material.Textures[CTEXT("gNormalMap")], normalImage);

      tinygltf::Image& ormImage = gltfModel.images.at(gltfMaterial.values["metallicRoughnessTexture"].TextureIndex());
      CreateTexture2D(device, cmdList, material.Textures[CTEXT("gORMMap")], ormImage);

      mesh->SetMaterial(material);
      model.AddMesh(mesh);
    }
  }
  logger.Info(CTEXT("Load: ") + fileName + CTEXT(" Successed"));
}

void ModelLoader::CreateTexture2D(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Texture2D& texture, const tinygltf::Image& image)
{
  texture.Dimension = D3D12_SRV_DIMENSION_TEXTURE2D;

  D3D12_RESOURCE_DESC textureDesc = {};
  textureDesc.MipLevels           = 1;
  textureDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.Width               = image.width;
  textureDesc.Height              = image.height;
  textureDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDesc.DepthOrArraySize    = 1;
  textureDesc.SampleDesc.Count    = 1;
  textureDesc.SampleDesc.Quality  = 0;
  textureDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  TIFF(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &textureDesc,
                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture.Resource)));

  TIFF(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                       &CD3DX12_RESOURCE_DESC::Buffer(image.image.size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&texture.ResourceUpload)));

  D3D12_SUBRESOURCE_DATA textureData = {};
  textureData.pData                  = image.image.data();
  textureData.RowPitch               = image.width * image.component;
  textureData.SlicePitch             = image.width * image.height * image.component;

  UpdateSubresources(cmdList, texture.Resource.Get(), texture.ResourceUpload.Get(), 0, 0, 1, &textureData);
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}