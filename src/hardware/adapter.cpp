#include "superior/hardware/adapter.h"
#include "superior/core/error.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
namespace superior::hardware {
using Microsoft::WRL::ComPtr;
AdapterInfo describe_adapter(std::uint64_t adapter_luid) {
    ComPtr<IDXGIFactory1> factory;
    check_hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");
    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        HRESULT hr = factory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        check_hr(hr, "EnumAdapters1");
        DXGI_ADAPTER_DESC1 desc{}; check_hr(adapter->GetDesc1(&desc), "GetDesc1");
        if (((static_cast<std::uint64_t>(desc.AdapterLuid.HighPart) << 32) |
            static_cast<unsigned>(desc.AdapterLuid.LowPart)) != adapter_luid) continue;
        AdapterInfo info{desc.Description, "Unknown", desc.DedicatedVideoMemory};
        switch (desc.VendorId) {
            case 0x10DE: info.vendor = "NVIDIA"; break;
            case 0x1002: case 0x1022: info.vendor = "AMD"; break;
            case 0x8086: info.vendor = "Intel"; break;
            default: break;
        }
        return info;
    }
    throw Error("Rendering adapter disappeared");
}
} // namespace superior::hardware
