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
  Model() : mMeshes(), mShaderInfos(), mMatDesc() {}
  ~Model() {}

  inline void AddMesh(IMesh *mesh) { mMeshes.push_back(mesh); }
  inline std::vector<IMesh *> &GetMeshes() { return mMeshes; }

  inline void AddShaderInfo(const CheString &infoName, const ShaderSettings &settings, ID3D12Device *device)
  {
    if (mShaderInfos.find(infoName) == mShaderInfos.end()) {
      mShaderInfos[infoName].Create(device, settings);
    }
  }

  inline ShaderInfo &GetShaderInfo(const CheString &infoName) { return mShaderInfos[infoName]; }

  inline const MaterialDesc &GetMaterialDesc() const { return mMatDesc; }
  inline void SetMaterialDesc(const MaterialDesc &matDesc) { mMatDesc = matDesc; }

  inline void SetPosition(float x, float y, float z) { mTransform.SetPosition(x, y, z); }
  inline DirectX::XMMATRIX GetTransformMatrix() const { return mTransform.GetLocalToWorldMatrixXM(); }

 private:
  std::vector<IMesh *> mMeshes;

  std::unordered_map<CheString, ShaderInfo> mShaderInfos;

  MaterialDesc mMatDesc;

  Transform mTransform;
};
#endif  // MODEL_MODEL_H
