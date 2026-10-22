#pragma once
// Interface implemented by every supported game except Dark Souls Remastered, whose
// reader is the original code in GameReader. Same rules as GameReader: the module only
// receives a read-only process handle and may only call ReadProcessMemory on it.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

struct GameSnapshot;

class GameModule {
public:
    virtual ~GameModule() = default;

    // Resolves globals and offsets from a copy of the exe image (`base` = load address).
    // Appends human-readable lines for the Diagnostics tab to `log`.
    virtual bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t size, std::string& log) = 0;
    // Fills `s` (already reset, with s.game set). Returns false if nothing could be read.
    virtual bool Read(HANDLE proc, GameSnapshot& s) = 0;
    // Short version text for the Diagnostics tab and the header ("" = unknown).
    virtual std::string VersionLabel(uint32_t moduleSize) const { (void)moduleSize; return {}; }
    // Extra lines for the Diagnostics tab with values that are not part of GameSnapshot.
    virtual std::string LiveDiagnostics() const { return {}; }

    // --- Optional: games that may not always be opened (Elden Ring under Easy Anti-Cheat) ---
    enum class Access {
        Open,    // the process may be opened and read
        Wait,    // not yet; GameReader asks again two seconds later
        Never,   // don't touch the process for the rest of this run of the game
    };
    // Asked before the game process is opened and every two seconds while it runs, without any
    // handle to it. `reason` is shown in the header and the Diagnostics tab.
    virtual Access CheckAccess(DWORD pid, std::string& reason) { (void)pid; (void)reason; return Access::Open; }
    // Counting without any access to the game process (Elden Ring: the save file). Used while
    // CheckAccess says Never, or when the exe can't be analyzed.
    virtual bool CanReadWithoutProcess() const { return false; }
    virtual bool ReadWithoutProcess(GameSnapshot& s) { (void)s; return false; }
    // Header text while counting without the process ("pelo arquivo de save").
    virtual std::string WithoutProcessStatus() const { return {}; }
    // Called on every GameReader::Update while the game runs, with or without a handle.
    virtual void Poll() {}
};

// Every offset of `pattern` ("48 8B ? ? 05", '?' = any byte) inside `img`.
std::vector<size_t> FindPattern(const std::vector<uint8_t>& img, const char* pattern);
// True if `pattern` matches at offset `at` of `img`.
bool PatternAt(const std::vector<uint8_t>& img, size_t at, const char* pattern);
// Little-endian int32 at `at` (0 if out of range).
int32_t ImageI32(const std::vector<uint8_t>& img, size_t at);
