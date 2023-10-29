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

class Graphics
{
 public:
  Graphics() = default;
  bool Initialize(CheeseWindow* window);

  void CreateCommandObjects();
  void CreateSwapChain(CheeseWindow* window);
  void CreateRtvAndDsvDescriptorHeaps();

  void FlushCommandQueue();
  void ResetCommandList();
  void ExecuteCommandList();

  ID3D12Resource* CurrentBackBuffer() const;
  ID3D12Resource* MotionVectorBuffer() const;
  D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE MotionVectorBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE MotionVectorBufferView1() const;
  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

  virtual void OnResize(CheeseWindow* window);
  void ResizeViewprot(uint32 width, uint32 height);

 public:
  bool m4xMsaaState   = false;
  UINT m4xMsaaQuality = 0;

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
  ComPtr<ID3D12Resource> mMotionVectorBuffer;
  ComPtr<ID3D12Resource> mDepthStencilBuffer;

  ComPtr<ID3D12DescriptorHeap> mRtvHeap;
  ComPtr<ID3D12DescriptorHeap> mDsvHeap;

  D3D12_VIEWPORT mScreenViewport = {};
  D3D12_RECT mScissorRect        = {};

  UINT mRtvDescriptorSize       = 0;
  UINT mDsvDescriptorSize       = 0;
  UINT mCbvSrvUavDescriptorSize = 0;

  D3D_DRIVER_TYPE mD3dDriverType  = D3D_DRIVER_TYPE_HARDWARE;
  DXGI_FORMAT mBackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_FORMAT mMotionVectorFormat = DXGI_FORMAT_R16G16_FLOAT;
  DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

#endif  // GRAPHICS_IGRAPHICS_H