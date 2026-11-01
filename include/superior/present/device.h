#pragma once
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
namespace superior::present {
class Device {
public:
    Device();
    ID3D11Device* get() const noexcept { return device_.Get(); }
    ID3D11DeviceContext* context() const noexcept { return context_.Get(); }
    std::uint64_t luid() const noexcept;
    void check_lost() const;
private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
};
} // namespace superior::present
