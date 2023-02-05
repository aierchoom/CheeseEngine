#ifndef SHADER_SHADER_HELPER_H
#define SHADER_SHADER_HELPER_H
#include "Common/TypeDef.h"
#include <unordered_map>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

#define UNINIT_SLOT_VALUE -1
#define CONSTANT_BUFFER_ALIGN_SIZE 256

struct Light {
  DirectX::XMFLOAT3 strength;
  float falloffStart;
  DirectX::XMFLOAT3 direction;
  float falloffEnd;
  DirectX::XMFLOAT3 position;
  float SpotPower;
};

struct MaterialDesc {
  DirectX::XMFLOAT4 DiffuseAlbedo;
  DirectX::XMFLOAT3 FresnelR0;
  float Roughness;
};

class CBufferInfo
{
 public:
  CBufferInfo();
  CBufferInfo(uint32 slot, uint32 byteSize, const std::unordered_map<CheString, uint32>& varOffsets);
  CBufferInfo(uint32 slot, uint32 byteSize, std::unordered_map<CheString, uint32>&& varOffsets);
  CBufferInfo(const CBufferInfo& rhs);
  CBufferInfo(CBufferInfo&& rhs) noexcept;

  ~CBufferInfo() {}

  const CBufferInfo& operator=(const CBufferInfo& rhs);
  const CBufferInfo& operator=(CBufferInfo&& rhs) noexcept;

  inline uint32 GetSlot() const { return mSlot; }
  inline uint32 GetByteSize() const { return mByteSize; }
  inline std::unordered_map<CheString, uint32> GetVariableOffsets() const { return mVariableOffsets; }

  inline static uint32 CalcAlignBufferByteSize(uint32 byteSize, uint32 alignSize = CONSTANT_BUFFER_ALIGN_SIZE);

 private:
  uint32 mSlot;
  uint32 mByteSize;
  std::unordered_map<CheString, uint32> mVariableOffsets;
};

class SRVInfo
{
 public:
  SRVInfo() : SRVInfo(UNINIT_SLOT_VALUE, D3D12_SRV_DIMENSION_TEXTURE2D) {}
  SRVInfo(uint32 slot, D3D12_SRV_DIMENSION dimension) : mSlot(slot), mDimension(dimension) {}
  SRVInfo(const SRVInfo& rhs) : SRVInfo(rhs.mSlot, rhs.mDimension) {}

  ~SRVInfo() {}

  inline uint32 GetSlot() const { return mSlot; }
  inline D3D12_SRV_DIMENSION GetDimension() const { return mDimension; }

 private:
  uint32 mSlot;
  D3D12_SRV_DIMENSION mDimension;
};

class ShaderSettings
{
 public:
  ShaderSettings();
  ShaderSettings(const std::unordered_map<CheString, CBufferInfo>& cbSettings,
                 const std::unordered_map<CheString, SRVInfo>& srvSettings);
  ShaderSettings(std::unordered_map<CheString, CBufferInfo>&& cbSettings, std::unordered_map<CheString, SRVInfo>&& srvSettings);
  ShaderSettings(const ShaderSettings& rhs);
  ShaderSettings(ShaderSettings&& rhs) noexcept;

  ShaderSettings& operator=(const ShaderSettings& rhs);
  ShaderSettings& operator=(ShaderSettings&& rhs) noexcept;

  inline uint32 GetCBSettingCount() const { return mCBufferSettings.size(); }
  inline uint32 GetSRVSettingCount() const { return mSRVSettings.size(); }

  inline std::unordered_map<CheString, CBufferInfo> GetCBSetting() const { return mCBufferSettings; }
  inline std::unordered_map<CheString, SRVInfo> GetSRVSetting() const { return mSRVSettings; }

  void SetCBSettings(const CheString& name, const CBufferInfo& info);
  void SetSRVSettings(const CheString& name, const SRVInfo& info);
  void SetCBSettings(const CheString& name, CBufferInfo&& info);
  void SetSRVSettings(const CheString& name, SRVInfo&& info);

 private:
  std::unordered_map<CheString, CBufferInfo> mCBufferSettings;
  std::unordered_map<CheString, SRVInfo> mSRVSettings;
};

#endif  // SHADER_SHADER_HELPER_H