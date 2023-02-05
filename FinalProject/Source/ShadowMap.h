#ifndef FINAL_PROJECT_SHADOW_MAP
#define FINAL_PROJECT_SHADOW_MAP
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
  CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv() const;
  CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv() const;

  D3D12_VIEWPORT GetViewport() const;
  D3D12_RECT GetScissorRect() const;

  void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

  void OnResize(uint32 width, uint32 height);

 private:
  void BuildDescriptors();
  void BuildResource();

 private:
  ID3D12Device* mD3dDevice = nullptr;

  D3D12_VIEWPORT mViewport;
  D3D12_RECT mScissorRect;

  uint32 mWidth       = 0;
  uint32 mHeight      = 0;
  DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

  CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
  CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
  CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

  ComPtr<ID3D12Resource> mShadowMap = nullptr;
};

#endif  // FINAL_PROJECT_SHADOW_MAP
