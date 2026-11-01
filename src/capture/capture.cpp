#include "superior/capture/capture.h"
#include "superior/capture/windows.h"
#include "superior/core/error.h"
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <inspectable.h>
#include <unknwn.h>

// Windows SDK interop interface for retrieving the WGC surface's DXGI texture.
MIDL_INTERFACE("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1") IDirect3DDxgiInterfaceAccess : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** object) = 0;
};

namespace superior::capture {
namespace winrt_capture = winrt::Windows::Graphics::Capture;
namespace winrt_dx = winrt::Windows::Graphics::DirectX;
using Microsoft::WRL::ComPtr;
Capture::Capture(ID3D11Device& device) {
    device_ = &device;
    device.GetImmediateContext(&context_);
    if (!context_) throw Error("GetImmediateContext returned no context");
    ComPtr<IDXGIDevice> dxgi;
    check_hr(device.QueryInterface(IID_PPV_ARGS(&dxgi)), "Query IDXGIDevice");
    winrt::com_ptr<IInspectable> inspectable;
    check_hr(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(), inspectable.put()), "CreateDirect3D11DeviceFromDXGIDevice");
    interop_device_ = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    event_.attach(checked(CreateEventW(nullptr, false, false, nullptr), "Create capture event"));
}
Capture::~Capture() { stop(); }
bool Capture::supported() {
    try {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
            L"Windows.Foundation.UniversalApiContract", 8) && winrt_capture::GraphicsCaptureSession::IsSupported();
    } catch (const winrt::hresult_error&) { return false; }
}
void Capture::create_pool(SIZE size) {
    if (size.cx < 1 || size.cy < 1 || size.cx > 16384 || size.cy > 16384) throw Error("Invalid capture dimensions");
    if (pool_) pool_.Recreate(interop_device_, winrt_dx::DirectXPixelFormat::B8G8R8A8UIntNormalized, 3, {size.cx,size.cy});
    else {
        pool_ = winrt_capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            interop_device_, winrt_dx::DirectXPixelFormat::B8G8R8A8UIntNormalized, 3, {size.cx,size.cy});
        frame_token_ = pool_.FrameArrived([this](auto&&, auto&&) {
            if (!SetEvent(event_.get())) OutputDebugStringA("Superior: capture SetEvent failed\n");
        });
    }
    size_ = size;
}
void Capture::start(HWND window) {
    stop();
    if (!supported()) throw Error("Windows.Graphics.Capture is unavailable. Use Windows 10 1903+ or Windows 11 with WGC enabled.");
    if (!IsWindow(window)) throw Error("The selected window no longer exists");
    check_win(GetWindowThreadProcessId(window,&process_) != 0, "Get capture process");
    target_ = window; closed_.store(false);
    try {
        const auto interop = winrt::get_activation_factory<winrt_capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        check_hr(interop->CreateForWindow(window, winrt::guid_of<winrt_capture::GraphicsCaptureItem>(),
            winrt::put_abi(item_)), "CreateForWindow");
        closed_token_ = item_.Closed([this](auto&&, auto&&) {
            closed_.store(true, std::memory_order_release);
            if (!SetEvent(event_.get())) OutputDebugStringA("Superior: capture close SetEvent failed\n");
        });
        const auto item_size=item_.Size(); create_pool({item_size.Width,item_size.Height});
        session_ = pool_.CreateCaptureSession(item_);
        if (auto session2=session_.try_as<winrt_capture::IGraphicsCaptureSession3>()) session2.IsCursorEnabled(false);
        // Keep the system capture border: disabling it can require restricted capabilities.
        session_.StartCapture();
    } catch (...) { stop(); throw; }
}
void Capture::release_texture() noexcept { srv_.Reset(); owned_.Reset(); }
void Capture::stop() noexcept {
    try { if (session_) { session_.Close(); session_ = nullptr; } } catch (...) {}
    try { if (pool_) { if (frame_token_) pool_.FrameArrived(frame_token_); pool_.Close(); pool_ = nullptr; } } catch (...) {}
    try { if (item_) { if (closed_token_) item_.Closed(closed_token_); item_ = nullptr; } } catch (...) {}
    frame_token_ = {}; closed_token_ = {}; target_ = nullptr; process_ = 0; size_ = {};
    release_texture();
    if (event_) ResetEvent(event_.get());
}
bool Capture::acquire() {
    if (!session_) return false;
    if (closed_.load() || !belongs_to(target_, process_)) { stop(); return false; }
    winrt_capture::Direct3D11CaptureFrame frame{nullptr};
    while (auto next = pool_.TryGetNextFrame()) {
        if (frame) frame.Close();
        frame = next;
    }
    if (!frame) return false;
    const auto content = frame.ContentSize();
    if (content.Width < 1 || content.Height < 1) { frame.Close(); return false; }
    if (content.Width != size_.cx || content.Height != size_.cy) {
        frame.Close(); release_texture(); create_pool({content.Width,content.Height}); return false;
    }
    try {
        const auto access = frame.Surface().as<IDirect3DDxgiInterfaceAccess>();
        ComPtr<ID3D11Texture2D> source;
        check_hr(access->GetInterface(IID_PPV_ARGS(&source)), "Get capture texture");
        D3D11_TEXTURE2D_DESC desc{}; source->GetDesc(&desc);
        if (desc.Width != static_cast<UINT>(size_.cx) || desc.Height != static_cast<UINT>(size_.cy) ||
            desc.MipLevels != 1 || desc.ArraySize != 1 || desc.SampleDesc.Count != 1 ||
            (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB))
            throw Error("Unsupported capture texture layout");
        if (!owned_) {
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.MiscFlags = 0; desc.CPUAccessFlags = 0; desc.Usage = D3D11_USAGE_DEFAULT;
            check_hr(device_->CreateTexture2D(&desc,nullptr,&owned_), "Create capture-owned texture");
            check_hr(device_->CreateShaderResourceView(owned_.Get(),nullptr,&srv_), "Create capture SRV");
        }
        // GPU-only ownership transfer. Closing the WGC frame before GPU completion is
        // unsafe with a reusable frame pool; CopyResource is ordered by the same context.
        context_->CopyResource(owned_.Get(),source.Get());
        frame.Close(); ++frames_; return true;
    } catch (...) { frame.Close(); throw; }
}
} // namespace superior::capture
