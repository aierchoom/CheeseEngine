#include "IGraphics.h"
#include <assert.h>

bool Graphics::Initialize(CheeseWindow* window)
{
  TIFF(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

  // use default adapter
  HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3dDevice));

  if (FAILED(hardwareResult)) {
    ComPtr<IDXGIAdapter> pWarpAdapter;
    TIFF(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    TIFF(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3dDevice)));
  }

  TIFF(mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

  mRtvDescriptorSize       = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  mDsvDescriptorSize       = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  mCbvSrvUavDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format           = mBackBufferFormat;
  msQualityLevels.SampleCount      = 4;
  msQualityLevels.Flags            = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  TIFF(mD3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

  m4xMsaaQuality = msQualityLevels.NumQualityLevels;
  assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

  CreateCommandObjects();
  CreateSwapChain(window);
  CreateRtvAndDsvDescriptorHeaps();
  OnResize(window);

  return true;
}

void Graphics::CreateCommandObjects()
{
  /**
   * Queue:The queue for the gpu processing task is a queue.Each gpu saves at least one queue.
   * The CPU USES list to submit commands to the queue of gpu processing.
   * Allocator:a buffer,when executed close list and ExecutedCommandLists(),submit the allocator to gpu queue.
   **/
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  // Specifies a command buffer that the gpu can execute.
  // A direct command list doesn't inherit and gpu state.
  queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  TIFF(mD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

  TIFF(mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
  const uint32 nodeMask = 0;
  TIFF(mD3dDevice->CreateCommandList(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     mDirectCmdListAlloc.Get(),  // Associated command allocator
                                     nullptr,                    // Initial PipelineStateObject
                                     IID_PPV_ARGS(mCommandList.GetAddressOf())));

  // Start off in a closed state.  This is because the first time we refer
  // to the command list we will Reset it, and it needs to be closed before calling Reset.
  mCommandList->Close();
}

void Graphics::CreateSwapChain(CheeseWindow* window)
{
  // Release the previous swapchain we will be recreating.
  mSwapChain.Reset();

  DXGI_SWAP_CHAIN_DESC sd;
  sd.BufferDesc.Width                   = window->GetWidth();
  sd.BufferDesc.Height                  = window->GetHeight();
  sd.BufferDesc.RefreshRate.Numerator   = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferDesc.Format                  = mBackBufferFormat;
  sd.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  sd.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
  sd.SampleDesc.Count                   = m4xMsaaState ? 4 : 1;
  sd.SampleDesc.Quality                 = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
  sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount                        = SwapChainBufferCount;
  sd.OutputWindow                       = window->GetHwnd();
  sd.Windowed                           = true;
  sd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  TIFF(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf()));
}

void Graphics::CreateRtvAndDsvDescriptorHeaps()
{
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
  rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
  rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtvHeapDesc.NodeMask       = 0;
  TIFF(mD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  dsvHeapDesc.NodeMask       = 0;
  TIFF(mD3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void Graphics::FlushCommandQueue()
{
  // Advance the fence value to mark commands up to this fence point.
  mCurrentFence++;

  // Add an instruction to the command queue to set a new fence point.  Because we
  // are on the GPU timeline, the new fence point won't be set until the GPU finishes
  // processing all the commands prior to this Signal().
  TIFF(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

  // CPU will waitting for GPU processing command.
  if (mFence->GetCompletedValue() < mCurrentFence) {
    HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    TIFF(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

void Graphics::ResetCommandList() { TIFF(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)); }

void Graphics::ExecuteCommandList()
{
  TIFF(mCommandList->Close());
  ID3D12CommandList* cmdLists[]{mCommandList.Get()};
  mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

ID3D12Resource* Graphics::CurrentBackBuffer() const { return mSwapChainBuffer[mCurrBackBuffer].Get(); }

D3D12_CPU_DESCRIPTOR_HANDLE Graphics::CurrentBackBufferView() const
{
  return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphics::DepthStencilView() const { return mDsvHeap->GetCPUDescriptorHandleForHeapStart(); }

void Graphics::OnResize(CheeseWindow* window)
{
  assert(mD3dDevice);
  assert(mSwapChain);
  assert(mDirectCmdListAlloc);

  /**
   * 1. FlushCommandQueue,waitting for GPU to complete the task.
   * 2. Reset CommandList,CmdListAlloc.
   * 3. Reset SwapChainBuffer pointer.
   * 4. Reset DepthStencilBuffer.
   * 5. Resize SwapChain size.
   * 6. Set ChainBuffer pointer, and Create TargetView for ChainBuffer.
   * 7. Create DepthStencil buffer&view.(committed resource)
   * 8. Using the barrier Transform depth buffer to depth write.
   * 9. Close and execute command list.
   * 10. FlushCommandQueue,wait until resize is complete.
   * 11. Set viewport.
   *
   **/

  FlushCommandQueue();

  TIFF(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

  // Release the previous resources we will be recreating.
  // First initialize will do nothing.
  for (int i = 0; i < SwapChainBufferCount; ++i) mSwapChainBuffer[i].Reset();
  mDepthStencilBuffer.Reset();

  // Resize the swap chain.
  TIFF(mSwapChain->ResizeBuffers(SwapChainBufferCount, window->GetWidth(), window->GetHeight(), mBackBufferFormat,
                                 DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

  mCurrBackBuffer = 0;

  // Bind buffer To mSwapChainBuffer and create target view.
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT i = 0; i < SwapChainBufferCount; i++) {
    TIFF(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
    mD3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
    rtvHeapHandle.Offset(1, mRtvDescriptorSize);
  }

  // Create the depth/stencil buffer and view.
  D3D12_RESOURCE_DESC depthStencilDesc;
  depthStencilDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depthStencilDesc.Alignment        = 0;
  depthStencilDesc.Width            = window->GetWidth();
  depthStencilDesc.Height           = window->GetHeight();
  depthStencilDesc.DepthOrArraySize = 1;
  depthStencilDesc.MipLevels        = 1;
  depthStencilDesc.Format           = DXGI_FORMAT_R24G8_TYPELESS;

  depthStencilDesc.SampleDesc.Count   = m4xMsaaState ? 4 : 1;
  depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
  depthStencilDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depthStencilDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear;
  optClear.Format                   = mDepthStencilFormat;
  optClear.DepthStencil.Depth       = 1.0f;
  optClear.DepthStencil.Stencil     = 0;
  CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  TIFF(mD3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON,
                                           &optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

  // Create descriptor to mip level 0 of entire resource using the format of the resource.
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
  dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
  dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsvDesc.Format             = mDepthStencilFormat;
  dsvDesc.Texture2D.MipSlice = 0;
  mD3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

  // Transition the resource from its initial state to be used as a depth buffer.
  CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
  mCommandList->ResourceBarrier(1, &resBarrier);

  // Execute the resize commands.
  TIFF(mCommandList->Close());
  ID3D12CommandList* cmdsLists[] = {mCommandList.Get()};
  mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  // Wait until resize is complete.
  FlushCommandQueue();

  // Update the viewport transform to cover the client area.
  ResizeViewprot(window->GetWidth(), window->GetHeight());
}

void Graphics::ResizeViewprot(uint32 width, uint32 height)
{
  mScreenViewport.TopLeftX = 0;
  mScreenViewport.TopLeftY = 0;
  mScreenViewport.Width    = static_cast<float>(width);
  mScreenViewport.Height   = static_cast<float>(height);
  mScreenViewport.MinDepth = 0.0f;
  mScreenViewport.MaxDepth = 1.0f;
  mScissorRect             = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
}
