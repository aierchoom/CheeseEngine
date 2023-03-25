#ifndef MODEL_MODEL_H
#define MODEL_MODEL_H
#include "Common/TypeDef.h"

#include <vector>
#include <memory>
#include <d3d12.h>

#include "Math/Transform.h"
#include "Shader/Shader.h"
#include "Shader/ShaderHelper.h"
#include "Mesh.h"

class Model
{
 public:
  Model() : mMeshes() {}
  ~Model() {}

  inline void AddMesh(IMesh *mesh) { mMeshes.push_back(mesh); }
  inline const std::vector<IMesh *> &GetMeshes() const { return mMeshes; }

 private:
  std::vector<IMesh *> mMeshes;
};
#endif  // MODEL_MODEL_H