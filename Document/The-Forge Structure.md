# The Forge 结构分析

## Renderer
渲染器的抽象，一般包含几项资源
- IDXGIFactory GI工厂类
- IDXGIAdapter4 物理设备，dx12对显卡的抽象
- ID3D12Device 逻辑设备

## RenderTarget
- Texture
- 