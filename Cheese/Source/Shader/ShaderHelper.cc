#include "ShaderHelper.h"

CBufferInfo::CBufferInfo() : CBufferInfo(UNINIT_SLOT_VALUE, 0, std::move(std::unordered_map<CheString, uint32>())) {}

CBufferInfo::CBufferInfo(uint32 slot, uint32 byteSize, const std::unordered_map<CheString, uint32>& varOffsets)
    : mSlot(slot), mByteSize(byteSize), mVariableOffsets(varOffsets)
{
  mByteSize = CalcAlignBufferByteSize(mByteSize);
}

CBufferInfo::CBufferInfo(uint32 slot, uint32 byteSize, std::unordered_map<CheString, uint32>&& varOffsets)
    : mSlot(slot), mByteSize(byteSize), mVariableOffsets(std::move(varOffsets))
{
  mByteSize = CalcAlignBufferByteSize(mByteSize);
}

CBufferInfo::CBufferInfo(const CBufferInfo& rhs) : CBufferInfo(rhs.mSlot, rhs.mByteSize, rhs.mVariableOffsets) {}

CBufferInfo::CBufferInfo(CBufferInfo&& rhs) noexcept : CBufferInfo(rhs.mSlot, rhs.mByteSize, std::move(rhs.mVariableOffsets)) {}

const CBufferInfo& CBufferInfo::operator=(CBufferInfo&& rhs) noexcept
{
  mSlot            = rhs.mSlot;
  mByteSize        = rhs.mByteSize;
  mVariableOffsets = std::move(rhs.mVariableOffsets);
  return (*this);
}

const CBufferInfo& CBufferInfo::operator=(const CBufferInfo& rhs)
{
  mSlot            = rhs.mSlot;
  mByteSize        = rhs.mByteSize;
  mVariableOffsets = rhs.mVariableOffsets;
  return (*this);
}

uint32 CBufferInfo::CalcAlignBufferByteSize(uint32 byteSize, uint32 alignSize)
{
  // Addition carry,clear low bit
  const uint32 clearBit = alignSize - 1;
  return (byteSize + (clearBit - 1)) & ~(clearBit - 1);
}

ShaderSettings::ShaderSettings() : mCBufferSettings(), mSRVSettings() {}

ShaderSettings::ShaderSettings(const std::unordered_map<CheString, CBufferInfo>& cbSettings,
                               const std::unordered_map<CheString, SRVInfo>& srvSettings)
    : mCBufferSettings(cbSettings), mSRVSettings(srvSettings)
{
}

ShaderSettings::ShaderSettings(std::unordered_map<CheString, CBufferInfo>&& cbSettings,
                               std::unordered_map<CheString, SRVInfo>&& srvSettings)
    : mCBufferSettings(std::move(cbSettings)), mSRVSettings(std::move(srvSettings))
{
}

ShaderSettings::ShaderSettings(const ShaderSettings& rhs) : ShaderSettings(rhs.mCBufferSettings, rhs.mSRVSettings) {}

ShaderSettings::ShaderSettings(ShaderSettings&& rhs) noexcept
    : ShaderSettings(std::move(rhs.mCBufferSettings), std::move(rhs.mSRVSettings))
{
}

ShaderSettings& ShaderSettings::operator=(const ShaderSettings& rhs)
{
  mCBufferSettings = rhs.mCBufferSettings;
  mSRVSettings     = rhs.mSRVSettings;
  return (*this);
}

ShaderSettings& ShaderSettings::operator=(ShaderSettings&& rhs) noexcept
{
  mCBufferSettings = std::move(rhs.mCBufferSettings);
  mSRVSettings     = std::move(rhs.mSRVSettings);
  return (*this);
}

void ShaderSettings::SetCBSettings(const CheString& name, const CBufferInfo& info)
{
  // If not exist,set it.
  if (mCBufferSettings.find(name) == mCBufferSettings.end()) {
    mCBufferSettings[name] = info;
  }
}

void ShaderSettings::SetSRVSettings(const CheString& name, const SRVInfo& info)
{
  // If not exist,set it.
  if (mSRVSettings.find(name) == mSRVSettings.end()) {
    mSRVSettings[name] = info;
  }
}

void ShaderSettings::SetCBSettings(const CheString& name, CBufferInfo&& info)
{
  // If not exist,set it.
  if (mCBufferSettings.find(name) == mCBufferSettings.end()) {
    mCBufferSettings[name] = std::move(info);
  }
}

void ShaderSettings::SetSRVSettings(const CheString& name, SRVInfo&& info)
{
  // If not exist,set it.
  if (mSRVSettings.find(name) == mSRVSettings.end()) {
    mSRVSettings[name] = std::move(info);
  }
}
