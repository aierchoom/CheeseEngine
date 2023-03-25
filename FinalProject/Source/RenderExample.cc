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
#include <Graphics/D3DUtil.h>
#include <Graphics/RenderData.h>
#include <Graphics/ShadowMap.h>
#include <Shader/Shader.h>
#include <Shader/ShaderResource.h>
#include <Model/ModelLoader.h>
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
  RenderExample() : mWindow(new CheeseWindow(1280, 720)), mGraphics(new Graphics()), mLight() {}

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
  void DrawRenderItem(RenderData& renderData, Shader* shader, bool drawBlend = false);

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

  Shader* mPBRShader;
  Shader* mSkyboxShader;
  Shader* mShadowShader;

  unordered_map<CheString, ComPtr<ID3D12PipelineState>> mPSOs;

  BoundingSphere mSceneBounds;

  float mTheta  = 1.5f * XM_PI;
  float mPhi    = XM_PIDIV4;
  float mRadius = 5.0f;

  RenderData* mRenderData;
  RenderData* mSkyboxRenderData;
  PointLight mLight;

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

  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gViewProj"), XMMatrixTranspose(mCamera.GetLocalToWorldMatrixXM()));
}

bool RenderExample::Load()
{
  mGraphics->ResetCommandList();

  logger.Info(CTEXT("Build Shader..."));
  mPBRShader = new Shader(CTEXT("PBRShader"));
  mPBRShader->AddShader(CTEXT("Shaders/PBR/PBR.hlsl"), ShaderType::VERTEX_SHADER);
  mPBRShader->AddShader(CTEXT("Shaders/PBR/PBR.hlsl"), ShaderType::PIXEL_SHADER);
  mPBRShader->CreateRootSignature(mGraphics->mD3dDevice.Get());
  mPBRShader->BuildPassCBuffer(mGraphics->mD3dDevice.Get());

  logger.Info(CTEXT("Build skybox shader..."));
  mSkyboxShader = new Shader(CTEXT("SkyboxShader"));
  mSkyboxShader->AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::VERTEX_SHADER);
  mSkyboxShader->AddShader(CTEXT("Shaders/Skybox/Skybox.hlsl"), ShaderType::PIXEL_SHADER);
  mSkyboxShader->CreateRootSignature(mGraphics->mD3dDevice.Get());
  mSkyboxShader->BuildPassCBuffer(mGraphics->mD3dDevice.Get());

  logger.Info(CTEXT("Build shadow shader..."));
  mShadowShader = new Shader(CTEXT("ShadowShader"));
  mShadowShader->AddShader(CTEXT("Shaders/Shadow/Shadow.hlsl"), ShaderType::VERTEX_SHADER);
  mShadowShader->AddShader(CTEXT("Shaders/Shadow/Shadow.hlsl"), ShaderType::PIXEL_SHADER);
  mShadowShader->CreateRootSignature(mGraphics->mD3dDevice.Get());
  mShadowShader->BuildPassCBuffer(mGraphics->mD3dDevice.Get());

  mSkyboxRenderData = new RenderData(mGraphics->mD3dDevice, mGraphics->mCommandList);
  mRenderData       = new RenderData(mGraphics->mD3dDevice, mGraphics->mCommandList);
  mSkyboxRenderData->AddShader(mSkyboxShader);
  mRenderData->AddShader(mPBRShader);
  mRenderData->AddShader(mShadowShader);

  IMesh* skyboxMesh = Geometry::GenerateBox(1, 1, 1);
  Material skyboxMat;
  TIFF(D3DUtil::CreateTexture2DFromDDS(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(),
                                       CTEXT("Resource/Texture/grasscube1024.dds"), skyboxMat.Textures[CTEXT("gCubeMap")],
                                       D3D12_SRV_DIMENSION_TEXTURECUBE));
  skyboxMesh->SetMaterial(skyboxMat);
  Model* skybox = new Model();
  skybox->AddMesh(skyboxMesh);
  mSkyboxRenderData->AddRenderItem(CTEXT("Skybox"), skybox);
  mSkyboxRenderData->BuildRenderData();

  XMMATRIX world = XMLoadFloat4x4(&Identity4x4());
  mSkyboxRenderData->GetItem(CTEXT("Skybox"))
      .GetPerObjectCBuffer(mSkyboxShader->GetName())
      .SetValue(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(world));

  IMesh* planeMesh = Geometry::GeneratePlane(5.0f, 5.0f);
  Material planeMaterial;
  TIFF(D3DUtil::CreateTexture2DFromDDS(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(), CTEXT("Resource/Texture/tile.dds"),
                                       planeMaterial.Textures[CTEXT("gAlbedoMap")]));

  TIFF(D3DUtil::CreateTexture2DFromDDS(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(), CTEXT("Resource/Texture/tile_nmap.dds"),
                                       planeMaterial.Textures[CTEXT("gNormalMap")]));

  planeMesh->SetMaterial(planeMaterial);

  Model* plane = new Model();
  plane->AddMesh(planeMesh);
  mRenderData->AddRenderItem(CTEXT("Plane"), plane);

  Model* flightHelmet = new Model();
  Model* boomBox      = new Model();
  ModelLoader::LoadGLTF(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(), CTEXT("Resource/Model/FlightHelmet/FlightHelmet.gltf"),
                        *flightHelmet);
  ModelLoader::LoadGLTF(mGraphics->mD3dDevice.Get(), mGraphics->mCommandList.Get(), CTEXT("Resource/Model/BoomBox/BoomBox.gltf"), *boomBox);

  mRenderData->AddRenderItem(CTEXT("FlightHelmet"), flightHelmet);
  mRenderData->AddRenderItem(CTEXT("BoomBox"), boomBox);

  mRenderData->BuildRenderData();
  mShadowMap->CreateShadowMapSrv(mRenderData->GetShadowMapHandleCPU());

  for (auto pair : mRenderData->GetRenderItems()) {
    auto itemName = pair.first;
    auto& ri      = mRenderData->GetItem(itemName);
    ri.GetPerObjectCBuffer(mPBRShader->GetName()).SetValue(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(world));
    ri.GetPerObjectCBuffer(mShadowShader->GetName()).SetValue(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(world));
  }

  MaterialDesc matDesc;
  matDesc.DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};
  matDesc.FresnelR0     = {0.0f, 0.0f, 0.0f};
  matDesc.Roughness     = 0.3f;
  mRenderData->GetItemPerObjectCB(CTEXT("Plane"), mPBRShader->GetName()).SetValue(CTEXT("cbPerObject.gMatDesc"), matDesc);

  matDesc.FresnelR0 = {0.16f, 0.16f, 0.16f};
  mRenderData->GetItemPerObjectCB(CTEXT("FlightHelmet"), mPBRShader->GetName()).SetValue(CTEXT("cbPerObject.gMatDesc"), matDesc);

  mRenderData->GetItem(CTEXT("FlightHelmet")).SetPosition(1.0f, 0.0f, 0.0f);
  mRenderData->GetItem(CTEXT("FlightHelmet")).SetScale(1.5f, 1.5f, 1.5f);
  XMMATRIX trans = XMLoadFloat4x4(&Identity4x4());

  mRenderData->GetItem(CTEXT("FlightHelmet"))
      .GetPerObjectCBuffer(mPBRShader->GetName())
      .SetValue(CTEXT("cbPerObject.gWorld"), XMMatrixTranspose(trans));

  mRenderData->SetCBValueWithItemTrans(CTEXT("FlightHelmet"), mPBRShader->GetName(), CTEXT("cbPerObject.gWorld"));
  mRenderData->SetCBValueWithItemTrans(CTEXT("FlightHelmet"), mShadowShader->GetName(), CTEXT("cbPerObject.gWorld"));

  matDesc.FresnelR0 = {0.16f, 0.16f, 0.16f};
  mRenderData->GetItemPerObjectCB(CTEXT("BoomBox"), mPBRShader->GetName()).SetValue(CTEXT("cbPerObject.gMatDesc"), matDesc);
  mRenderData->GetItem(CTEXT("BoomBox")).SetPosition(-1.0f, 0.5f, 0.0f);
  mRenderData->GetItem(CTEXT("BoomBox")).SetScale(30.0f, 30.0f, 30.0f);
  mRenderData->SetCBValueWithItemTrans(CTEXT("BoomBox"), mPBRShader->GetName(), CTEXT("cbPerObject.gWorld"));
  mRenderData->SetCBValueWithItemTrans(CTEXT("BoomBox"), mShadowShader->GetName(), CTEXT("cbPerObject.gWorld"));

  mLight.strength     = {3.0f, 3.0f, 3.0f};
  mLight.falloffStart = 1.0f;
  mLight.direction    = {0.6f, -0.6f, 0.0f};
  mLight.falloffEnd   = 10.0f;
  mLight.position     = {0.0f, 1.5f, 0.2f};
  mLight.SpotPower    = 64.0f;

  BuildPSO();

  mGraphics->ExecuteCommandList();
  mGraphics->FlushCommandQueue();

  mCamera.LookAt(XMFLOAT3(0.0f, 2.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f));
  mCamera.SetFrustum(XM_PI / 3, mWindow->GetAspectRatio(), 0.5f, 1000.0f);
  mCamera.SetViewPort(0.0f, 0.0f, (float)mWindow->GetWidth(), (float)mWindow->GetHeight());

  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));

  mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
  mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

  return true;
}

