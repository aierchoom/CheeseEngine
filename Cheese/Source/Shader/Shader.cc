#include "Shader.h"

#include <vector>

#include "Graphics/D3DUtil.h"
#include "Utils/Log/Logger.h"

using namespace std;

void Shader::AddShader(const CheString& fileName, ShaderType type)
{
  switch (type) {
    case ShaderType::VERTEX_SHADER:
      AddVS(D3DUtil::CompileShader(fileName, nullptr, CTEXT("VS"), CTEXT("vs_5_1")).Get());
      GenerateShaderSettings(mVsByteCode.Get());
      break;
    case ShaderType::HULL_SHADER:
      AddHS(D3DUtil::CompileShader(fileName, nullptr, CTEXT("HS"), CTEXT("hs_5_1")).Get());
      GenerateShaderSettings(mHsByteCode.Get());
      break;
    case ShaderType::DOMAIN_SHADER:
      AddDS(D3DUtil::CompileShader(fileName, nullptr, CTEXT("DS"), CTEXT("ds_5_1")).Get());
      GenerateShaderSettings(mDsByteCode.Get());
      break;
    case ShaderType::GEOMETRY_SHADER:
      AddHS(D3DUtil::CompileShader(fileName, nullptr, CTEXT("GS"), CTEXT("hs_5_1")).Get());
      GenerateShaderSettings(mGsByteCode.Get());
      break;
    case ShaderType::PIXEL_SHADER:
      AddPS(D3DUtil::CompileShader(fileName, nullptr, CTEXT("PS"), CTEXT("ps_5_1")).Get());
      GenerateShaderSettings(mPsByteCode.Get());
      break;
  }
}

void Shader::AddVS(ID3DBlob* shaderByteCode) { mVsByteCode = shaderByteCode; }

void Shader::AddPS(ID3DBlob* shaderByteCode) { mPsByteCode = shaderByteCode; }

void Shader::AddGS(ID3DBlob* shaderByteCode) { mGsByteCode = shaderByteCode; }

void Shader::AddHS(ID3DBlob* shaderByteCode) { mHsByteCode = shaderByteCode; }

void Shader::AddDS(ID3DBlob* shaderByteCode) { mDsByteCode = shaderByteCode; }

void Shader::GenerateShaderSettings(ID3DBlob* shader)
{
  ComPtr<ID3D12ShaderReflection> shaderReflection;
  TIFF(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), __uuidof(ID3D12ShaderReflection),
                  reinterpret_cast<void**>(shaderReflection.GetAddressOf())));

  D3D12_SHADER_DESC shaderDesc;
  shaderReflection->GetDesc(&shaderDesc);
  ShaderType shaderType = static_cast<ShaderType>(1 << D3D12_SHVER_GET_TYPE(shaderDesc.Version));

  for (uint32 i = 0;; ++i) {
    D3D12_SHADER_INPUT_BIND_DESC shaderInputDesc;
    HRESULT hr = shaderReflection->GetResourceBindingDesc(i, &shaderInputDesc);

    // It wiil fail after reading, but it's not a failed call.
    if (FAILED(hr)) break;

    // Process construct buffer build.
    if (shaderInputDesc.Type == D3D_SIT_CBUFFER) {
      GenerateCBSettings(shaderInputDesc, shaderReflection->GetConstantBufferByName(shaderInputDesc.Name));
    }
    if (shaderInputDesc.Type == D3D_SIT_TEXTURE) {
      mSettings.SetSRVSettings(ConvertToCheString(shaderInputDesc.Name),
                               SRVInfo(shaderInputDesc.BindPoint, (D3D12_SRV_DIMENSION)shaderInputDesc.Dimension));
    }
  }
}

