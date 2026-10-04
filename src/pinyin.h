#pragma once

#include "keys.h"

namespace td {
class Terminal;
using WordIndices = std::vector<uint16_t, secure_allocator<uint16_t>>;

// Chinese wordlist indices with a toneless reading equal to `pinyin` (v for u-umlaut).
// Words whose most common reading matches come first, each group in wordlist order.
WordIndices PinyinCandidates(std::string_view pinyin);
std::span<const uint8_t, 32> HanziGlyph(unsigned index);

// Reads `count` Chinese recovery words with pinyin on the framebuffer and returns
// them in Chinese, separated by spaces. Keys derive from the English translation.
SecretBytes ChineseMnemonic(Terminal& terminal, size_t count);
}