void RenderExample::Draw()
{
  TIFF(mGraphics->mDirectCmdListAlloc->Reset());
  TIFF(mGraphics->mCommandList->Reset(mGraphics->mDirectCmdListAlloc.Get(), mPSOs[CTEXT("StandardPSO")].Get()));

  mGraphics->mCommandList->RSSetViewports(1, &mShadowMap->GetViewport());
  mGraphics->mCommandList->RSSetScissorRects(1, &mShadowMap->GetScissorRect());

  // Change to DEPTH_WRITE.
  mGraphics->mCommandList->ResourceBarrier(
      1,
      &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
  mGraphics->mCommandList->ClearDepthStencilView(mShadowMap->GetDsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0,
                                                 nullptr);
  mGraphics->mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->GetDsv());

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("ShadowPSO")].Get());
  DrawRenderItem(*mRenderData, mShadowShader);

  mGraphics->mCommandList->ResourceBarrier(
      1,
      &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

  mGraphics->mCommandList->RSSetViewports(1, &mGraphics->mScreenViewport);
  mGraphics->mCommandList->RSSetScissorRects(1, &mGraphics->mScissorRect);

  CD3DX12_RESOURCE_BARRIER backBufferBarrier =
      CD3DX12_RESOURCE_BARRIER::Transition(mGraphics->CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
  mGraphics->mCommandList->ResourceBarrier(1, &backBufferBarrier);

  // Clear the back buffer and depth buffer.
  mGraphics->mCommandList->ClearRenderTargetView(mGraphics->CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
  mGraphics->mCommandList->ClearDepthStencilView(mGraphics->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0,
                                                 nullptr);

  // Specify the buffers we are going to render to.
  D3D12_CPU_DESCRIPTOR_HANDLE currBackBufferView = mGraphics->CurrentBackBufferView();
  D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView   = mGraphics->DepthStencilView();
  mGraphics->mCommandList->OMSetRenderTargets(1, &currBackBufferView, true, &depthStencilView);

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("StandardPSO")].Get());
  DrawRenderItem(*mRenderData, mPBRShader);

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("SkyboxPSO")].Get());
  DrawRenderItem(*mSkyboxRenderData, mSkyboxShader);

  mGraphics->mCommandList->SetPipelineState(mPSOs[CTEXT("TransparentPSO")].Get());
  DrawRenderItem(*mRenderData, mPBRShader, true);

  // Indicate a state tansition on the resource usage.
  backBufferBarrier =
      CD3DX12_RESOURCE_BARRIER::Transition(mGraphics->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
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

void RenderExample::DrawRenderItem(RenderData& renderData, Shader* shader, bool drawBlend)
{
  mGraphics->mCommandList->SetGraphicsRootSignature(shader->GetRootSignature());

  // Bind shader pass cbuffer.
  for (const auto& pair : shader->GetCBufferManager().GetCBuffers()) {
    auto cbuffer = pair.second;
    auto cbInfo  = cbuffer.GetCBufferInfo();
    mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(), cbuffer.GetResource()->GetGPUVirtualAddress());
  }

  for (auto pair : renderData.GetRenderItems()) {
    auto itemName = pair.first;
    auto& item    = renderData.GetRenderItems()[itemName];

    ID3D12DescriptorHeap* descriptorHeaps[] = {renderData.GetSrvDescriptorHeap()};
    mGraphics->mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    for (const auto& pair : renderData.GetItemPerObjectCB(itemName, shader->GetName()).GetCBuffers()) {
      auto cbuffer = pair.second;
      auto cbInfo  = cbuffer.GetCBufferInfo();
      mGraphics->mCommandList->SetGraphicsRootConstantBufferView(cbInfo.GetSlot(), cbuffer.GetResource()->GetGPUVirtualAddress());
    }

    D3D12_VERTEX_BUFFER_VIEW vBufferView = item.GetVertexBufferView();
    mGraphics->mCommandList->IASetVertexBuffers(0, 1, &vBufferView);
    mGraphics->mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_INDEX_BUFFER_VIEW iBufferView16 = item.GetIndexBufferView16();
    D3D12_INDEX_BUFFER_VIEW iBufferView32 = item.GetIndexBufferView32();

    for (auto arg : item.GetDrawArgs()) {
      if (arg.IsBlend != drawBlend) continue;
      D3D12_INDEX_BUFFER_VIEW iBufferView(arg.IndexFormat == DXGI_FORMAT_R16_UINT ? iBufferView16 : iBufferView32);
      mGraphics->mCommandList->IASetIndexBuffer(&iBufferView);

      for (auto pair : shader->GetSettings().GetSRVSetting()) {
        auto srvName    = pair.first;
        auto srvSetting = pair.second;

        // offset cbuffer paramter index.
        uint32 slot = srvSetting.GetSlot() + shader->GetSettings().GetCBSettingCount();
        if (srvName == CTEXT("gShadowMap")) {
          mGraphics->mCommandList->SetGraphicsRootDescriptorTable(slot, mRenderData->GetShadowMapHandleGPU());
          continue;
        }

        // Default bind null srv.
        uint32 srvIndex = mRenderData->GetNullSrvIndex();

        if (arg.DrawSrvs.find(srvName) != arg.DrawSrvs.end()) {
          srvIndex = arg.DrawSrvs[srvName].SrvIndex + item.GetSrvDescriptorOffset();
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(renderData.GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(srvIndex, mGraphics->mCbvSrvUavDescriptorSize);
        mGraphics->mCommandList->SetGraphicsRootDescriptorTable(slot, tex);
      }

      mGraphics->mCommandList->DrawIndexedInstanced(arg.IndexCount, 1, arg.StartIndexLocation, arg.BaseVertexLocation, 0);
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

  float lightRotSpeed = 0.1f;
  mLight.direction.x  = sin(rotTime * lightRotSpeed);
  mLight.direction.z  = cos(rotTime * lightRotSpeed);

  XMVECTOR lightDir  = XMLoadFloat3(&mLight.direction);
  XMVECTOR lightPos  = -2.0f * mSceneBounds.Radius * lightDir;
  XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
  XMVECTOR lightUp   = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

  // Transform bounding sphere to light space.
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

  XMMATRIX texTrans(0.5f, 0.0f, 0.0f, 0.0f,   // line 1
                    0.0f, -0.5f, 0.0f, 0.0f,  // line 2
                    0.0f, 0.0f, 1.0f, 0.0f,   // line 3
                    0.5f, 0.5f, 0.0f, 1.0f    // line 4
  );

  XMMATRIX lightViewProj   = lightView * lightProj;
  XMMATRIX shadowTransform = lightView * lightProj * texTrans;

  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gShadowTransform"), XMMatrixTranspose(shadowTransform));
  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gLight"), mLight);
  mPBRShader->GetCBufferManager().SetValue(CTEXT("cbPass.gEyePosW"), mCamera.GetPostion());
  mShadowShader->GetCBufferManager().SetValue(CTEXT("cbPass.gViewProj"), XMMatrixTranspose(lightViewProj));
  mSkyboxShader->GetCBufferManager().SetValue(CTEXT("cbPass.gViewProj"), XMMatrixTranspose(mCamera.GetViewProjMatrixXM()));
  mSkyboxShader->GetCBufferManager().SetValue(CTEXT("cbPass.gEyePosW"), mCamera.GetPostion());
}

void RenderExample::BuildPSO()
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC standardPsoDesc;
  ZeroMemory(&standardPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  standardPsoDesc.InputLayout     = {Vertex::InputLayout.data(), (UINT)Vertex::InputLayout.size()};
  standardPsoDesc.pRootSignature  = mPBRShader->GetRootSignature();
  standardPsoDesc.VS              = {reinterpret_cast<BYTE*>(mPBRShader->GetVS()->GetBufferPointer()), mPBRShader->GetVS()->GetBufferSize()};
  standardPsoDesc.PS              = {reinterpret_cast<BYTE*>(mPBRShader->GetPS()->GetBufferPointer()), mPBRShader->GetPS()->GetBufferSize()};
  standardPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  standardPsoDesc.BlendState      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
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
  skyPsoDesc.pRootSignature                     = mSkyboxShader->GetRootSignature();

  skyPsoDesc.VS = {reinterpret_cast<BYTE*>(mSkyboxShader->GetVS()->GetBufferPointer()), mSkyboxShader->GetVS()->GetBufferSize()};
  skyPsoDesc.PS = {reinterpret_cast<BYTE*>(mSkyboxShader->GetPS()->GetBufferPointer()), mSkyboxShader->GetPS()->GetBufferSize()};

  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("SkyboxPSO")])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc   = standardPsoDesc;
  shadowPsoDesc.RasterizerState.DepthBias            = 100000;
  shadowPsoDesc.RasterizerState.DepthBiasClamp       = 0.0f;
  shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
  shadowPsoDesc.pRootSignature                       = mShadowShader->GetRootSignature();

  shadowPsoDesc.VS = {reinterpret_cast<BYTE*>(mShadowShader->GetVS()->GetBufferPointer()), mShadowShader->GetVS()->GetBufferSize()};
  shadowPsoDesc.PS = {reinterpret_cast<BYTE*>(mShadowShader->GetPS()->GetBufferPointer()), mShadowShader->GetPS()->GetBufferSize()};

  // Shadow map pass does not have a render target.
  shadowPsoDesc.RTVFormats[0]    = DXGI_FORMAT_UNKNOWN;
  shadowPsoDesc.NumRenderTargets = 0;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("ShadowPSO")])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = standardPsoDesc;

  D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;

  transparencyBlendDesc.BlendEnable           = true;
  transparencyBlendDesc.LogicOpEnable         = false;
  transparencyBlendDesc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
  transparencyBlendDesc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
  transparencyBlendDesc.BlendOp               = D3D12_BLEND_OP_ADD;
  transparencyBlendDesc.SrcBlendAlpha         = D3D12_BLEND_ONE;
  transparencyBlendDesc.DestBlendAlpha        = D3D12_BLEND_ZERO;
  transparencyBlendDesc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
  transparencyBlendDesc.LogicOp               = D3D12_LOGIC_OP_NOOP;
  transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
  TIFF(mGraphics->mD3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[CTEXT("TransparentPSO")])));
}

DEFINE_APPLICATION_MAIN(RenderExample)