#ifndef MODEL_TEXTURE2D_H
#define MODEL_TEXTURE2D_H
#include "Common/TypeDef.h"
#include <d3d12.h>
struct Texture2D {
  D3D12_SRV_DIMENSION Dimension;
  ComPtr<ID3D12Resource> Resource;
  ComPtr<ID3D12Resource> ResourceUpload;
};
#endif  // MODEL_TEXTURE2D_H