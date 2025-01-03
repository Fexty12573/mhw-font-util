#include "Font.h"

#include <filesystem>
#include <fstream>
#include <print>
#include <span>
#include <vector>

#define check_magic(data, magic) (std::memcmp(data, magic, 4) == 0)


template<typename T>
T read(std::ifstream& file) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

void read(std::ifstream& file, std::span<uint8_t> buffer) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
}

void process_outline_font(std::ifstream& file, const FontHeader& header, const std::filesystem::path& file_path);
void generate_outline_font(std::ifstream& file, const std::filesystem::path& file_path);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("MHW Font Util 1.0 by Fexty");
        std::println("Converts between GFD and TTF/OTF font files.");
        std::println("  Usage: {} <font_file>", argv[0]);
        std::println("Note: Texture based fonts are not supported.");
        return 1;
    }

    const std::filesystem::path font_file = argv[1];
    std::ifstream file(font_file, std::ios::binary | std::ios::in);

    if (font_file.extension() == ".otf" || font_file.extension() == ".ttf") {
        generate_outline_font(file, font_file);
        return 0;
    }

    const auto header = read<FontHeader>(file);
    if (!check_magic(header.magic, "GFD\0")) {
        std::println("Invalid font file: {}", header.magic);
        return 1;
    }

    if (header.version != 135432) {
        std::println("Unsupported font version: {}", header.version);
        return 1;
    }

    if (header.type != FontType::Outlined) {
        std::println("Unsupported font type: {}", static_cast<int>(header.type));
        return 1;
    }

    process_outline_font(file, header, font_file);

    return 0;
}

void xor_crypt(std::span<uint8_t> buffer) {
    uint64_t n = 1;
    const uint32_t len3f = buffer.size() & 0x3F;
    for (uint32_t i = 0; i < len3f; ++i) {
        n = (n << 1) + 1;
    }

    n = (n & 0xae6e39b58a355f45) << (0x40U - len3f) | 0xae6e39b58a355f45U >> len3f;

    const auto xor_key = (uint8_t*)&n;
    const auto ptr = buffer.data();

    for (uint32_t j = 0; j < buffer.size(); ++j) {
        ptr[j] = ptr[j] ^ xor_key[j % 8];
    }
}

void process_outline_font(std::ifstream& file, const FontHeader& header, const std::filesystem::path& file_path) {
    if (header.attr & 1 || header.texture_count != 0) {
        std::println("Outline Font not suitable for processing.");
        return;
    }

    file.seekg(header.descent_count * sizeof(float), std::ios::cur); // Skip descent data

    const auto tex_path_size = read<uint32_t>(file);
    file.seekg(tex_path_size + 1, std::ios::cur); // Skip texture path

    if (!(header.attr >> 3 & 1) && header.texture_count != 0) {
        for (uint32_t i = 0; i < header.texture_count; ++i) {
            const auto tex_name_size = read<uint32_t>(file);
            file.seekg(tex_name_size + 1, std::ios::cur); // Skip texture name
        }
    }

    file.seekg((header.char_count + header.unk0) * 0x14LL, std::ios::cur); // Skip character data

    read<uint32_t>(file); // f0
    read<uint32_t>(file); // f4
    const auto buffer_size = read<uint32_t>(file);

    std::println("Starting font processing at offset: 0x{:X}", (uint64_t)file.tellg());
    std::println("Buffer size: {}", buffer_size);

    std::vector<uint8_t> buffer(buffer_size);
    read(file, buffer);

    xor_crypt(buffer);

    const auto extension = check_magic(buffer.data(), "OTTO") ? ".otf" : ".ttf";

    auto out_file = file_path;
    out_file.replace_filename(out_file.stem().string() + extension);
    std::ofstream out(out_file, std::ios::binary | std::ios::out);
    out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

    std::println("Font processed successfully. Output file: {}", out_file.string());
}

void generate_outline_font(std::ifstream& file, const std::filesystem::path& file_path) {
    const auto write = [](std::ofstream& out, const auto& value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    };
    const auto write_buf = [](std::ofstream& out, std::span<const uint8_t> buffer) {
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    };

    FontHeader header{};
    header.magic[0] = 'G';
    header.magic[1] = 'F';
    header.magic[2] = 'D';
    header.magic[3] = '\0';

    header.version = 135432;
    header.attr = 12;
    header.suffix = TexSuffix::ID_HQ;
    header.type = FontType::Outlined;
    header.size = 32;
    header.texture_count = 0;
    header.char_count = 0;
    header.unk0 = 0;
    header.descent_count = 0;
    header.max_ascent = 32.384f;
    header.max_descent = 32.7f;
    header.unk1 = 12.128f;

    std::vector<uint8_t> buffer(std::filesystem::file_size(file_path));
    read(file, buffer);
    xor_crypt(buffer);

    auto out_file = file_path;
    out_file.replace_filename(out_file.stem().string() + ".gfd");
    std::ofstream out(out_file, std::ios::binary | std::ios::out);

    write(out, header);
    write(out, 0); // Texture path size
    write(out, (uint8_t)0); // Texture path null terminator
    write(out, 1877690412); // f0
    write(out, 0); // f4
    write(out, (uint32_t)buffer.size());
    write_buf(out, buffer);

    std::println("Font generated successfully. Output file: {}", out_file.string());
}
