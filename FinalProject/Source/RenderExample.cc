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
#include <Graphics/DDSTextureLoader.h>
#include <Shader/Shader.h>
#include <Model/Model.h>
#include <Model/Geometry.h>

#include "ShadowMap.h"

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

class RenderExample : public CheeseApp
{
 public:
  RenderExample() : mWindow(new CheeseWindow(1280, 720)), mGraphics(new Graphics()), models(), mSkybox(), mLight() {}

  virtual ~RenderExample() {}

  virtual bool Init() override;
  virtual void Exit() override {}
  virtual bool Load() override;

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

  virtual void OnResize(uint32 width, uint32 height);

  virtual void Run() override;
  virtual void Update(float dt) override;
  void Draw();
  void DrawScene();

  void BuildPSO();

  inline CheeseWindow* GetWindow() const override { return mWindow; }
  inline CheString GetName() const override { return mProgramName; }

 private:
  Camera mCamera;

  CheString mProgramName = CTEXT("FinalProject");
  GameTimer mTimer;

  CheeseWindow* mWindow;
  Graphics* mGraphics;

  unique_ptr<ShadowMap> mShadowMap;

  Shader mBoxShader;
  Shader mSkyboxShader;

  unordered_map<CheString, ComPtr<ID3D12PipelineState>> mPSOs;

  XMFLOAT4X4 mWorld = Identity4x4();
  XMFLOAT4X4 mView  = Identity4x4();
  XMFLOAT4X4 mProj  = Identity4x4();

  float mTheta  = 1.5f * XM_PI;
  float mPhi    = XM_PIDIV4;
  float mRadius = 5.0f;

  Model mSkybox;

  std::vector<Model*> models;
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

  mShadowMap = std::make_unique<ShadowMap>(mGraphics->mD3dDevice.Get(), 2048, 2048);

  return true;
}

void RenderExample::OnResize(uint32 width, uint32 height)
{
  mWindow->ReSize(width, height);
  mGraphics->OnResize(mWindow);
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  for (auto model : models) {
    model->GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gCameraTrans"), mCamera.GetViewProjMatrixXM());
  }
}

bool RenderExample::Load()
{
  mGraphics->ResetCommandList();

  logger.Info(CTEXT("Build Shader..."));
  mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::VERTEX_SHADER);
  mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::PIXEL_SHADER);
  mBoxShader.CreateRootSignature(mGraphics->mD3dDevice.Get());

  logger.Info(CTEXT("Build skybox shader..."));
  mSkyboxShader.AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::VERTEX_SHADER);
  mSkyboxShader.AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::PIXEL_SHADER);
  mSkyboxShader.CreateRootSignature(mGraphics->mD3dDevice.Get());

  IMesh* skyboxMesh = Geometry::GenerateBox(1, 1, 1);

  Material skyboxMat;
  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/grasscube1024.dds"), skyboxMat.Textures[CTEXT("gCubeMap")].Resource,
                                  skyboxMat.Textures[CTEXT("gCubeMap")].ResourceUpload));
  skyboxMesh->SetMaterial(skyboxMat);
  skyboxMesh->CreateGPUResource(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                mSkyboxShader.GetSettings().GetSRVSetting());

  mSkybox.AddMesh(skyboxMesh);
  mSkybox.CreateShaderInfo(mGraphics->mD3dDevice.Get(), mSkyboxShader.GetSettings());

  IMesh* mesh = Geometry::GenerateBox(2, 2, 2);
  Material material;
  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks.dds"), material.Textures[CTEXT("gAlbedoMap")].Resource,
                                  material.Textures[CTEXT("gAlbedoMap")].ResourceUpload));

  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks_nmap.dds"), material.Textures[CTEXT("gNormalMap")].Resource,
                                  material.Textures[CTEXT("gNormalMap")].ResourceUpload));

  mesh->SetMaterial(material);
  mesh->CreateGPUResource(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(), mBoxShader.GetSettings().GetSRVSetting());

  models.push_back(new Model());
  models.push_back(new Model());

  models[0]->SetPosition(1.05f, 0.0f, 0.0f);
  models[1]->SetPosition(-1.05f, 0.0f, 0.0f);

  MaterialDesc matDesc;
  matDesc.DiffuseAlbedo = {0.01f, 0.01f, 0.01f, 1.0f};
  matDesc.FresnelR0     = {0.04f, 0.04f, 0.04f};
  matDesc.Roughness     = 0.6f;

  for (auto model : models) {
    model->AddMesh(mesh);
    model->CreateShaderInfo(mGraphics->mD3dDevice.Get(), mBoxShader.GetSettings());
    model->SetMaterialDesc(matDesc);
  }

  mLight.strength     = {1.0f, 1.0f, 1.0f};
  mLight.falloffStart = 1.0f;
  mLight.direction    = {0.0f, 1.0f, 0.0f};
  mLight.falloffEnd   = 10.0f;
  mLight.position     = {0.0f, 2.0f, 0.2f};
  mLight.SpotPower    = 64.0f;

  BuildPSO();

  mGraphics->ExecuteCommandList();
  mGraphics->FlushCommandQueue();

  mCamera.LookAt(XMFLOAT3(0.0f, 2.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f));
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  mSkybox.GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(mSkybox.GetTransformMatrix()));
  mSkybox.GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  for (auto model : models) {
    model->GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
  }

  return true;
}

