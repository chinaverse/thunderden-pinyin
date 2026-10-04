#include "pinyin.h"
#include "chinese.h"
#include "hardware.h"
#include "terminal.h"

#include <algorithm>
#include <optional>

namespace td {
namespace {
constexpr size_t MAX_PINYIN = 6, PAGE = 9;
constexpr uint8_t BRIGHT = 255, DIM = 168;

std::string_view View(const SecretBytes& bytes)
{
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void Wipe(SecretBytes& bytes)
{
    memory_cleanse(bytes.data(), bytes.size());
    bytes.clear();
}
}

WordIndices PinyinCandidates(std::string_view pinyin)
{
    WordIndices primary, other;
    if (pinyin.empty()) return primary;
    for (unsigned index = 0; index < PINYIN.size(); ++index) {
        const auto readings = PINYIN[index];
        for (size_t start = 0, reading = 0; start <= readings.size(); ++reading) {
            const size_t end = std::min(readings.find(' ', start), readings.size());
            if (readings.substr(start, end - start) == pinyin) {
                (reading == 0 ? primary : other).push_back(index);
                break;
            }
            start = end + 1;
        }
    }
    primary.insert(primary.end(), other.begin(), other.end());
    return primary;
}

std::span<const uint8_t, 32> HanziGlyph(unsigned index)
{
    Require(index < CHINESE.size(), "Invalid Chinese word index");
    return std::span<const uint8_t, 32>(HANZI_GLYPHS.data() + index * 32, 32);
}

SecretBytes ChineseMnemonic(Terminal& terminal, size_t count)
{
    Require(count >= 12 && count <= 24 && count % 3 == 0, "Invalid recovery word count");
    Display display(terminal);
    const unsigned columns = display.Columns(), rows = display.Rows();
    Require(columns >= 40, "The screen is too small for Chinese input.");
    const unsigned width = std::min(80U, columns - 2), left = (columns - width) / 2;
    // Each candidate takes "9 " plus a two-cell character and two spaces.
    const unsigned per_row = width / 6, candidate_rows = (PAGE + per_row - 1) / per_row;
    const auto intro = Wrap({"Type the pinyin of each word without tones, then press its number to choose "
        "the character. Use v for u-umlaut (lv, nv).", "",
        "Characters are visible while you choose. Make sure no one else can see this screen."}, width);
    const auto footer = Wrap({"1-9: Choose   Left/Right: More   Enter: Next   Backspace: Edit   "
        "Up: Previous word   Tab: Show/hide chosen word   Esc: Cancel"}, width);
    const unsigned error_row = 5 + intro.size() + 1, input_row = error_row + 3;
    const unsigned candidate_row = input_row + 2, page_row = candidate_row + candidate_rows;
    const unsigned footer_row = rows - footer.size() - 1;
    Require(footer_row > page_row + 1, "The screen is too small for Chinese input.");

    display.Text(left, 1, "Thunder Den", DIM);
    display.Text(left, 2, "Enter your Chinese recovery words", BRIGHT);
    display.Text(left, 3, std::string(width, '-'), DIM);
    for (size_t i = 0; i < intro.size(); ++i) display.Text(left, 5 + i, intro[i]);
    display.Text(left, footer_row - 1, std::string(width, '-'), DIM);
    for (size_t i = 0; i < footer.size(); ++i) display.Text(left, footer_row + i, footer[i], DIM);

    std::vector<SecretBytes> words(count);
    SecretBytes pinyin;
    pinyin.reserve(MAX_PINYIN);
    size_t index = 0, page = 0;
    bool visible = false;
    std::string error;
    WordIndices candidates;
    const auto update = [&]() {
        candidates = PinyinCandidates(View(pinyin));
        for (unsigned row = error_row; row <= page_row; ++row) display.ClearRow(row);
        const auto message = Wrap({error}, width);
        for (size_t i = 0; i < std::min<size_t>(message.size(), 2); ++i) display.Text(left, error_row + i, message[i], BRIGHT);
        unsigned column = display.Text(left, input_row, "Word " + std::to_string(index + 1) + " of " + std::to_string(count) + ": ");
        column = display.Text(column, input_row, View(pinyin), BRIGHT);
        column = display.Text(column, input_row, "_   ", DIM);
        if (!words[index].empty()) {
            column = display.Text(column, input_row, "Chosen: ", DIM);
            if (visible) display.Hanzi(column, input_row, HanziGlyph(ChineseWordIndex(View(words[index]))));
            else display.Text(column, input_row, "*");
        }
        if (!pinyin.empty() && candidates.empty()) display.Text(left, candidate_row, "No recovery word has this pinyin.", DIM);
        for (size_t i = 0; i < PAGE && page * PAGE + i < candidates.size(); ++i) {
            column = left + (i % per_row) * 6;
            column = display.Text(column, candidate_row + i / per_row, std::string(1, char('1' + i)) + " ");
            display.Hanzi(column, candidate_row + i / per_row, HanziGlyph(candidates[page * PAGE + i]), BRIGHT);
        }
        if (candidates.size() > PAGE) {
            display.Text(left, page_row, "Page " + std::to_string(page + 1) + "/"
                + std::to_string((candidates.size() + PAGE - 1) / PAGE), DIM);
        }
    };
    // Moves past the current word; returns the phrase once all words form a valid one.
    const auto next = [&]() -> std::optional<SecretBytes> {
        Wipe(pinyin);
        page = 0;
        if (++index < count) return {};
        SecretBytes phrase;
        phrase.reserve(count * 4);
        for (const auto& word : words) {
            if (!phrase.empty()) phrase.push_back(' ');
            phrase.insert(phrase.end(), word.begin(), word.end());
        }
        try {
            ValidateMnemonic(EnglishMnemonic(phrase));
            return phrase;
        } catch (const std::invalid_argument&) {
            error = "These words do not make a valid recovery phrase. Check your backup and enter the "
                + std::to_string(count) + " words again.";
            for (auto& word : words) Wipe(word);
            index = 0;
            return {};
        }
    };

    terminal.Flush();
    while (true) {
        update();
        int key = terminal.Key();
        if (!key) continue;
        error.clear();
        if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
        if (key == 27 || key == 3) throw Cancelled{};
        if (key >= 'a' && key <= 'z') {
            if (pinyin.size() < MAX_PINYIN) pinyin.push_back(key);
            else error = "Pinyin has at most 6 letters.";
            page = 0;
        } else if (key == 127 || key == 8) {
            if (!pinyin.empty()) { pinyin.back() = 0; pinyin.pop_back(); }
            page = 0;
        } else if (key == KEY_RIGHT) {
            if ((page + 1) * PAGE < candidates.size()) ++page;
        } else if (key == KEY_LEFT) {
            if (page) --page;
        } else if (key == 9) {
            visible = !visible;
        } else if (key == KEY_UP) {
            if (index) { Wipe(pinyin); page = 0; --index; }
        } else if ((key >= '1' && key <= '9') || key == '\r' || key == '\n') {
            size_t pick = 0;
            if (key >= '1' && key <= '9') pick = page * PAGE + (key - '1');
            else {
                if (pinyin.empty() && !words[index].empty()) {
                    if (auto phrase = next()) return std::move(*phrase);
                    continue;
                }
                if (candidates.size() != 1) {
                    if (!pinyin.empty()) error = candidates.empty() ? "No recovery word has this pinyin."
                        : "Press a number to choose a character.";
                    continue;
                }
            }
            if (pick >= candidates.size()) continue;
            const auto word = CHINESE[candidates[pick]];
            Wipe(words[index]);
            words[index].assign(word.begin(), word.end());
            if (auto phrase = next()) return std::move(*phrase);
        }
    }
}
}
