#pragma once
#include "superior/present/device.h"
#include "superior/app/window.h"
#include <filesystem>
#include <array>
#include <winrt/base.h>
namespace superior::present {
class Presenter {
public:
    Presenter(Device& device, const std::filesystem::path& shader_directory);
    void render(HWND target, ID3D11ShaderResourceView* source, SIZE source_size, bool click_through);
    void hide() noexcept;
    bool visible() const noexcept;
private:
    static LRESULT CALLBACK window_proc(HWND,UINT,WPARAM,LPARAM,void*) noexcept;
    void resize(int width,int height);
    void create_target();
    void release_target() noexcept;
    Device& device_;
    std::unique_ptr<Window> window_;
    Microsoft::WRL::ComPtr<IDXGISwapChain2> swap_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer_;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> compute_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
    winrt::handle waitable_;
    SIZE size_{};
    bool compute_supported_ = false;
    bool click_through_ = true;
};
} // namespace superior::present
