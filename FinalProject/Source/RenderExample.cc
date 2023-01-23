#include <comdef.h>
#include <memory>
#include <vector>
#include <array>

#include <DirectXColors.h>

#include <Core/CheeseApp.h>
#include <Core/Camera.h>
#include <Utils/GameTimer.h>
#include <Utils/Log/Logger.h>
#include <Input/InputConponent.h>
#include <Graphics/IGraphics.h>
#include <Graphics/Shader.h>
#include <Graphics/ShaderInfo.h>
#include <Graphics/DDSTextureLoader.h>
#include <Model/Box.h>
#include <Windows.h>

using namespace DirectX;
using namespace std;

const float Pi = 3.1415926f;

XMFLOAT4X4 Identity4x4()
{
  static XMFLOAT4X4 I(1.0f, 0.0f, 0.0f, 0.0,  // line 1
                      0.0f, 1.0f, 0.0f, 0.0,  // line 2
                      0.0f, 0.0f, 1.0f, 0.0,  // line 3
                      0.0f, 0.0f, 0.0f, 1.0   // line 4
  );
  return I;
}

class RenderExample : public CheeseApp, IEvent
{
 public:
  RenderExample() : mWindow(new CheeseWindow(1280, 720)), mGraphics(new Graphics()), mBox(2, 2, 2) {}

  virtual ~RenderExample() {}

  virtual bool Init() override;
  virtual void Exit() override {}
  virtual bool Load() override;
  virtual void UnLoad() override {}

  void AddPitch(float val)
  {
    if (mIsMovingMouse) {
      mCamera.AddPitch(val * 0.001f);
    }
  }
  void AddYaw(float val)
  {
    if (mIsMovingMouse) {
      mCamera.AddYaw(val * 0.001f);
    }
  }

  void StartMoveMouse() { mIsMovingMouse = true; }
  void StopMoveMouse() { mIsMovingMouse = false; }

  virtual void Clear() override
  {
    SAFE_RELEASE_PTR(mWindow);
    SAFE_RELEASE_PTR(mGraphics);
  }

  virtual void OnResize(uint32 width, uint32 height) override;

  virtual void Run() override;
  virtual void Update(float dt) override;
  void Draw();

  void BuildPSO();

  inline CheeseWindow* GetWindow() const override { return mWindow; }
  inline CheString GetName() const override { return mProgramName; }

 private:
  Camera mCamera;

  CheString mProgramName = CTEXT("FinalProject");
  GameTimer mTimer;

  CheeseWindow* mWindow;
  Graphics* mGraphics;

  Shader mBoxShader;

  ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

  ComPtr<ID3D12Resource> mAlbedoMap       = nullptr;
  ComPtr<ID3D12Resource> mAlbedoMapUpload = nullptr;

  ComPtr<ID3D12Resource> mNormalMap       = nullptr;
  ComPtr<ID3D12Resource> mNormalMapUpload = nullptr;

  ComPtr<ID3D12PipelineState> mPSO = nullptr;

  XMFLOAT4X4 mWorld = Identity4x4();
  XMFLOAT4X4 mView  = Identity4x4();
  XMFLOAT4X4 mProj  = Identity4x4();

  float mTheta  = 1.5f * XM_PI;
  float mPhi    = XM_PIDIV4;
  float mRadius = 5.0f;

  Box<VertexPosNormalTex> mBox;
  Light mLight;

  bool mIsMovingMouse = false;
};

bool RenderExample::Init()
{
  // Register Action
  auto ic = InputComponent::Get();
  InputComponent::Get()->RegisterOnAction(EKeyMap::MOUSE_RBUTTON, EKeyStatus::PRESSED, *this, &RenderExample::StartMoveMouse);
  InputComponent::Get()->RegisterOnAction(EKeyMap::MOUSE_RBUTTON, EKeyStatus::RELEASED, *this, &RenderExample::StopMoveMouse);
  InputComponent::Get()->RegisterOnAxis(EAxisEvent::MOUSE_X, *this, &RenderExample::AddYaw);
  InputComponent::Get()->RegisterOnAxis(EAxisEvent::MOUSE_Y, *this, &RenderExample::AddPitch);

  // Register Event
  InputComponent::Get()->RegisterOnResize(*this, &RenderExample::OnResize);

  // Init graphics and Reset CommandList
  mGraphics->Initialize(mWindow);

  return true;
}

void RenderExample::OnResize(uint32 width, uint32 height)
{
  mWindow->ReSize(width, height);
  mGraphics->OnResize(mWindow);
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  mBoxShader.GetShaderInfo()->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), mCamera.GetViewProjMatrixXM());
}

