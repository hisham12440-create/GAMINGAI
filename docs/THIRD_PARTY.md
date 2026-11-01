# Third-party components

Build-time dependencies are declared in `cmake/Dependencies.cmake` with immutable commits and archive SHA-256 hashes.

| Component | Revision | Use | License |
|---|---|---|---|
| Dear ImGui | v1.91.9 / 97428e8ac99e339ce05eee531cf55b77b29ea709 | UI and bundled stb glyph rasterizer | MIT |
| nlohmann/json | v3.11.3 / 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 | Settings | MIT |
| GoogleTest | v1.15.2 / b514bdc898e2951020cbdca1304b75f5950d1f59 | CPU tests only | BSD-3-Clause |
| HarfBuzz | 10.2.0 / 7b27c8edd46c674e01dd226fa9e1aa7549f5c436 | OpenType shaping | HarfBuzz/MIT-style license |

The portable ZIP includes upstream license files. Windows SDK, Uniscribe, system fonts, Direct3D and WGC are platform components under Microsoft's terms; system fonts are not redistributed. No neural model weights or vendor SDKs are included in this stage.
