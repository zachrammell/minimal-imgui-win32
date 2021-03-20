#pragma once

#include <d3d11.h>
#include <WindowsNumerics.h>
#include <winrt/base.h>
using namespace Windows::Foundation::Numerics;

namespace CS350
{
class OS_Win32;

class Render_DX11
{
public:
  Render_DX11(OS_Win32& os);
  ~Render_DX11();

  void SetClearColor(float3 c);

  void Present();
  void ResizeFramebuffer(OS_Win32 const& os);

  void ClearDefaultFramebuffer();

  /* stupid temporary hack section of the API */

  ID3D11Device* GetD3D11Device() const;
  ID3D11DeviceContext* GetD3D11Context() const;

private: /* ==== Raw DirectX resources ==== */
  winrt::com_ptr<IDXGISwapChain> swap_chain_;
  winrt::com_ptr<ID3D11Device> device_;
  winrt::com_ptr<ID3D11DeviceContext> device_context_;
  winrt::com_ptr<ID3D11RenderTargetView> render_target_view_;
  winrt::com_ptr<ID3D11DepthStencilView> depth_stencil_view_;

  winrt::com_ptr<ID3D11Texture2D> depth_stencil_buffer_;

  winrt::com_ptr<ID3D11DepthStencilState> depth_stencil_state_;
  winrt::com_ptr<ID3D11RasterizerState> rasterizer_state_;
  winrt::com_ptr<ID3D11RasterizerState> rasterizer_state_wireframe_;
  winrt::com_ptr<ID3D11SamplerState> sampler_state_;
  std::vector<ID3D11Buffer*> constant_buffers_;

  D3D11_VIEWPORT viewport_;
private:
  /* internal helpers */

  void SetupViewport(OS_Win32 const& os);
  void CleanupRenderTarget();
  void DeleteDepthBuffer();
  void CreateDepthBuffer(OS_Win32 const& os);
  void CreateRenderTarget();

  float3 clear_color_;
};

}
