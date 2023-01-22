#include "Shader.h"
#include "Core/CoreMinimal.h"
#include "Graphics/D3DUtil.h"

void Shader::AddShader(const CheString& fileName, ShaderType type)
{
  switch (type) {
    case ShaderType::VERTEX_SHADER:
      mVsByteCode.ReleaseAndGetAddressOf();
      mVsByteCode = D3DUtil::CompileShader(fileName, nullptr, CTEXT("VS"), CTEXT("vs_5_0"));
      break;
    case ShaderType::HULL_SHADER:
      mHsByteCode.ReleaseAndGetAddressOf();
      mHsByteCode = D3DUtil::CompileShader(fileName, nullptr, CTEXT("HS"), CTEXT("hs_5_0"));
      break;
    case ShaderType::DOMAIN_SHADER:
      mDsByteCode.ReleaseAndGetAddressOf();
      mDsByteCode = D3DUtil::CompileShader(fileName, nullptr, CTEXT("DS"), CTEXT("ds_5_0"));
      break;
    case ShaderType::GEOMETRY_SHADER:
      mHsByteCode.ReleaseAndGetAddressOf();
      mHsByteCode = D3DUtil::CompileShader(fileName, nullptr, CTEXT("GS"), CTEXT("hs_5_0"));
      break;
    case ShaderType::PIXEL_SHADER:
      mPsByteCode.ReleaseAndGetAddressOf();
      mPsByteCode = D3DUtil::CompileShader(fileName, nullptr, CTEXT("PS"), CTEXT("ps_5_0"));
      break;
  }
}

void Shader::GenerateShaderInfo(ID3D12Device* device)
{
  if (mVsByteCode != nullptr) {
    mShaderInfo->Generate(mVsByteCode.Get(), device);
  }
  if (mPsByteCode != nullptr) {
    mShaderInfo->Generate(mPsByteCode.Get(), device);
  }
  if (mGsByteCode != nullptr) {
    mShaderInfo->Generate(mGsByteCode.Get(), device);
  }
  if (mHsByteCode != nullptr) {
    mShaderInfo->Generate(mHsByteCode.Get(), device);
  }
  if (mDsByteCode != nullptr) {
    mShaderInfo->Generate(mDsByteCode.Get(), device);
  }
}

void Shader::AddVS(ID3DBlob* shaderByteCode)
{
  mVsByteCode->Release();
  mVsByteCode = shaderByteCode;
}

void Shader::AddPS(ID3DBlob* shaderByteCode)
{
  mPsByteCode->Release();
  mPsByteCode = shaderByteCode;
}

void Shader::AddGS(ID3DBlob* shaderByteCode)
{
  mGsByteCode->Release();
  mGsByteCode = shaderByteCode;
}

void Shader::AddHS(ID3DBlob* shaderByteCode)
{
  mHsByteCode->Release();
  mHsByteCode = shaderByteCode;
}

void Shader::AddDS(ID3DBlob* shaderByteCode)
{
  mDsByteCode->Release();
  mDsByteCode = shaderByteCode;
}