#ifndef GRAPHICS_SHADOW_MAP
#define GRAPHICS_SHADOW_MAP
#include <Common/TypeDef.h>
#include <d3d12.h>
#include <d3dx12.h>

class ShadowMap
{
 public:
  ShadowMap(ID3D12Device* device, uint32 width, uint32 height);

  ShadowMap(const ShadowMap& rhs)            = delete;
  ShadowMap& operator=(const ShadowMap& rhs) = delete;
  ~ShadowMap()                               = default;

  uint32 GetWidth() const;
  UINT GetHeight() const;
  ID3D12Resource* GetResource();

  CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv() const;

  D3D12_VIEWPORT GetViewport() const;
  D3D12_RECT GetScissorRect() const;

  void OnResize(uint32 width, uint32 height);
  void CreateShadowMapSrv(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv);

 private:
  void BuildResource();
  void BuildDescriptors();

 private:
  ID3D12Device* mD3dDevice = nullptr;

  D3D12_VIEWPORT mViewport;
  D3D12_RECT mScissorRect;

  uint32 mWidth       = 0;
  uint32 mHeight      = 0;
  DXGI_FORMAT mFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

  ComPtr<ID3D12Resource> mShadowMap = nullptr;
  ComPtr<ID3D12DescriptorHeap> mDsvHeap;
};

#endif  // FINAL_PROJECT_SHADOW_MAP