void RenderExample::Draw()
{
  // Reuse the memroy associated with command recording.
  // We can only reset when the associated command lists have finished execution on the GPU.
  TIFF(mGraphics->mDirectCmdListAlloc->Reset());
  TIFF(mGraphics->mCommandList->Reset(mGraphics->mDirectCmdListAlloc.Get(), mPSOs[CTEXT("StandardPSO")].Get()));

  // A command list can be reset after it has been added to the command queue via ExecutedCommandList.
  // Reusing the command list reuses memory.

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

  DrawScene();

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("SkyboxPSO")].Get());
  mGraphics->mCommandList->SetGraphicsRootSignature(mSkyboxShader.GetRootSignature());
  for (auto& mesh : mSkybox.GetMeshes()) {
    ID3D12DescriptorHeap* descriptorHeaps[] = {mesh->GetSrvDescriptor()};
    mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    D3D12_INDEX_BUFFER_VIEW iBufferView = mesh->GetIndexBufferView();
    mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);

    D3D12_VERTEX_BUFFER_VIEW vBufferView = mesh->GetVertexBufferView();
    mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);
    mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const auto& pair : mSkybox.GetShaderInfo().GetCBuffers()) {
      auto cbuffer       = pair.second;
      const auto& cbInfo = cbuffer.GetCBufferInfo();
      mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(),
                                                                 cbuffer.mUploadBuffer->GetGPUVirtualAddress());
    }

    for (auto pair : mSkyboxShader.GetSettings().GetSRVSetting()) {
      auto srvSetting = pair.second;
      CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mesh->GetSrvDescriptor()->GetGPUDescriptorHandleForHeapStart());
      tex.Offset(srvSetting.GetSlot(), mGraphics->mCbvSrvUavDescriptorSize);
      // offset cbuffer paramter index.
      uint32 slot = srvSetting.GetSlot() + mSkyboxShader.GetSettings().GetCBSettingCount();
      mGraphics->mCommandList->SetGraphicsRootDescriptorTable(slot, tex);
    }

    mGraphics->mCommandList->DrawIndexedInstanced(static_cast<UINT>(mesh->GetIndexCount()), 1, 0, 0, 0);
  }

  // Indicate a state tansition on the resource usage.
  backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mGraphics->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
  mGraphics->mCommandList->ResourceBarrier(1, &backBufferBarrier);

  mGraphics->ExecuteCommandList();

  // swap the back and front buffers
  TIFF(mGraphics->mSwapChain->Present(0, 0));
  mGraphics->mCurrBackBuffer = (mGraphics->mCurrBackBuffer + 1) % mGraphics->SwapChainBufferCount;

  // Wait until frame commands are complete. This waiting is inefficient and is
  // done for simplicity. Later we will show show how to organize our rendering code
  // so we do not have to wait per frame.
  mGraphics->FlushCommandQueue();
}

