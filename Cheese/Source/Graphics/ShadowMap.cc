#include "ShadowMap.h"

#include "D3DUtil.h"

ShadowMap::ShadowMap(ID3D12Device* device, uint32 width, uint32 height)
{
  mD3dDevice = device;
  mWidth     = width;
  mHeight    = height;

  mViewport    = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
  mScissorRect = {0, 0, (int)width, (int)height};

  BuildResource();
  BuildDescriptors();
}

uint32 ShadowMap::GetWidth() const { return mWidth; }

uint32 ShadowMap::GetHeight() const { return mHeight; }

ID3D12Resource* ShadowMap::GetResource() { return mShadowMap.Get(); }

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDsv() const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart()); }

D3D12_VIEWPORT ShadowMap::GetViewport() const { return mViewport; }

D3D12_RECT ShadowMap::GetScissorRect() const { return mScissorRect; }

void ShadowMap::OnResize(uint32 width, uint32 height)
{
  if ((mWidth != width) || (mHeight != height)) {
    mWidth  = width;
    mHeight = height;

    BuildResource();
  }
}

void ShadowMap::BuildDescriptors()
{
  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  dsvHeapDesc.NodeMask       = 0;
  TIFF(mD3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

  // Create DSV to resource so we can render to the shadow map.
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
  dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
  dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsvDesc.Format             = mFormat;
  dsvDesc.Texture2D.MipSlice = 0;
  CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
  mD3dDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, dsvHandle);
}

void ShadowMap::CreateShadowMapSrv(CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle)
{
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format                          = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  srvDesc.Format                          = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
  srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip       = 0;
  srvDesc.Texture2D.MipLevels             = 1;
  srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
  srvDesc.Texture2D.PlaneSlice            = 0;
  mD3dDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, srvCpuHandle);
}

void ShadowMap::BuildResource()
{
  D3D12_RESOURCE_DESC texDesc;
  ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
  texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texDesc.Alignment          = 0;
  texDesc.Width              = mWidth;
  texDesc.Height             = mHeight;
  texDesc.DepthOrArraySize   = 1;
  texDesc.MipLevels          = 1;
  texDesc.Format             = mFormat;
  texDesc.SampleDesc.Count   = 1;
  texDesc.SampleDesc.Quality = 0;
  texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear;
  optClear.Format               = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  optClear.DepthStencil.Depth   = 1.0f;
  optClear.DepthStencil.Stencil = 0;

  TIFF(mD3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear,
                                           IID_PPV_ARGS(&mShadowMap)));
}
