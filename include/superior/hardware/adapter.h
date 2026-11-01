#pragma once
#include <string>
#include <cstdint>
namespace superior::hardware {
struct AdapterInfo { std::wstring name; std::string vendor; std::uint64_t dedicated_video_bytes = 0; };
AdapterInfo describe_adapter(std::uint64_t adapter_luid);
} // namespace superior::hardware
