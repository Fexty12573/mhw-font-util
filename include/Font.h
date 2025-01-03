#pragma once

#include <cstdint>

enum class FontType {
    Textured = 0,
    ImageBased,
    MergeTexture,
    Outlined,
    Merge
};

enum class TexSuffix {
    None = 0,
    ID,
    HQ,
    ID_HQ,
    IDL,
    IDL_HQ,
    AM_NOMIP
};

struct FontHeader {
    char magic[4];
    uint32_t version;
    uint32_t attr;
    TexSuffix suffix;
    FontType type;
    uint32_t size;
    uint32_t texture_count;
    uint32_t char_count;
    uint32_t unk0;
    uint32_t descent_count;
    float max_ascent;
    float max_descent;
    float unk1;
};
