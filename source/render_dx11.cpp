#include "render_dx11.h"

#include <iostream>
#include <winrt/base.h>

#include "os_win32.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#define UNICODE
#include <Windows.h>

namespace dx = DirectX;

namespace CS350
{

Render_DX11::Render_DX11(OS_Win32& os)
: swap_chain_{ nullptr },
  device_{ nullptr },
  device_context_{ nullptr },
  render_target_view_{ nullptr },
  clear_color_{ 0, 0, 0 }
{
  os.AttachRender(this);

  HRESULT hr;
  DXGI_MODE_DESC buffer_description
  {
    UINT(os.GetWidth()),
    UINT(os.GetHeight()),
    // TODO: get monitor refresh rate
    {},
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
    DXGI_MODE_SCALING_UNSPECIFIED
  };

  DXGI_SWAP_CHAIN_DESC swap_chain_descriptor
  {
    buffer_description,
    DXGI_SAMPLE_DESC{1, 0},
    DXGI_USAGE_RENDER_TARGET_OUTPUT,
    1,
    os.GetWindowHandle(),
    true,
    DXGI_SWAP_EFFECT_SEQUENTIAL,
    0
  };

  hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION,
                                     &swap_chain_descriptor, swap_chain_.put(), device_.put(), nullptr,
                                     device_context_.put());
  assert(SUCCEEDED(hr));

  ResizeFramebuffer(os);

  D3D11_RASTERIZER_DESC rasterizer_descriptor
  {
    D3D11_FILL_SOLID,
    D3D11_CULL_BACK,
    true,
  };

  hr = device_->CreateRasterizerState(&rasterizer_descriptor, rasterizer_state_.put());
  assert(SUCCEEDED(hr));

  D3D11_RASTERIZER_DESC rasterizer_descriptor_wireframe
  {
    D3D11_FILL_WIREFRAME,
    D3D11_CULL_NONE,
    true,
  };

  hr = device_->CreateRasterizerState(&rasterizer_descriptor_wireframe, rasterizer_state_wireframe_.put());
  assert(SUCCEEDED(hr));

  // set up the default viewport to match the window
  {
    RECT window_rect;
    GetClientRect(os.GetWindowHandle(), &window_rect);
    viewport_ =
    {
      0.0f,
      0.0f,
      (FLOAT)(window_rect.right - window_rect.left),
      (FLOAT)(window_rect.bottom - window_rect.top),
      0.0f,
      1.0f
    };
    device_context_->RSSetViewports(1, &viewport_);
  }

  D3D11_SAMPLER_DESC sampler_desc{};
  sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 1;
  sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampler_desc.MinLOD = -FLT_MAX;
  sampler_desc.MaxLOD = FLT_MAX;

  hr = GetD3D11Device()->CreateSamplerState(&sampler_desc, sampler_state_.put());
  assert(SUCCEEDED(hr));
}

Render_DX11::~Render_DX11()
{
  swap_chain_->Release();
  device_->Release();
  device_context_->Release();
}

void Render_DX11::ClearDefaultFramebuffer()
{
  GetD3D11Context()->RSSetViewports(1, &viewport_);
  device_context_->ClearRenderTargetView(render_target_view_.get(), &(clear_color_.x));
  device_context_->ClearDepthStencilView(
    depth_stencil_view_.get(),
    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
    1.0f,
    0
  );
}

void Render_DX11::SetClearColor(float3 c)
{
  clear_color_ = {c.x, c.y, c.z};
}

void Render_DX11::Present()
{
  swap_chain_->Present(1, 0);
}

void Render_DX11::ResizeFramebuffer(OS_Win32 const& os)
{
  CleanupRenderTarget();
  DeleteDepthBuffer();
  CreateDepthBuffer(os);
  CreateRenderTarget();
  SetupViewport(os);

  // the windows api is not const-correct. it won't modify this,
  // but we still need a non-const pointer as our 1-element array
  auto* p_render_target_view = render_target_view_.get();
  device_context_->OMSetRenderTargets(1, &p_render_target_view, depth_stencil_view_.get());
  assert(p_render_target_view == render_target_view_.get()); // the pointer should be unchanged
}

ID3D11Device* Render_DX11::GetD3D11Device() const
{
  return device_.get();
}

ID3D11DeviceContext* Render_DX11::GetD3D11Context() const
{
  return device_context_.get();
}

void Render_DX11::SetupViewport(OS_Win32 const& os)
{
  //set up viewport
  viewport_.Width = os.GetWidth();
  viewport_.Height = os.GetHeight();
  viewport_.TopLeftX = 0;
  viewport_.TopLeftY = 0;
  viewport_.MinDepth = 0;
  viewport_.MaxDepth = 1;
  device_context_->RSSetViewports(1, &viewport_);
}

void Render_DX11::CleanupRenderTarget()
{
  if (render_target_view_)
  {
    device_context_->OMSetRenderTargets(0, 0, 0);
    render_target_view_.attach(nullptr);
  }
}

void Render_DX11::DeleteDepthBuffer()
{
  depth_stencil_view_.attach(nullptr);
  depth_stencil_buffer_.attach(nullptr);
  depth_stencil_state_.attach(nullptr);
}

void Render_DX11::CreateDepthBuffer(OS_Win32 const& os)
{
  //set up Depth/Stencil buffer
  D3D11_TEXTURE2D_DESC depth_stencil_buffer_desc;
  depth_stencil_buffer_desc.Width = os.GetWidth();
  depth_stencil_buffer_desc.Height = os.GetHeight();
  depth_stencil_buffer_desc.MipLevels = 1;
  depth_stencil_buffer_desc.ArraySize = 1;
  depth_stencil_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_stencil_buffer_desc.SampleDesc.Count = 1;
  depth_stencil_buffer_desc.SampleDesc.Quality = 0;
  depth_stencil_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
  depth_stencil_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depth_stencil_buffer_desc.CPUAccessFlags = 0;
  depth_stencil_buffer_desc.MiscFlags = 0;

  D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;

  // Depth test parameters
  depth_stencil_desc.DepthEnable = true;
  depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;

  // Stencil test parameters
  depth_stencil_desc.StencilEnable = true;
  depth_stencil_desc.StencilReadMask = 255;
  depth_stencil_desc.StencilWriteMask = 255;

  // Stencil operations if pixel is front-facing
  depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing
  depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  HRESULT hr;

  // Create depth stencil state
  hr = device_->CreateDepthStencilState(&depth_stencil_desc, depth_stencil_state_.put());
  assert(SUCCEEDED(hr));

  hr = device_->CreateTexture2D(&depth_stencil_buffer_desc, nullptr, depth_stencil_buffer_.put());
  assert(SUCCEEDED(hr));

  hr = device_->CreateDepthStencilView(depth_stencil_buffer_.get(), nullptr, depth_stencil_view_.put());
  assert(SUCCEEDED(hr));

  device_context_->OMSetDepthStencilState(depth_stencil_state_.get(), 0);
}

void Render_DX11::CreateRenderTarget()
{
  HRESULT hr;
  hr = swap_chain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, 0);
  assert(SUCCEEDED(hr));
  //get backbuffer from swap chain
  winrt::com_ptr<ID3D11Texture2D> back_buffer;
  hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), back_buffer.put_void());
  assert(SUCCEEDED(hr));

  hr = device_->CreateRenderTargetView(back_buffer.get(), nullptr, render_target_view_.put());
  assert(SUCCEEDED(hr));
}

}
