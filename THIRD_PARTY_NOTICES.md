# Third-party notices

DS Death Counter includes the following open source libraries. Both are compiled into `DSDeathCounter.exe`.

## Dear ImGui

- Source: https://github.com/ocornut/imgui (v1.91.8, in `third_party/imgui`)
- License: MIT. Copyright (c) 2014-2025 Omar Cornut
- Full text: [`third_party/imgui/LICENSE.txt`](third_party/imgui/LICENSE.txt)

## JSON for Modern C++

- Source: https://github.com/nlohmann/json (v3.11.3, in `third_party/nlohmann/json.hpp`)
- License: MIT. Copyright (c) 2013-2023 Niels Lohmann <https://nlohmann.me>
- The full license text is at the top of `json.hpp`.

## Community research (no code included)

The memory offsets and byte signatures used to read the game come from public research by the Souls modding and speedrunning community. No source code from these projects is included in DS Death Counter; they are credited because their work made it possible:

- [DSDeaths](https://github.com/quidrex/DSDeaths) by quidrex
- [SoulSplitter / SoulMemory](https://github.com/FrankvdStam/SoulSplitter) by Frank van der Stam (incl. work by CapitaineToinon)
- [DSR-Gadget](https://github.com/JKAnderson/DSR-Gadget) and [DS-Gadget](https://github.com/JKAnderson/DS-Gadget) by JKAnderson
- [DS2S-META](https://github.com/Nordgaren/DS2S-META) by Nordgaren: Dark Souls II signatures and offsets, and the bonfire ids, area ids and names used in `src/games/ds2_data.cpp` (data from its bonfire list; MIT License, Copyright (c) 2021 Norgaren)

---

DARK SOULS is a trademark of BANDAI NAMCO Entertainment Inc. / FromSoftware, Inc. This project is a fan-made tool and is not affiliated with, endorsed by or sponsored by them.