void RenderExample::DrawScene()
{
  mGraphics->mCommandList->SetGraphicsRootSignature(mBoxShader.GetRootSignature());

  for (auto model : models) {
    for (auto& mesh : model->GetMeshes()) {
      ID3D12DescriptorHeap* descriptorHeaps[] = {mesh->GetSrvDescriptor()};
      mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
      D3D12_INDEX_BUFFER_VIEW iBufferView = mesh->GetIndexBufferView();
      mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);

      D3D12_VERTEX_BUFFER_VIEW vBufferView = mesh->GetVertexBufferView();
      mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);
      mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      for (const auto& pair : model->GetShaderInfo().GetCBuffers()) {
        auto cbuffer       = pair.second;
        const auto& cbInfo = cbuffer.GetCBufferInfo();
        mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(),
                                                                   cbuffer.mUploadBuffer->GetGPUVirtualAddress());
      }

      for (auto pair : mBoxShader.GetSettings().GetSRVSetting()) {
        auto srvSetting = pair.second;
        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mesh->GetSrvDescriptor()->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(srvSetting.GetSlot(), mGraphics->mCbvSrvUavDescriptorSize);
        // offset cbuffer paramter index.
        mGraphics->mCommandList->SetGraphicsRootDescriptorTable(
            srvSetting.GetSlot() + mBoxShader.GetSettings().GetCBSettingCount(), tex);
      }

      mGraphics->mCommandList->DrawIndexedInstanced(static_cast<UINT>(mesh->GetIndexCount()), 1, 0, 0, 0);
    }
  }
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
  static float rotTime = 0.0f;
  rotTime += dt;

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

  mLight.position.x = sin(rotTime);

  mSkybox.GetShaderInfo().SetFloat3(CTEXT("cbPerObject.gEyePosW"), mCamera.GetPostion());
  mSkybox.GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  for (auto model : models) {
    model->GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(model->GetTransformMatrix()));
    model->GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gInvWorld"), InverseTranspose(model->GetTransformMatrix()));

    model->GetShaderInfo().SetMaterialDesc(CTEXT("cbMaterial.gMaterial"), model->GetMaterialDesc());
    model->GetShaderInfo().SetLight(CTEXT("cbMaterial.gLight"), mLight);
    model->GetShaderInfo().SetFloat3(CTEXT("cbMaterial.gEyePosW"), mCamera.GetPostion());

    model->GetShaderInfo().SetMatrix(CTEXT("cbPerObject.gCameraTrans"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
  }
}

void RenderExample::BuildPSO()
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC standardPsoDesc;
  ZeroMemory(&standardPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  standardPsoDesc.InputLayout    = {Vertex::InputLayout.data(), (UINT)Vertex::InputLayout.size()};
  standardPsoDesc.pRootSignature = {mBoxShader.GetRootSignature()};
  standardPsoDesc.VS = {reinterpret_cast<BYTE*>(mBoxShader.GetVS()->GetBufferPointer()), mBoxShader.GetVS()->GetBufferSize()};
  standardPsoDesc.PS = {reinterpret_cast<BYTE*>(mBoxShader.GetPS()->GetBufferPointer()), mBoxShader.GetPS()->GetBufferSize()};
  standardPsoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  standardPsoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  standardPsoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  standardPsoDesc.SampleMask            = UINT_MAX;
  standardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  standardPsoDesc.NumRenderTargets      = 1;
  standardPsoDesc.RTVFormats[0]         = mGraphics->mBackBufferFormat;
  standardPsoDesc.SampleDesc.Count      = mGraphics->m4xMsaaState ? 4 : 1;
  standardPsoDesc.SampleDesc.Quality    = mGraphics->m4xMsaaState ? (mGraphics->m4xMsaaQuality - 1) : 0;
  standardPsoDesc.DSVFormat             = mGraphics->mDepthStencilFormat;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&standardPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("StandardPSO")])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = standardPsoDesc;
  skyPsoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  skyPsoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  skyPsoDesc.pRootSignature                     = mSkyboxShader.GetRootSignature();
  skyPsoDesc.VS = {reinterpret_cast<BYTE*>(mSkyboxShader.GetVS()->GetBufferPointer()), mSkyboxShader.GetVS()->GetBufferSize()};
  skyPsoDesc.PS = {reinterpret_cast<BYTE*>(mSkyboxShader.GetPS()->GetBufferPointer()), mSkyboxShader.GetPS()->GetBufferSize()};
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("SkyboxPSO")])));
}

DEFINE_APPLICATION_MAIN(RenderExample)