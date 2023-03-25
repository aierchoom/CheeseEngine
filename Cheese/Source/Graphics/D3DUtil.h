#ifndef GRAPHICS_D3DUTIL_H
#define GRAPHICS_D3DUTIL_H
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include "Model/Texture2D.h"
#include "Core/CoreMinimal.h"
#include "DDSTextureLoader.h"

class DxException
{
 public:
  DxException() = default;
  DxException(HRESULT hr, const CheString& funcName, const CheString& fileName, int lineNo);

  CheString ToString() const;

 private:
  HRESULT mErrorCode = S_OK;
  CheString mFuncName;
  CheString mFileName;
  int mLineNo = -1;
};

// Throw exception if failed
#ifndef TIFF
#define TIFF(x)                                              \
  {                                                          \
    HRESULT hr__       = (x);                                \
    CheString funcName = ConvertToCheString(#x);             \
    CheString fileName = ConvertToCheString(__FILE__);       \
    if (FAILED(hr__)) {                                      \
      throw DxException(hr__, funcName, fileName, __LINE__); \
    }                                                        \
  }
#endif  // TIFF

#ifndef ReleaseCom
#define ReleaseCom(x) \
  {                   \
    if (x) {          \
      x->Release();   \
      x = 0;          \
    }                 \
  }
#endif  // ReleaseCom

class D3DUtil
{
 public:
  static bool IsKeyDown(int vKeyCode);
  static CheString ToString(HRESULT hr);

  static ComPtr<ID3DBlob> LoadBinary(const CheString& fileName);

  static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData,
                                                    uint64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);

  static ComPtr<ID3DBlob> CompileShader(const CheString& fileName, const D3D_SHADER_MACRO* defines, const CheString& entryPoint,
                                        const CheString& target);

  static HRESULT CreateTexture2DFromDDS(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, CheString szFileName, Texture2D& texture,
                                        D3D12_SRV_DIMENSION dimension = D3D12_SRV_DIMENSION_TEXTURE2D)
  {
    texture.Dimension = dimension;
    return DirectX::CreateDDSTextureFromFile12(device, cmdList, szFileName.c_str(), texture.Resource, texture.ResourceUpload);
  }
};

inline DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::FXMMATRIX M)
{
  using namespace DirectX;

  XMMATRIX A = M;
  A.r[3]     = g_XMIdentityR3;

  return XMMatrixTranspose(XMMatrixInverse(nullptr, A));
}

#endif  // GRAPHICS_D3DUTIL_H
