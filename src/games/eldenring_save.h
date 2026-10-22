#pragma once
// Elden Ring save file (ER0000.sl2) reader.
//
// Used while Easy Anti-Cheat protects the game: the app then never touches the game
// process and reads the death counter from the save, which the game writes after every
// death. The file is opened read-only, shared, for a few milliseconds, and never modified.
//
// Format: a BND4 container (not encrypted) with 10 character slots of 0x10 (MD5) +
// 0x280000 bytes, USER_DATA010 (profile summaries) and USER_DATA011. Inside a slot the
// death counter sits in a 0x1D-byte block between the tutorial data and the event flags.
// See docs/how-it-was-made.md for how it is located and validated.

#include <windows.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct ErSaveSlot {
    bool     used = false;        // the profile summary marks the slot as in use
    bool     valid = false;       // the death counter was located and the slot passed every check
    uint32_t version = 0;
    uint32_t mapId = 0;           // map the character is in (load screen map)
    uint64_t playTimeMs = 0;
    std::wstring name;            // from the profile summary
    int      level = 0;
    uint32_t deaths = 0;
    uint32_t lastGrace = 0;       // entity id of the last Site of Grace rested at
    bool     detailOk = false;    // the full sequential parse matched (NG+ and bloodstain below)
    int      ngPlus = -1;
    uint32_t bloodstainMap = 0;   // map of the latest bloodstain (where the character last died)
    bool     bloodstainActive = false;
    bool     bloodstainChanged = false; // moved since the previous read (a new death)
    uint8_t  bloodstain[0x44] = {};
    uint8_t  md5[16] = {};        // checksum stored in the file (changes whenever the game writes the slot)
    uint32_t deathsAt = 0, bloodstainAt = 0;   // where they were found in the slot (diagnostics/tests)
    std::string error;            // why the slot is not valid
};

struct ErSaveData {
    bool ok = false;              // container and USER_DATA010 are readable
    uint64_t steamId = 0;
    std::array<ErSaveSlot, 10> slots;
    std::string error;
};

// UTF-16 -> UTF-8 (names and paths in the Diagnostics text).
std::string ErUtf8(const std::wstring& w);

// Parses a whole ER0000.sl2 / .co2 file already in memory. Never throws.
bool ErParseSave(const std::vector<uint8_t>& file, ErSaveData& out);

// Watches the newest save file under %APPDATA%\EldenRing\<steamid>\ and re-parses it after
// the game writes it. Also works out which slot is being played: the one the game rewrites.
class ErSaveWatcher {
public:
    // Cheap; call often. Re-reads the file only after it changed and stayed unchanged for a moment.
    void Poll();
    void Reset();
    // When set, Seamless Co-op saves (ER0000.co2) are considered too.
    void SetAllowCoop(bool allow) { allowCoop_ = allow; }

    bool HasData() const { return data_.ok; }
    const ErSaveData& data() const { return data_; }
    // Slot the player is using, or -1 while unknown (several characters and no write seen yet).
    int ActiveSlot() const { return activeSlot_; }
    // Picks the slot of a character seen in memory (name, level and play time in ms). -1 if none matches.
    int MatchSlot(const std::wstring& name, int level, uint32_t playTimeMs) const;

    const std::wstring& path() const { return path_; }
    ULONGLONG lastReadTick() const { return lastRead_; }
    int readCount() const { return reads_; }
    std::string StatusText() const;

private:
    bool ReadFile(std::vector<uint8_t>& buf);
    void FindNewestFile();

    std::wstring path_;
    bool allowCoop_ = false;
    FILETIME lastWrite_{};          // of the parsed content
    uint64_t lastSize_ = 0;
    FILETIME seenWrite_{};          // last observed, waiting to settle
    uint64_t seenSize_ = 0;
    ULONGLONG seenSince_ = 0;
    ULONGLONG nextPoll_ = 0, nextScan_ = 0, retryAt_ = 0;
    ULONGLONG lastRead_ = 0;
    int reads_ = 0;
    int retries_ = 0;
    bool parsedOnce_ = false;
    ErSaveData data_;
    int activeSlot_ = -1;
    std::string lastError_;
};
