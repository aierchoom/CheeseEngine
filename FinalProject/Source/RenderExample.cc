#include <comdef.h>
#include <memory>
#include <vector>
#include <array>

#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <Core/CheeseApp.h>
#include <Core/Camera.h>
#include <Utils/GameTimer.h>
#include <Utils/Log/Logger.h>
#include <Input/InputConponent.h>
#include <Graphics/IGraphics.h>
#include <Graphics/ShadowMap.h>
#include <Graphics/DDSTextureLoader.h>
#include <Shader/Shader.h>
#include <Model/Model.h>
#include <Model/Geometry.h>

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
  BoundingSphere mSceneBounds;

  Shader mBoxShader;
  Shader mSkyboxShader;
  Shader mShadowShader;

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
}

bool RenderExample::Load()
{
  mGraphics->ResetCommandList();

  logger.Info(CTEXT("Build Shader..."));
  // mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::VERTEX_SHADER);
  // mBoxShader.AddShader(CTEXT("Shaders/color.hlsl"), ShaderType::PIXEL_SHADER);
  mBoxShader.AddShader(CTEXT("Shaders/SHD/Default.hlsl"), ShaderType::VERTEX_SHADER);
  mBoxShader.AddShader(CTEXT("Shaders/SHD/Default.hlsl"), ShaderType::PIXEL_SHADER);
  mBoxShader.CreateRootSignature(mGraphics->mD3dDevice.Get());

  logger.Info(CTEXT("Build skybox shader..."));
  mSkyboxShader.AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::VERTEX_SHADER);
  mSkyboxShader.AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::PIXEL_SHADER);
  mSkyboxShader.CreateRootSignature(mGraphics->mD3dDevice.Get());

  logger.Info(CTEXT("Build shadow shader..."));
  mShadowShader.AddShader(CTEXT("Shaders/Shadow/Shadow.hlsl"), ShaderType::VERTEX_SHADER);
  mShadowShader.AddShader(CTEXT("Shaders/Shadow/Shadow.hlsl"), ShaderType::PIXEL_SHADER);
  mShadowShader.CreateRootSignature(mGraphics->mD3dDevice.Get());

  IMesh* skyboxMesh = Geometry::GenerateBox(1, 1, 1);

  Material skyboxMat;
  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/grasscube1024.dds"), skyboxMat.Textures[CTEXT("gCubeMap")].Resource,
                                  skyboxMat.Textures[CTEXT("gCubeMap")].ResourceUpload));
  skyboxMesh->SetMaterial(skyboxMat);
  skyboxMesh->CreateGPUResource(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                mSkyboxShader.GetSettings().GetSRVSetting());

  mSkybox.AddMesh(skyboxMesh);
  mSkybox.AddShaderInfo(CTEXT("skybox"), mSkyboxShader.GetSettings(), mGraphics->mD3dDevice.Get());

  IMesh* boxMesh = Geometry::GenerateBox(1, 1, 1);
  Material boxMaterial;
  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks.dds"), boxMaterial.Textures[CTEXT("gAlbedoMap")].Resource,
                                  boxMaterial.Textures[CTEXT("gAlbedoMap")].ResourceUpload));

  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/bricks_nmap.dds"), boxMaterial.Textures[CTEXT("gNormalMap")].Resource,
                                  boxMaterial.Textures[CTEXT("gNormalMap")].ResourceUpload));

  boxMesh->SetMaterial(boxMaterial);
  boxMesh->CreateGPUResource(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                             mBoxShader.GetSettings().GetSRVSetting());
  Model* box = new Model();
  models.push_back(box);
  box->AddMesh(boxMesh);
  box->SetPosition(0.0f, 0.5f, 0.0f);

  IMesh* planeMesh = Geometry::GeneratePlane(10.0f, 10.0f);

  Material planeMaterial;
  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/tile.dds"), planeMaterial.Textures[CTEXT("gAlbedoMap")].Resource,
                                  planeMaterial.Textures[CTEXT("gAlbedoMap")].ResourceUpload));

  TIFF(CreateDDSTextureFromFile12(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                  CTEXT("Resource/Texture/tile_nmap.dds"), planeMaterial.Textures[CTEXT("gNormalMap")].Resource,
                                  planeMaterial.Textures[CTEXT("gNormalMap")].ResourceUpload));
  planeMesh->SetMaterial(planeMaterial);
  planeMesh->CreateGPUResource(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                               mBoxShader.GetSettings().GetSRVSetting());

  Model* plane = new Model();
  models.push_back(plane);
  plane->AddMesh(planeMesh);
  plane->SetPosition(0.0f, 0.0f, 0.0f);

  MaterialDesc matDesc;
  matDesc.DiffuseAlbedo = {0.7f, 0.7f, 0.7f, 1.0f};
  matDesc.FresnelR0     = {0.04f, 0.04f, 0.04f};
  matDesc.Roughness     = 0.6f;

  for (auto model : models) {
    model->AddShaderInfo(CTEXT("pbr"), mBoxShader.GetSettings(), mGraphics->mD3dDevice.Get());
    model->AddShaderInfo(CTEXT("shadow"), mShadowShader.GetSettings(), mGraphics->mD3dDevice.Get());
    model->SetMaterialDesc(matDesc);
  }

  mLight.strength     = {1.0f, 1.0f, 1.0f};
  mLight.falloffStart = 1.0f;
  mLight.direction    = {0.6f, -0.6f, 0.0f};
  mLight.falloffEnd   = 10.0f;
  mLight.position     = {0.0f, 2.0f, 0.2f};
  mLight.SpotPower    = 64.0f;

  BuildPSO();

  mGraphics->ExecuteCommandList();
  mGraphics->FlushCommandQueue();

  mCamera.LookAt(XMFLOAT3(0.0f, 2.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f));
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
  mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

  mSkybox.GetShaderInfo(CTEXT("skybox"))
      .SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(mSkybox.GetTransformMatrix()));
  mSkybox.GetShaderInfo(CTEXT("skybox"))
      .SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  for (auto model : models) {
    for (auto& mesh : model->GetMeshes()) {
      mShadowMap->BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE(mesh->GetSrvDescriptor()->GetCPUDescriptorHandleForHeapStart(),
                                                                 2, mGraphics->mCbvSrvUavDescriptorSize));
    }
    model->GetShaderInfo(CTEXT("pbr")).SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(model->GetTransformMatrix()));
    model->GetShaderInfo(CTEXT("pbr"))
        .SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

    model->GetShaderInfo(CTEXT("shadow"))
        .SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(model->GetTransformMatrix()));
  }

  return true;
}