bool RenderExample::Load()
{
  mGraphics->ResetCommandList();

  logger.Info(CTEXT("Build Shader..."));
  mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::VERTEX_SHADER);
  mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::PIXEL_SHADER);
  mBoxShader.GenerateShaderInfo(mGraphics->mD3dDevice.Get());
  mBoxShader.GetShaderInfo()->CreateRootSignature(mGraphics->mD3dDevice.Get());

  mBox.CreateGPUInfo(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get());

  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks.dds"), mAlbedoMap, mAlbedoMapUpload));

  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks_nmap.dds"), mNormalMap, mNormalMapUpload));

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  srvHeapDesc.NumDescriptors             = 1;
  srvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  TIFF(mGraphics->mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

  CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  srvDescriptor.Offset(0, 32);
  D3D12_SHADER_RESOURCE_VIEW_DESC albedoSrvDesc = {};
  albedoSrvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  albedoSrvDesc.Format                          = mAlbedoMap->GetDesc().Format;
  albedoSrvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  albedoSrvDesc.Texture2D.MostDetailedMip       = 0;
  albedoSrvDesc.Texture2D.MipLevels             = mAlbedoMap->GetDesc().MipLevels;
  albedoSrvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
  mGraphics->mD3dDevice->CreateShaderResourceView(mAlbedoMap.Get(), &albedoSrvDesc, srvDescriptor);

  D3D12_SHADER_RESOURCE_VIEW_DESC normalSrvDesc = {};

  srvDescriptor.Offset(1, 32);
  normalSrvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  normalSrvDesc.Format                        = mNormalMap->GetDesc().Format;
  normalSrvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
  normalSrvDesc.Texture2D.MostDetailedMip     = 0;
  normalSrvDesc.Texture2D.MipLevels           = mNormalMap->GetDesc().MipLevels;
  normalSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
  mGraphics->mD3dDevice->CreateShaderResourceView(mNormalMap.Get(), &normalSrvDesc, srvDescriptor);

  BuildPSO();

  mGraphics->ExecuteCommandList();
  mGraphics->FlushCommandQueue();

  mCamera.LookAt(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f));
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());
  mBoxShader.GetShaderInfo()->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  return true;
}

void RenderExample::Draw()
{
  // Reuse the memroy associated with command recording.
  // We can only reset when the associated command lists have finished execution on the GPU.
  TIFF(mGraphics->mDirectCmdListAlloc->Reset());

  // A command list can be reset after it has been added to the command queue via ExecutedCommandList.
  // Reusing the command list reuses memory.

  TIFF(mGraphics->mCommandList->Reset(mGraphics->mDirectCmdListAlloc.Get(), mPSO.Get()));

  // Indicate a state transition on the resource usage.
  CD3DX12_RESOURCE_BARRIER backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      mGraphics->CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
  mGraphics->mCommandList->ResourceBarrier(1, &backBufferBarrier);

  mGraphics->mCommandList->RSSetViewports(1, &mGraphics->mScreenViewport);
  mGraphics->mCommandList->RSSetScissorRects(1, &mGraphics->mScissorRect);

  // Clear the back buffer and depth buffer.
  mGraphics->mCommandList->ClearRenderTargetView(mGraphics->CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
  mGraphics->mCommandList->ClearDepthStencilView(mGraphics->DepthStencilView(),
                                                 D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

  // Specify the buffers we are going to render to.
  D3D12_CPU_DESCRIPTOR_HANDLE currBackBufferView = mGraphics->CurrentBackBufferView();
  D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView   = mGraphics->DepthStencilView();
  mGraphics->mCommandList->OMSetRenderTargets(1, &currBackBufferView, true, &depthStencilView);

  ID3D12DescriptorHeap* descriptorHeaps[] = {mSrvDescriptorHeap.Get()};
  mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  mGraphics->mCommandList->SetGraphicsRootSignature(mBoxShader.GetShaderInfo()->GetRootSignature());

  D3D12_VERTEX_BUFFER_VIEW vBufferView = mBox.VertexBufferView();
  mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);

  D3D12_INDEX_BUFFER_VIEW iBufferView = mBox.IndexBufferView();
  mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);
  mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
  tex.Offset(0, mGraphics->mCbvSrvUavDescriptorSize);

  mGraphics->mCommandList->SetGraphicsRootDescriptorTable(0, tex);
  tex.Offset(1, mGraphics->mCbvSrvUavDescriptorSize);
  mGraphics->mCommandList->SetGraphicsRootDescriptorTable(1, tex);

  for (auto pair : mBoxShader.GetShaderInfo()->mCBuffers) {
    auto cbuffer = pair.second;
    mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbuffer.GetSlot() + 2,
                                                               cbuffer.mUploadBuffer->GetGPUVirtualAddress());
  }
  mGraphics->mCommandList->DrawIndexedInstanced(static_cast<UINT>(mBox.mMesh.indexVec.size()), 1, 0, 0, 0);

  // Indicate a state tansition on the resource usage.
  backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mGraphics->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
  mGraphics->mCommandList->ResourceBarrier(1, &backBufferBarrier);

  mGraphics->ExecuteCommandList();

  // swap the back and front buffers
  HRESULT hr                 = mGraphics->mSwapChain->Present(0, 0);
  mGraphics->mCurrBackBuffer = (mGraphics->mCurrBackBuffer + 1) % mGraphics->SwapChainBufferCount;

  if (FAILED(hr)) {
    hr = mGraphics->mD3dDevice->GetDeviceRemovedReason();
    logger.Error(_com_error(hr).ErrorMessage());
  }

  // Wait until frame commands are complete. This waiting is inefficient and is
  // done for simplicity. Later we will show show how to organize our rendering code
  // so we do not have to wait per frame.
  mGraphics->FlushCommandQueue();
}

