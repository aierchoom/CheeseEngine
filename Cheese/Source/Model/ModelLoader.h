#ifndef MODEL_MODEL_LOADER_H
#define MODEL_MODEL_LOADER_H
#include "Common/TypeDef.h"
#include "tinygltf/tiny_gltf.h"
#include "Model/Model.h"

class ModelLoader
{
 public:
  static void LoadGLTF(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const CheString& fileName, Model& model);

  static void CreateTexture2D(ID3D12Device*, ID3D12GraphicsCommandList* cmdList, Texture2D& texture, const tinygltf::Image& image);
};
#endif  // MODEL_MODEL_LOADER_H