void RenderExample::Draw()
{
  // Reuse the memroy associated with command recording.
  // We can only reset when the associated command lists have finished execution on the GPU.
  TIFF(mGraphics->mDirectCmdListAlloc->Reset());
  TIFF(mGraphics->mCommandList->Reset(mGraphics->mDirectCmdListAlloc.Get(), mPSOs[CTEXT("StandardPSO")].Get()));

  mGraphics->mCommandList->RSSetViewports(1, &mShadowMap->GetViewport());
  mGraphics->mCommandList->RSSetScissorRects(1, &mShadowMap->GetScissorRect());

  // Change to DEPTH_WRITE.
  mGraphics->mCommandList->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ,
                                               D3D12_RESOURCE_STATE_DEPTH_WRITE));
  mGraphics->mCommandList->SetGraphicsRootSignature(mShadowShader.GetRootSignature());

  mGraphics->mCommandList->ClearDepthStencilView(mShadowMap->GetDsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f,
                                                 0, 0, nullptr);
  mGraphics->mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->GetDsv());

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("ShadowPSO")].Get());

  for (auto model : models) {
    for (auto& mesh : model->GetMeshes()) {
      ID3D12DescriptorHeap* descriptorHeaps[] = {mesh->GetSrvDescriptor()};
      mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
      D3D12_INDEX_BUFFER_VIEW iBufferView = mesh->GetIndexBufferView();
      mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);

      D3D12_VERTEX_BUFFER_VIEW vBufferView = mesh->GetVertexBufferView();
      mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);
      mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      for (const auto& pair : model->GetShaderInfo(CTEXT("shadow")).GetCBuffers()) {
        auto cbuffer       = pair.second;
        const auto& cbInfo = cbuffer.GetCBufferInfo();
        mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(),
                                                                   cbuffer.mUploadBuffer->GetGPUVirtualAddress());
      }
      mGraphics->mCommandList->DrawIndexedInstanced(static_cast<UINT>(mesh->GetIndexCount()), 1, 0, 0, 0);
    }
  }

  mGraphics->mCommandList->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                               D3D12_RESOURCE_STATE_GENERIC_READ));

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
  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("StandardPSO")].Get());

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

    for (const auto& pair : mSkybox.GetShaderInfo(CTEXT("skybox")).GetCBuffers()) {
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

      for (const auto& pair : model->GetShaderInfo(CTEXT("pbr")).GetCBuffers()) {
        auto cbuffer       = pair.second;
        const auto& cbInfo = cbuffer.GetCBufferInfo();
        mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(),
                                                                   cbuffer.mUploadBuffer->GetGPUVirtualAddress());
      }

      for (auto pair : mBoxShader.GetSettings().GetSRVSetting()) {
        auto srvName    = pair.first;
        auto srvSetting = pair.second;
        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mesh->GetSrvDescriptor()->GetGPUDescriptorHandleForHeapStart());
        if (srvName == CTEXT("gShadowMap")) {
          mGraphics->mCommandList->SetGraphicsRootDescriptorTable(
              srvSetting.GetSlot() + mBoxShader.GetSettings().GetCBSettingCount(),
              tex.Offset(2, mGraphics->mCbvSrvUavDescriptorSize));
          continue;
        }
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

  mLight.direction.x = sin(rotTime * 0.5);
  mLight.direction.z = cos(rotTime * 0.3);

  // Only the first "main" light casts a shadow.
  XMVECTOR lightDir  = XMLoadFloat3(&mLight.direction);
  XMVECTOR lightPos  = -2.0f * mSceneBounds.Radius * lightDir;
  XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
  XMVECTOR lightUp   = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

  XMFLOAT3 sphereCenterLS;
  XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

  // Ortho frustum in light space encloses scene.
  float l = sphereCenterLS.x - mSceneBounds.Radius;
  float b = sphereCenterLS.y - mSceneBounds.Radius;
  float n = sphereCenterLS.z - mSceneBounds.Radius;
  float r = sphereCenterLS.x + mSceneBounds.Radius;
  float t = sphereCenterLS.y + mSceneBounds.Radius;
  float f = sphereCenterLS.z + mSceneBounds.Radius;

  XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

  // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
  XMMATRIX T(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f);

  XMMATRIX lightViewProj   = lightView * lightProj;
  XMMATRIX shadowTransform = lightView * lightProj * T;

  mSkybox.GetShaderInfo(CTEXT("skybox")).SetFloat3(CTEXT("cbPerObject.gEyePosW"), mCamera.GetPostion());
  mSkybox.GetShaderInfo(CTEXT("skybox"))
      .SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  for (auto model : models) {
    model->GetShaderInfo(CTEXT("pbr")).SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(model->GetTransformMatrix()));
    model->GetShaderInfo(CTEXT("pbr")).SetMaterialDesc(CTEXT("cbMaterial.gMaterial"), model->GetMaterialDesc());
    model->GetShaderInfo(CTEXT("pbr")).SetLight(CTEXT("cbMaterial.gLight"), mLight);
    model->GetShaderInfo(CTEXT("pbr")).SetFloat3(CTEXT("cbMaterial.gEyePosW"), mCamera.GetPostion());
    model->GetShaderInfo(CTEXT("pbr"))
        .SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
    model->GetShaderInfo(CTEXT("pbr")).SetMatrix(CTEXT("cbPerObject.gShadowTransform"), XMMatrixTranspose(shadowTransform));

    model->GetShaderInfo(CTEXT("shadow"))
        .SetMatrix(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(model->GetTransformMatrix()));
    model->GetShaderInfo(CTEXT("shadow")).SetMatrix(CTEXT("cbPerObject.gViewProj"), XMMatrixTranspose(lightViewProj));
  }
}

