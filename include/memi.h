#ifndef MEMI_H
#define MEMI_H

enum memi_hash {
    // Web Core
    HASH_HTML   = 0x1A56DC1CU, // wyhash32("html")
    HASH_HTM    = 0x9D43A094U, // wyhash32("htm")
    HASH_CSS    = 0x52474291U, // wyhash32("css")
    HASH_JS     = 0x5CF454EBU, // wyhash32("js")
    HASH_MJS    = 0xE501678CU, // wyhash32("mjs")
    HASH_JSON   = 0xF5E111DEU, // wyhash32("json")
    HASH_WASM   = 0x781F5301U, // wyhash32("wasm")
    HASH_TXT    = 0x2DDE9D56U, // wyhash32("txt")
    HASH_XML    = 0xB8C2B66DU, // wyhash32("xml")

    // Images
    HASH_SVG    = 0xA5CBEC2AU, // wyhash32("svg")
    HASH_PNG    = 0xDB25A487U, // wyhash32("png")
    HASH_JPG    = 0x998C21A2U, // wyhash32("jpg")
    HASH_JPEG   = 0xAE85D9F1U, // wyhash32("jpeg")
    HASH_GIF    = 0x72B8D054U, // wyhash32("gif")
    HASH_WEBP   = 0xA82A5600U, // wyhash32("webp")
    HASH_AVIF   = 0x7CA60608U, // wyhash32("avif")
    HASH_ICO    = 0x4C7469F2U, // wyhash32("ico")

    // Fonts
    HASH_WOFF2  = 0x57820663U, // wyhash32("woff2")
    HASH_WOFF   = 0xCC968037U, // wyhash32("woff")
    HASH_TTF    = 0x4A789E76U, // wyhash32("ttf")
    HASH_OTF    = 0x9BE83846U, // wyhash32("otf")

    // Media & Docs
    HASH_MP4    = 0x2890E211U, // wyhash32("mp4")
    HASH_WEBM   = 0x4A14237CU, // wyhash32("webm")
    HASH_MP3    = 0x7D9BAFFCU, // wyhash32("mp3")
    HASH_PDF    = 0x7524A72EU  // wyhash32("pdf")
};

const char *get_memi_type(const char *path);

#endif