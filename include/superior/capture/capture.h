#pragma once
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <winrt/base.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <atomic>
#include <cstdint>
#include <string>
namespace superior::capture {
class Capture {
public:
    explicit Capture(ID3D11Device& device);
    ~Capture();
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
    static bool supported();
    void start(HWND window);
    void stop() noexcept;
    bool acquire(); // false = no new frame; failures throw, except target closure returns false.
    bool closed() const noexcept { return closed_.load(std::memory_order_acquire); }
    bool active() const noexcept { return session_ != nullptr; }
    HANDLE event() const noexcept { return event_.get(); }
    ID3D11ShaderResourceView* texture() const noexcept { return srv_.Get(); }
    SIZE size() const noexcept { return size_; }
    HWND target() const noexcept { return target_; }
    std::uint64_t frames() const noexcept { return frames_; }
private:
    void create_pool(SIZE size);
    void release_texture() noexcept;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice interop_device_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr};
    winrt::event_token frame_token_{}, closed_token_{};
    winrt::handle event_;
    HWND target_ = nullptr;
    DWORD process_ = 0;
    SIZE size_{};
    std::atomic_bool closed_{false};
    std::uint64_t frames_ = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> owned_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;
};
} // namespace superior::capture
