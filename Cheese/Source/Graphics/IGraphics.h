#ifndef GRAPHICS_IGRAPHICS_H
#define GRAPHICS_IGRAPHICS_H

#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "Core/CoreMinimal.h"
#include "Core/CheeseWindow.h"
#include "D3DUtil.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

struct ResolutionInfo {
  int32 RenderWidth;    ///< The current render width.
  int32 RenderHeight;   ///< The current render height.
  int32 DisplayWidth;   ///< The current display width.
  int32 DisplayHeight;  ///< The current display height.
};

class Graphics
{
 public:
  Graphics() = default;
  bool Initialize(CheeseWindow* window, const ResolutionInfo& resolution);

  void CreateCommandObjects();
  void CreateSwapChain(CheeseWindow* window);
  void CreateRtvAndDsvDescriptorHeaps();
  void CreateFsr2RtvAndDsvDescriptorHeaps();

  void CreateFsr2Buffer(const ResolutionInfo& resolution);

  void FlushCommandQueue();
  void ResetCommandList();
  void ExecuteCommandList();

  ID3D12Resource* CurrentBackBuffer() const;
  ID3D12Resource* RenderTargetBuffer() const;
  ID3D12Resource* ColorTargetBuffer() const;
  ID3D12Resource* ColorDepthBuffer() const;
  ID3D12Resource* MotionVectorBuffer() const;
  D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE ColorTargetBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE ColorDepthBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE MotionVectorBufferView() const;

  virtual void OnResize(const ResolutionInfo& resolution);
  void ResizeViewprot(uint32 width, uint32 height);

 public:
  ComPtr<IDXGIFactory4> mdxgiFactory;
  ComPtr<IDXGISwapChain> mSwapChain;
  ComPtr<ID3D12Device> mD3dDevice;

  ComPtr<ID3D12Fence> mFence;
  UINT64 mCurrentFence = 0;

  ComPtr<ID3D12CommandQueue> mCommandQueue;
  ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
  ComPtr<ID3D12GraphicsCommandList> mCommandList;

  static const int SwapChainBufferCount = 2;
  int mCurrBackBuffer                   = 0;
  ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
  ComPtr<ID3D12Resource> mRenderBuffer;
  ComPtr<ID3D12Resource> mColorBuffer;
  ComPtr<ID3D12Resource> mColorDepthBuffer;
  ComPtr<ID3D12Resource> mMotionVectorBuffer;
  ComPtr<ID3D12Resource> mRenderDepthBuffer;

  ComPtr<ID3D12DescriptorHeap> mRtvHeap;
  ComPtr<ID3D12DescriptorHeap> mDsvHeap;
  const int32 mFsr2BufferCount     = 3;
  const int32 RenderRtvIndex       = 0;
  const int32 ColorRtvIndex        = 1;
  const int32 MotionVectorRtvIndex = 2;
  ComPtr<ID3D12DescriptorHeap> mFsr2RtvHeap;
  ComPtr<ID3D12DescriptorHeap> mFsr2DsvHeap;

  D3D12_VIEWPORT mScreenViewport = {};
  D3D12_RECT mScissorRect        = {};

  UINT mRtvDescriptorSize       = 0;
  UINT mDsvDescriptorSize       = 0;
  UINT mCbvSrvUavDescriptorSize = 0;

  D3D_DRIVER_TYPE mD3dDriverType  = D3D_DRIVER_TYPE_HARDWARE;
  DXGI_FORMAT mBackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_FORMAT mMotionVectorFormat = DXGI_FORMAT_R16G16_FLOAT;
  DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
};

#endif  // GRAPHICS_IGRAPHICS_H