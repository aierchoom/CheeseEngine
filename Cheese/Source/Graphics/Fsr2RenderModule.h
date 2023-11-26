#pragma once
#include <FidelityFX/host/ffx_fsr2.h>
#include <d3d12.h>
#include <cmath>
#include <limits>
#include <Core/Camera.h>
#include <Core/CoreMinimal.h>
#include <Graphics/IGraphics.h>

float CalculateMipBias(float upscalerRatio);

ResolutionInfo UpdateResolution(int32 displayWidth, int32 displayHeight, float upscaleRatio = 1.5f);

class Fsr2RenderModule
{
 public:
  void Init(ID3D12Device* device, const ResolutionInfo& resolution);

  void EnableModule();

  void OnResize(const ResolutionInfo& resInfo);

  void Execute(double deltaTime, ID3D12CommandList* pCmdList, ID3D12Resource* renderTarget, ID3D12Resource* colorTarget, ID3D12Resource* depthTarget, ID3D12Resource* motionVector,
               Camera& camera);

  // Enum representing the FSR 2 quality modes.
  enum class FSR2ScalePreset {
    Quality = 0,       // 1.5f
    Balanced,          // 1.7f
    Performance,       // 2.f
    UltraPerformance,  // 3.f
    Custom             // 1.f - 3.f range
  };

  // Enum representing the reactivity mask modes.
  enum class FSR2MaskMode { Disabled = 0, Manual, Auto };

  const float cMipBias[static_cast<uint32_t>(FSR2ScalePreset::Custom)] = {
      CalculateMipBias(1.5f),  //
      CalculateMipBias(1.7f),  //
      CalculateMipBias(2.0f),  //
      CalculateMipBias(3.0f)   //
  };

  void UpdateFSR2Context(bool enbale);

  FSR2ScalePreset m_ScalePreset = FSR2ScalePreset::Quality;
  float m_UpscaleRatio          = 2.f;
  float m_MipBias               = cMipBias[static_cast<uint32_t>(FSR2ScalePreset::Quality)];
  FSR2MaskMode m_MaskMode       = FSR2MaskMode::Manual;
  float m_Sharpness             = 0.8f;
  uint32_t m_JitterIndex        = 0;

  FfxFsr2ContextDescription m_InitializationParameters = {};
  FfxFsr2Context m_FSR2Context;

  bool m_UpscaleRatioEnabled = false;
  bool m_UseMask             = true;
  bool m_RCASSharpen         = true;

  ID3D12Device* m_Device;
  ResolutionInfo m_Resolution;
  ID3D12Resource* m_pRenderTarget  = nullptr;
  ID3D12Resource* m_pColorTarget   = nullptr;
  ID3D12Resource* m_pDepthTarget   = nullptr;
  ID3D12Resource* m_pMotionVectors = nullptr;
};
