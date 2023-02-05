#ifndef MODEL_MODEL_H
#define MODEL_MODEL_H
#include "Common/TypeDef.h"

#include <vector>
#include <memory>
#include <d3d12.h>

#include "Math/Transform.h"
#include "Shader/ShaderInfo.h"
#include "Mesh.h"

class Model
{
 public:
  Model() : mMeshes(), mShaderInfo(), mMatDesc() {}
  ~Model() {}

  inline void AddMesh(IMesh *mesh) { mMeshes.push_back(mesh); }
  inline std::vector<IMesh *> &GetMeshes() { return mMeshes; }

  inline void CreateShaderInfo(ID3D12Device *device, const ShaderSettings &settings) { mShaderInfo.Create(device, settings); }
  inline ShaderInfo &GetShaderInfo() { return mShaderInfo; }

  inline const MaterialDesc &GetMaterialDesc() const { return mMatDesc; }
  inline void SetMaterialDesc(const MaterialDesc &matDesc) { mMatDesc = matDesc; }

  inline void SetPosition(float x, float y, float z) { mTransform.SetPosition(x, y, z); }
  inline DirectX::XMMATRIX GetTransformMatrix() const { return mTransform.GetLocalToWorldMatrixXM(); }

 private:
  std::vector<IMesh *> mMeshes;
  ShaderInfo mShaderInfo;

  MaterialDesc mMatDesc;

  Transform mTransform;
};
#endif  // MODEL_MODEL_H