void Shader::GenerateCBSettings(D3D12_SHADER_INPUT_BIND_DESC bindDesc, ID3D12ShaderReflectionConstantBuffer* cbReflection)
{
  // Get the varible info in the cbuffer and create the mapping.
  D3D12_SHADER_BUFFER_DESC cbufferDescs{};
  HRESULT hr = cbReflection->GetDesc(&cbufferDescs);
  if (FAILED(hr)) {
    logger.Error(CTEXT("Can't get shader info"));
    TIFF(hr);
  }

  std::unordered_map<CheString, uint32> varOffsets;
  for (uint32 j = 0; j < cbufferDescs.Variables; ++j) {
    ID3D12ShaderReflectionVariable* cbufferVar = cbReflection->GetVariableByIndex(j);
    D3D12_SHADER_VARIABLE_DESC varDesc;
    TIFF(cbufferVar->GetDesc(&varDesc));
    CheString varName   = ConvertToCheString(varDesc.Name);
    varOffsets[varName] = varDesc.StartOffset;
  }

  CheString cbufferName = ConvertToCheString(bindDesc.Name);
  mSettings.SetCBSettings(cbufferName, CBufferInfo(bindDesc.BindPoint, cbufferDescs.Size, std::move(varOffsets)));
}

void Shader::CreateRootSignature(ID3D12Device* device)
{
  const uint32 cbufferCount  = mSettings.GetCBSettingCount();
  const uint32 srvCount      = mSettings.GetSRVSettingCount();
  const uint32 paramterCount = cbufferCount + srvCount;
  vector<CD3DX12_ROOT_PARAMETER> slotRootParameter(paramterCount);

  for (uint32 i = 0; i < cbufferCount; ++i) {
    slotRootParameter[i].InitAsConstantBufferView(i);
  }

  vector<CD3DX12_DESCRIPTOR_RANGE> srvTable(srvCount);
  for (uint32 i = 0; i < srvCount; ++i) {
    srvTable[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);
    // offset cbuffer paramter index.
    slotRootParameter[i + cbufferCount].InitAsDescriptorTable(1, &srvTable[i], D3D12_SHADER_VISIBILITY_PIXEL);
  }

  auto staticSampler = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(static_cast<UINT>(slotRootParameter.size()), slotRootParameter.data(),
                                          static_cast<UINT>(staticSampler.size()), staticSampler.data(),
                                          D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob         = nullptr;
  TIFF(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

  if (errorBlob != nullptr) {
    logger.Error(ConvertToCheString((char*)errorBlob->GetBufferPointer()));
  }

  TIFF(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                   IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Shader::BuildPassCBuffer(ID3D12Device* device)
{
  for (auto pair : mSettings.GetCBSetting()) {
    auto cbName = pair.first;
    auto cbInfo = pair.second;
    // Shader just save tag:PASS data.
    if (CBufferManager::CBufferConfig[cbName] == CBufferType::PASS) {
      mCBManager.AddCBuffer(device, cbName, cbInfo);
    }
  }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> Shader::GetStaticSamplers()
{
  const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,                                 // shaderRegister
                                              D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
                                              D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
                                              D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
                                              D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,                                  // shaderRegister
                                               D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
                                               D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
                                               D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
                                               D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,                                 // shaderRegister
                                               D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
                                               D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
                                               D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
                                               D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,                                  // shaderRegister
                                                D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
                                                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
                                                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
                                                D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4,                                // shaderRegister
                                                    D3D12_FILTER_ANISOTROPIC,         // filter
                                                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                                                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                                                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                                                    0.0f,                             // mipLODBias
                                                    8);                               // maxAnisotropy

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,                                       // shaderRegister
                                                     D3D12_FILTER_ANISOTROPIC,                // filter
                                                     D3D12_TEXTURE_ADDRESS_MODE_CLAMP,        // addressU
                                                     D3D12_TEXTURE_ADDRESS_MODE_CLAMP,        // addressV
                                                     D3D12_TEXTURE_ADDRESS_MODE_CLAMP,        // addressW
                                                     0.0f,                                    // mipLODBias
                                                     8);                                      // maxAnisotropy
  const CD3DX12_STATIC_SAMPLER_DESC shadow(6,                                                 // shaderRegister
                                           D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,  // filter
                                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressU
                                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressV
                                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressW
                                           0.0f,                                              // mipLODBias
                                           16,                                                // maxAnisotropy
                                           D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

  return {pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp, shadow};
}