void RenderExample::BuildPSO()
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

  //
  // PSO for opaque objects.
  //
  ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  opaquePsoDesc.InputLayout    = {Vertex::InputLayout.data(), (UINT)Vertex::InputLayout.size()};
  opaquePsoDesc.pRootSignature = mBoxShader.GetRootSignature();
  opaquePsoDesc.VS = {reinterpret_cast<BYTE*>(mBoxShader.GetVS()->GetBufferPointer()), mBoxShader.GetVS()->GetBufferSize()};
  opaquePsoDesc.PS = {reinterpret_cast<BYTE*>(mBoxShader.GetPS()->GetBufferPointer()), mBoxShader.GetPS()->GetBufferSize()};
  opaquePsoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  opaquePsoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  opaquePsoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  opaquePsoDesc.SampleMask            = UINT_MAX;
  opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  opaquePsoDesc.NumRenderTargets      = 1;
  opaquePsoDesc.RTVFormats[0]         = mGraphics->mBackBufferFormat;
  opaquePsoDesc.SampleDesc.Count      = mGraphics->m4xMsaaState ? 4 : 1;
  opaquePsoDesc.SampleDesc.Quality    = mGraphics->m4xMsaaState ? (mGraphics->m4xMsaaQuality - 1) : 0;
  opaquePsoDesc.DSVFormat             = mGraphics->mDepthStencilFormat;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("StandardPSO")])));

  //
  // PSO for shadow map pass.
  //
  D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc   = opaquePsoDesc;
  smapPsoDesc.RasterizerState.DepthBias            = 100000;
  smapPsoDesc.RasterizerState.DepthBiasClamp       = 0.0f;
  smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
  smapPsoDesc.pRootSignature                       = mShadowShader.GetRootSignature();
  smapPsoDesc.VS = {reinterpret_cast<BYTE*>(mShadowShader.GetVS()->GetBufferPointer()), mShadowShader.GetVS()->GetBufferSize()};
  smapPsoDesc.PS = {reinterpret_cast<BYTE*>(mShadowShader.GetPS()->GetBufferPointer()), mShadowShader.GetPS()->GetBufferSize()};

  // Shadow map pass does not have a render target.
  smapPsoDesc.RTVFormats[0]    = DXGI_FORMAT_UNKNOWN;
  smapPsoDesc.NumRenderTargets = 0;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("ShadowPSO")])));

  //
  // PSO for sky.
  //
  D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

  // The camera is inside the sky sphere, so just turn off culling.
  skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

  // Make sure the depth function is LESS_EQUAL and not just LESS.
  // Otherwise, the normalized depth values at z = 1 (NDC) will
  // fail the depth test if the depth buffer was cleared to 1.
  skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  skyPsoDesc.pRootSignature              = mSkyboxShader.GetRootSignature();
  skyPsoDesc.VS = {reinterpret_cast<BYTE*>(mSkyboxShader.GetVS()->GetBufferPointer()), mSkyboxShader.GetVS()->GetBufferSize()};
  skyPsoDesc.PS = {reinterpret_cast<BYTE*>(mSkyboxShader.GetPS()->GetBufferPointer()), mSkyboxShader.GetPS()->GetBufferSize()};
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("SkyboxPSO")])));
}

DEFINE_APPLICATION_MAIN(RenderExample)