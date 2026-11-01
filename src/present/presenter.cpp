#include "superior/present/presenter.h"
#include "superior/core/error.h"
#include <fstream>
#include <vector>
#include <algorithm>
namespace superior::present {
namespace {
std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if (!f) throw Error("Missing compiled shader: " + path.string());
    const auto size=f.tellg();
    if (size<=0 || size>4*1024*1024) throw Error("Invalid compiled shader size");
    std::vector<unsigned char> bytes(static_cast<size_t>(size)); f.seekg(0);
    f.read(reinterpret_cast<char*>(bytes.data()),size);
    if (!f) throw Error("Cannot read compiled shader");
    return bytes;
}
}
Presenter::Presenter(Device& device,const std::filesystem::path& directory) : device_(device) {
    window_=std::make_unique<Window>(L"Superior.Overlay",L"Superior Overlay",WS_POPUP,
        WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_TOPMOST,640,360,window_proc,this);
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width=640;desc.Height=360;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount=3;desc.Scaling=DXGI_SCALING_NONE;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
    check_hr(device.get()->QueryInterface(IID_PPV_ARGS(&dxgi)),"Query DXGI device");
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    check_hr(dxgi->GetAdapter(&adapter),"Get presenting adapter");
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    check_hr(adapter->GetParent(IID_PPV_ARGS(&factory)),"Get DXGI factory");
    check_hr(factory->CreateSwapChainForHwnd(device.get(),window_->handle(),&desc,nullptr,nullptr,&swap_),
        "Create flip-model swap chain");
    check_hr(swap_->SetMaximumFrameLatency(1),"SetMaximumFrameLatency");
    waitable_.attach(swap_->GetFrameLatencyWaitableObject());
    if (!waitable_) throw Error("Swap chain returned no waitable handle");
    auto cs=read_bytes(directory/L"present.cso");
    check_hr(device.get()->CreateComputeShader(cs.data(),cs.size(),nullptr,&compute_),"Create scale compute shader");
    auto vs=read_bytes(directory/L"present_vs.cso");
    check_hr(device.get()->CreateVertexShader(vs.data(),vs.size(),nullptr,&vertex_),"Create fullscreen vertex shader");
    auto ps=read_bytes(directory/L"present_ps.cso");
    check_hr(device.get()->CreatePixelShader(ps.data(),ps.size(),nullptr,&pixel_),"Create fullscreen pixel shader");
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    check_hr(device.get()->CreateSamplerState(&sd,&sampler_),"Create sampler");
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    check_hr(device.get()->CreateBuffer(&bd,nullptr,&constants_),"Create scale constants");
    D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.ScissorEnable=FALSE;
    check_hr(device.get()->CreateRasterizerState(&rd,&rasterizer_),"Create rasterizer");
    UINT support=0;
    check_hr(device.get()->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM,&support),"Check UAV format support");
    compute_supported_=(support&D3D11_FORMAT_SUPPORT2_UAV_TYPED_STORE)!=0;
    resize(640,360);
}
LRESULT CALLBACK Presenter::window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,void*) noexcept {
    if (msg==WM_NCHITTEST) return HTTRANSPARENT;
    if (msg==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(hwnd,msg,wp,lp);
}
bool Presenter::visible() const noexcept { return window_ && IsWindowVisible(window_->handle()); }
void Presenter::hide() noexcept { if(window_) ShowWindow(window_->handle(),SW_HIDE); }
void Presenter::release_target() noexcept {
    auto* ctx=device_.context();
    if(ctx) { ctx->OMSetRenderTargets(0,nullptr,nullptr); ctx->CSSetUnorderedAccessViews(0,1,nullptr,nullptr); }
    uav_.Reset();rtv_.Reset();backbuffer_.Reset();
}
void Presenter::create_target() {
    check_hr(swap_->GetBuffer(0,IID_PPV_ARGS(&backbuffer_)),"Get swap-chain back buffer");
    check_hr(device_.get()->CreateRenderTargetView(backbuffer_.Get(),nullptr,&rtv_),"Create overlay RTV");
    if(compute_supported_) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_B8G8R8A8_UNORM;ud.ViewDimension=D3D11_UAV_DIMENSION_TEXTURE2D;
        check_hr(device_.get()->CreateUnorderedAccessView(backbuffer_.Get(),&ud,&uav_),"Create overlay UAV");
    }
}
void Presenter::resize(int width,int height) {
    if(width<1||height<1||width>16384||height>16384) throw Error("Invalid overlay size");
    if(size_.cx==width&&size_.cy==height&&backbuffer_) return;
    release_target();
    check_hr(swap_->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT),"Resize overlay");
    size_={width,height};create_target();
}
void Presenter::render(HWND target,ID3D11ShaderResourceView* source,SIZE source_size,bool click_through) {
    if(!source||!IsWindow(target)||IsIconic(target)) {hide();return;}
    RECT rect{};check_win(GetWindowRect(target,&rect),"Get target window rectangle");
    const int width=rect.right-rect.left,height=rect.bottom-rect.top;
    if(width<1||height<1) {hide();return;}
    resize(width,height);
    if(click_through!=click_through_) {
        SetLastError(ERROR_SUCCESS);
        const auto previous=GetWindowLongPtrW(window_->handle(),GWL_EXSTYLE);
        if(!previous&&GetLastError()) check_win(false,"Get overlay style");
        const auto style=click_through?(previous|WS_EX_TRANSPARENT):(previous&~WS_EX_TRANSPARENT);
        SetLastError(ERROR_SUCCESS);
        if(!SetWindowLongPtrW(window_->handle(),GWL_EXSTYLE,style)&&GetLastError()) check_win(false,"Set click-through");
        click_through_=click_through;
    }
    check_win(SetWindowPos(window_->handle(),HWND_TOPMOST,rect.left,rect.top,width,height,
        SWP_NOACTIVATE|SWP_SHOWWINDOW),"Position overlay");
    const DWORD wait=WaitForSingleObject(waitable_.get(),1000);
    if(wait!=WAIT_OBJECT_0) throw Error(wait==WAIT_TIMEOUT?"Overlay frame latency timeout":"Overlay frame wait failed");
    auto* ctx=device_.context();
    const float dimensions[]={static_cast<float>(source_size.cx),static_cast<float>(source_size.cy),
        static_cast<float>(width),static_cast<float>(height)};
    D3D11_MAPPED_SUBRESOURCE mapped{};
    check_hr(ctx->Map(constants_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),"Map scale constants");
    std::copy_n(dimensions,4,static_cast<float*>(mapped.pData));ctx->Unmap(constants_.Get(),0);
    ctx->CSSetShaderResources(0,1,&source);ctx->CSSetSamplers(0,1,&sampler_);ctx->CSSetConstantBuffers(0,1,&constants_);
    if(compute_supported_) {
        ctx->CSSetShader(compute_.Get(),nullptr,0);
        ID3D11UnorderedAccessView* uav=uav_.Get();ctx->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
        ctx->Dispatch((width+15)/16,(height+15)/16,1);
        ctx->CSSetUnorderedAccessViews(0,1,nullptr,nullptr);
    } else {
        // DX11 permits BGRA render targets even on older GPUs without BGRA typed UAV stores.
        ID3D11RenderTargetView* rtv=rtv_.Get();ctx->OMSetRenderTargets(1,&rtv,nullptr);
        D3D11_VIEWPORT vp{0,0,static_cast<float>(width),static_cast<float>(height),0,1};ctx->RSSetViewports(1,&vp);
        ctx->RSSetState(rasterizer_.Get());ctx->IASetInputLayout(nullptr);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(vertex_.Get(),nullptr,0);ctx->PSSetShader(pixel_.Get(),nullptr,0);
        ctx->PSSetShaderResources(0,1,&source);ctx->PSSetSamplers(0,1,&sampler_);ctx->PSSetConstantBuffers(0,1,&constants_);
        ctx->Draw(3,0);ctx->OMSetRenderTargets(0,nullptr,nullptr);
    }
    ctx->CSSetShaderResources(0,1,nullptr);ctx->PSSetShaderResources(0,1,nullptr);
    const HRESULT present=swap_->Present(1,0);
    if(present==DXGI_STATUS_OCCLUDED) {hide();return;}
    check_hr(present,"Present overlay");device_.check_lost();
}
} // namespace superior::present
