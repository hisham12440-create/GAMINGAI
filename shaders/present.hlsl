Texture2D<float4> Source : register(t0);
SamplerState LinearClamp : register(s0);
RWTexture2D<unorm float4> Destination : register(u0);
cbuffer ScaleConstants : register(b0) {
    float4 Dimensions; // source width/height, destination width/height
};
float4 sample_scaled(float2 pixel) {
    float2 scale = min(Dimensions.z / Dimensions.x, Dimensions.w / Dimensions.y);
    float2 extent = Dimensions.xy * scale;
    float2 uv = (pixel - (Dimensions.zw - extent) * 0.5) / extent;
    if (any(uv < 0.0) || any(uv > 1.0)) return float4(0, 0, 0, 1);
    return float4(Source.SampleLevel(LinearClamp, saturate(uv), 0).rgb, 1);
}
[numthreads(16, 16, 1)]
void scale_cs(uint3 id : SV_DispatchThreadID) {
    if (all(id.xy < asuint(Dimensions.zw))) Destination[id.xy] = sample_scaled(float2(id.xy) + 0.5);
}
struct VertexOutput { float4 position : SV_Position; float2 pixel : TEXCOORD0; };
VertexOutput fullscreen_vs(uint id : SV_VertexID) {
    VertexOutput o;
    float2 p = float2((id << 1) & 2, id & 2);
    o.position = float4(p * float2(2, -2) + float2(-1, 1), 0, 1);
    o.pixel = p * Dimensions.zw;
    return o;
}
float4 scale_ps(VertexOutput input) : SV_Target0 { return sample_scaled(input.pixel); }
