#include "superior/present/device.h"
#include "superior/core/error.h"
namespace superior::present {
Device::Device() {
    constexpr D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if !defined(NDEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL selected{};
    HRESULT hr = D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,levels,2,
        D3D11_SDK_VERSION,&device_,&selected,&context_);
#if !defined(NDEBUG)
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
        hr = D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels,2,D3D11_SDK_VERSION,&device_,&selected,&context_);
#endif
    check_hr(hr,"D3D11CreateDevice");
    if (selected < D3D_FEATURE_LEVEL_11_0) throw Error("A Direct3D 11 feature-level GPU is required");
    Microsoft::WRL::ComPtr<ID3D11Multithread> mt;
    check_hr(context_.As(&mt),"Query ID3D11Multithread");
    check_win(mt->SetMultithreadProtected(TRUE),"SetMultithreadProtected");
}
std::uint64_t Device::luid() const noexcept {
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
    if (FAILED(device_.As(&dxgi))) return 0;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgi->GetAdapter(&adapter))) return 0;
    DXGI_ADAPTER_DESC desc{};
    if (FAILED(adapter->GetDesc(&desc))) return 0;
    return (static_cast<std::uint64_t>(desc.AdapterLuid.HighPart) << 32) | static_cast<unsigned>(desc.AdapterLuid.LowPart);
}
void Device::check_lost() const {
    const HRESULT hr = device_->GetDeviceRemovedReason();
    if (FAILED(hr)) throw Error("Graphics device lost; restart Superior after the driver recovers",hr);
}
} // namespace superior::present
