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
#include <Graphics/ShaderInfo.h>
#include <Model/Box.h>

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

  virtual void Clear() override
  {
    SAFE_RELEASE_PTR(mWindow);
    SAFE_RELEASE_PTR(mGraphics);
  }

  virtual void OnKeyDown(EKeyMap keyMap) override;

  virtual void OnResize(uint32 width, uint32 height) override;

  virtual void Run() override;
  virtual void Update(float dt) override;
  void Draw();

  void LoadBox();
  void BuildRootSignture();
  void BuildShader();
  void BuildPSO();

  inline CheeseWindow* GetWindow() const override { return mWindow; }
  inline CheString GetName() const override { return mProgramName; }

 private:
  Camera mCamera;

  CheString mProgramName = CTEXT("FinalProject");
  GameTimer mTimer;

  CheeseWindow* mWindow;

  Graphics* mGraphics;

  ComPtr<ID3DBlob> mVsByteCode = nullptr;
  ShaderInfo* mVsReflect       = nullptr;
  ComPtr<ID3DBlob> mPsByteCode = nullptr;
  ShaderInfo* mPsReflect       = nullptr;

  ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

  ComPtr<ID3D12PipelineState> mPSO = nullptr;

  XMFLOAT4X4 mWorld = Identity4x4();
  XMFLOAT4X4 mView  = Identity4x4();
  XMFLOAT4X4 mProj  = Identity4x4();

  float mTheta  = 1.5f * XM_PI;
  float mPhi    = XM_PIDIV4;
  float mRadius = 5.0f;

  Box<VertexPosNormalTex> mBox;
  Light mLight;
};

bool RenderExample::Init()
{
  // Register Event
  InputComponent::Get()->RegisterOnKeyDown(*this);
  InputComponent::Get()->RegisterOnResize(*this);
  auto ic = InputComponent::Get();

  // Init graphics and Reset CommandList
  mGraphics->Initialize(mWindow);

  return true;
}

void RenderExample::OnKeyDown(EKeyMap keyMap)
{
  if (keyMap == EKeyMap::KEY_W) {
    logger.Info(CTEXT("CLICK W"));
  }
}

void RenderExample::OnResize(uint32 width, uint32 height)
{
  mWindow->ReSize(width, height);
  mGraphics->OnResize(mWindow);
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  mVsReflect->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), mCamera.GetViewProjMatrixXM());
}

bool RenderExample::Load()
{
  mGraphics->ResetCommandList();

  BuildRootSignture();
  BuildShader();
  LoadBox();
  BuildPSO();

  mGraphics->ExecuteCommandList();
  mGraphics->FlushCommandQueue();

  mCamera.SetPosition(0.0f, 0.0f, -5.0f);
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());
  mVsReflect->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

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

  ID3D12DescriptorHeap* cbvHeap           = mVsReflect->GetCbvHeap();
  ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap};
  mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  mGraphics->mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

  D3D12_VERTEX_BUFFER_VIEW vBufferView = mBox.VertexBufferView();
  mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);

  D3D12_INDEX_BUFFER_VIEW iBufferView = mBox.IndexBufferView();
  mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);
  mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  auto cbPerObject = mVsReflect->mCBuffers[CTEXT("cbPerObject")];
  mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbPerObject.GetSlot(),
                                                             cbPerObject.mUploadBuffer->GetGPUVirtualAddress());
  auto cbMaterial = mPsReflect->mCBuffers[CTEXT("cbMaterial")];
  mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbMaterial.GetSlot(),
                                                             cbMaterial.mUploadBuffer->GetGPUVirtualAddress());
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
  static float rotTime = 0.0f;
  rotTime += dt;
  // Convert Spherical to Cartesian coordinates.
  float x = mRadius * sinf(mPhi) * cosf(mTheta);
  float z = mRadius * sinf(mPhi) * sinf(mTheta);
  float y = mRadius * cosf(mPhi);

  XMMATRIX world = XMLoadFloat4x4(&mWorld);
  world          = XMMatrixRotationZ(sin(rotTime));
  world          = world * XMMatrixRotationY(sin(rotTime));

  mVsReflect->SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(world));
  mVsReflect->SetMatrix(CTEXT("cbPerObject.gInvWorld"), InverseTranspose(world));

  mPsReflect->SetMaterial(CTEXT("cbMaterial.gMaterial"), mBox.mMaterial);
  mPsReflect->SetLight(CTEXT("cbMaterial.gLight"), mLight);
  mPsReflect->SetFloat3(CTEXT("cbMaterial.gEyePosW"), mCamera.GetPostion());

  mVsReflect->SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
}

void RenderExample::LoadBox() { mBox.CreateGPUInfo(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get()); }

void RenderExample::BuildRootSignture()
{
  CD3DX12_ROOT_PARAMETER slotRootParameter[3];

  slotRootParameter[0].InitAsConstantBufferView(0);
  slotRootParameter[1].InitAsConstantBufferView(1);
  slotRootParameter[2].InitAsConstantBufferView(2);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
                                          D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob         = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(),
                                           errorBlob.GetAddressOf());

  if (errorBlob != nullptr) {
    ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
  }
  TIFF(hr);

  TIFF(mGraphics->mD3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                                  IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void RenderExample::BuildShader()
{
  HRESULT hr = S_OK;

  logger.Debug(CTEXT("Build vertex shader."));
  mVsByteCode = D3DUtil::CompileShader(CTEXT("Shaders/color.hlsl"), nullptr, CTEXT("VS"), CTEXT("vs_5_0"));
  mVsReflect  = new ShaderInfo();
  mVsReflect->Create(mVsByteCode.Get(), mGraphics->mD3dDevice.Get());

  logger.Debug(CTEXT("Build pixel shader."));
  mPsByteCode = D3DUtil::CompileShader(CTEXT("Shaders/color.hlsl"), nullptr, CTEXT("PS"), CTEXT("ps_5_0"));
  mPsReflect  = new ShaderInfo();
  mPsReflect->Create(mPsByteCode.Get(), mGraphics->mD3dDevice.Get());
}

void RenderExample::BuildPSO()
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
  ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  psoDesc.InputLayout           = {VertexPosNormalTex::inputLayout.data(), (UINT)VertexPosNormalTex::inputLayout.size()};
  psoDesc.pRootSignature        = {mRootSignature.Get()};
  psoDesc.VS                    = {reinterpret_cast<BYTE*>(mVsByteCode->GetBufferPointer()), mVsByteCode->GetBufferSize()};
  psoDesc.PS                    = {reinterpret_cast<BYTE*>(mPsByteCode->GetBufferPointer()), mPsByteCode->GetBufferSize()};
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