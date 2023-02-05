#ifndef MODEL_TEXTURE2D_H
#define MODEL_TEXTURE2D_H
#include "Common/TypeDef.h"
#include <d3d12.h>
struct Texture2D {
  ComPtr<ID3D12Resource> Resource;
  ComPtr<ID3D12Resource> ResourceUpload;
};
#endif  // MODEL_TEXTURE2D_H