void RenderExample::Run()
{
  MSG msg = {0};
  mTimer.Reset();

  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      mTimer.Tick();
      Update(mTimer.DeltaTime());
      Draw();
    }
  }
}

void RenderExample::Update(float dt)
{
  float speed = 2.0f;
  if (InputComponent::Get()->GetKeyStatus(EKeyMap::KEY_W) == EKeyStatus::PRESSED) {
    mCamera.MoveForward(speed * dt);
  }
  if (InputComponent::Get()->GetKeyStatus(EKeyMap::KEY_S) == EKeyStatus::PRESSED) {
    mCamera.MoveForward(-speed * dt);
  }
  if (InputComponent::Get()->GetKeyStatus(EKeyMap::KEY_A) == EKeyStatus::PRESSED) {
    mCamera.MoveRight(-speed * dt);
  }
  if (InputComponent::Get()->GetKeyStatus(EKeyMap::KEY_D) == EKeyStatus::PRESSED) {
    mCamera.MoveRight(speed * dt);
  }
  // Convert Spherical to Cartesian coordinates.
  float x = mRadius * sinf(mPhi) * cosf(mTheta);
  float z = mRadius * sinf(mPhi) * sinf(mTheta);
  float y = mRadius * cosf(mPhi);

  XMMATRIX world = XMLoadFloat4x4(&mWorld);

  world = XMMatrixRotationX(sin(15.f));
  world = world * XMMatrixRotationY(sin(10.f));

  mBoxShader.GetShaderInfo()->SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(world));
  mBoxShader.GetShaderInfo()->SetMatrix(CTEXT("cbPerObject.gInvWorld"), InverseTranspose(world));

  mBoxShader.GetShaderInfo()->SetMaterial(CTEXT("cbMaterial.gMaterial"), mBox.mMaterial);
  mBoxShader.GetShaderInfo()->SetLight(CTEXT("cbMaterial.gLight"), mLight);
  mBoxShader.GetShaderInfo()->SetFloat3(CTEXT("cbMaterial.gEyePosW"), mCamera.GetPostion());

  mBoxShader.GetShaderInfo()->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
}

void RenderExample::BuildPSO()
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
  ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  psoDesc.InputLayout    = {VertexPosNormalTex::inputLayout.data(), (UINT)VertexPosNormalTex::inputLayout.size()};
  psoDesc.pRootSignature = {mBoxShader.GetShaderInfo()->GetRootSignature()};
  psoDesc.VS = {reinterpret_cast<BYTE*>(mBoxShader.GetVS()->GetBufferPointer()), mBoxShader.GetVS()->GetBufferSize()};
  psoDesc.PS = {reinterpret_cast<BYTE*>(mBoxShader.GetPS()->GetBufferPointer()), mBoxShader.GetPS()->GetBufferSize()};
  psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask            = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets      = 1;
  psoDesc.RTVFormats[0]         = mGraphics->mBackBufferFormat;
  psoDesc.SampleDesc.Count      = mGraphics->m4xMsaaState ? 4 : 1;
  psoDesc.SampleDesc.Quality    = mGraphics->m4xMsaaState ? (mGraphics->m4xMsaaQuality - 1) : 0;
  psoDesc.DSVFormat             = mGraphics->mDepthStencilFormat;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

DEFINE_APPLICATION_MAIN(RenderExample)