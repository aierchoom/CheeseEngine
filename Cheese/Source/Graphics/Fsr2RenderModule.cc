#include "Fsr2RenderModule.h"
#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

float CalculateMipBias(float upscalerRatio) { return std::log2f(1.f / upscalerRatio) - 1.f + std::numeric_limits<float>::epsilon(); }

ResolutionInfo UpdateResolution(int32 displayWidth, int32 displayHeight, float upscaleRatio)
{
  return {static_cast<int32>((float)displayWidth / upscaleRatio), static_cast<int32>((float)displayHeight / upscaleRatio), displayWidth, displayHeight};
}

inline FfxSurfaceFormat GetFfxSurfaceFormat(const DXGI_FORMAT& format)
{
  switch (format) {
    case (DXGI_FORMAT_R8G8B8A8_UNORM):
      return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (DXGI_FORMAT_R16G16_FLOAT):
      return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case (DXGI_FORMAT_D32_FLOAT_S8X24_UINT):
      return FFX_SURFACE_FORMAT_R32_FLOAT;
    default:
      FFX_ASSERT_MESSAGE(false, "ValidationRemap: Unsupported format requested. Please implement.");
      return FFX_SURFACE_FORMAT_UNKNOWN;
  }
}

bool IsDepth(const DXGI_FORMAT& format) { return (format == DXGI_FORMAT_D32_FLOAT); }

inline FfxResourceDescription GetFfxResourceDescription(ID3D12Resource* pResource, FfxResourceUsage additionalUsages = {})
{
  FfxResourceDescription resourceDescription = {};

  // This is valid
  if (!pResource) return resourceDescription;

  D3D12_RESOURCE_DESC texDesc = pResource->GetDesc();

  // Set flags properly for resource registration
  resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
  resourceDescription.usage = IsDepth(texDesc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
  // if (static_cast<bool>(texDesc.Flags & cauldron::ResourceFlags::AllowUnorderedAccess))
  //   resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

  resourceDescription.width    = texDesc.Width;
  resourceDescription.height   = texDesc.Height;
  resourceDescription.depth    = texDesc.DepthOrArraySize;
  resourceDescription.mipCount = texDesc.MipLevels;
  resourceDescription.format   = GetFfxSurfaceFormat(texDesc.Format);

  resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | additionalUsages);

  resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;

  return resourceDescription;
}

void Fsr2RenderModule::Init(ID3D12Device* device, const ResolutionInfo& resolution)
{
  m_Device     = device;
  m_Resolution = resolution;

  EnableModule();
}

void Fsr2RenderModule::EnableModule()
{
  // Setup Cauldron FidelityFX interface.
  const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR2_CONTEXT_COUNT);
  void* scratchBuffer            = malloc(scratchBufferSize);
  FfxErrorCode errorCode         = ffxGetInterfaceDX12(&m_InitializationParameters.backendInterface, m_Device, scratchBuffer, scratchBufferSize, FFX_FSR2_CONTEXT_COUNT);
  FFX_ASSERT(errorCode == FFX_OK);
  UpdateFSR2Context(true);
}
void FfxMsgCallback(FfxMsgType type, const wchar_t* message) { OutputDebugStringW(message); }

void Fsr2RenderModule::UpdateFSR2Context(bool enabled)
{
  if (enabled) {
    m_InitializationParameters.maxRenderSize.width  = m_Resolution.RenderWidth;
    m_InitializationParameters.maxRenderSize.height = m_Resolution.RenderHeight;
    m_InitializationParameters.displaySize.width    = m_Resolution.DisplayWidth;
    m_InitializationParameters.displaySize.height   = m_Resolution.DisplayHeight;

    m_InitializationParameters.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

    static bool s_InvertedDepth = false;

#if defined(_DEBUG)
    m_InitializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
    m_InitializationParameters.fpMessage = &FfxMsgCallback;
#endif  // #if defined(_DEBUG)

    FfxErrorCode errorCode = ffxFsr2ContextCreate(&m_FSR2Context, &m_InitializationParameters);
    FFX_ASSERT(errorCode == FFX_OK);
  } else {
    // Destroy the FSR2 context
    ffxFsr2ContextDestroy(&m_FSR2Context);
  }
}

void Fsr2RenderModule::OnResize(const ResolutionInfo& resInfo)
{
  m_Resolution = resInfo;
  m_JitterIndex = 0;
  UpdateFSR2Context(false);  // Destroy
  UpdateFSR2Context(true);   // Re-create
}

void Fsr2RenderModule::Execute(double deltaTime, ID3D12CommandList* pCmdList, ID3D12Resource* renderTarget, ID3D12Resource* colorTarget, ID3D12Resource* depthTarget,
                               ID3D12Resource* motionVector, Camera& camera)
{
  // Increment jitter index for frame
  ++m_JitterIndex;

  // Update FSR2 jitter for built in TAA
  float jitterX, jitterY;
  const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(m_Resolution.RenderWidth, m_Resolution.DisplayWidth);
  ffxFsr2GetJitterOffset(&jitterX, &jitterY, m_JitterIndex, jitterPhaseCount);
  camera.SetJitterValues({-2.f * jitterX / m_Resolution.RenderWidth, 2.f * jitterY / m_Resolution.RenderHeight});

  // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource |
  // ResourceState::PixelShaderResource)
  FfxFsr2DispatchDescription dispatchParameters = {};
  dispatchParameters.commandList                = ffxGetCommandListDX12(pCmdList);
  dispatchParameters.color         = ffxGetResourceDX12(colorTarget, GetFfxResourceDescription(colorTarget), L"FSR2_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  dispatchParameters.depth         = ffxGetResourceDX12(depthTarget, GetFfxResourceDescription(depthTarget), L"FSR2_InputDepth", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  dispatchParameters.motionVectors = ffxGetResourceDX12(motionVector, GetFfxResourceDescription(motionVector), L"FSR2_InputMotionVectors", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  dispatchParameters.exposure      = ffxGetResourceDX12(nullptr, {}, L"FSR2_InputExposure", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  dispatchParameters.output        = ffxGetResourceDX12(renderTarget, GetFfxResourceDescription(renderTarget), L"FSR2_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

  dispatchParameters.reactive                   = ffxGetResourceDX12(nullptr, {}, L"FSR2_EmptyInputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  dispatchParameters.transparencyAndComposition = ffxGetResourceDX12(nullptr, {}, L"FSR2_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

  dispatchParameters.jitterOffset.x      = jitterX;
  dispatchParameters.jitterOffset.y      = jitterY;
  dispatchParameters.motionVectorScale.x = m_Resolution.RenderWidth;
  dispatchParameters.motionVectorScale.y = m_Resolution.RenderHeight;
  dispatchParameters.reset               = false;
  dispatchParameters.enableSharpening    = m_RCASSharpen;
  dispatchParameters.sharpness           = m_Sharpness;

  dispatchParameters.frameTimeDelta = static_cast<float>(deltaTime * 1000.f);

  dispatchParameters.preExposure       = 1.0f;
  dispatchParameters.renderSize.width  = m_Resolution.RenderWidth;
  dispatchParameters.renderSize.height = m_Resolution.RenderHeight;

  dispatchParameters.cameraFovAngleVertical = camera.GetFovY();

  dispatchParameters.cameraFar  = camera.GetNearZ();
  dispatchParameters.cameraNear = camera.GetNearZ();

  FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_FSR2Context, &dispatchParameters);
  FFX_ASSERT(errorCode == FFX_OK);
}