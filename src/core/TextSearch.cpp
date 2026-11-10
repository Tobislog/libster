#include "core/TextSearch.h"

#include <cstdint>

namespace libster {
namespace {

constexpr std::uint32_t kInvalid = 0xFFFFFFFFu;

void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// Grundbuchstabe für Latin-1-Zeichen (U+00C0..U+00FF), 0 = keiner.
char latin1BaseLetter(std::uint32_t cp) {
    if (cp >= 0x00C0 && cp <= 0x00C5) return 'a'; // À Á Â Ã Ä Å
    if (cp == 0x00C7) return 'c';
    if (cp >= 0x00C8 && cp <= 0x00CB) return 'e';
    if (cp >= 0x00CC && cp <= 0x00CF) return 'i';
    if (cp == 0x00D0 || cp == 0x00F0) return 'd';
    if (cp == 0x00D1 || cp == 0x00F1) return 'n';
    if (cp >= 0x00D2 && cp <= 0x00D6) return 'o'; // Ò Ó Ô Õ Ö
    if (cp == 0x00D8 || cp == 0x00F8) return 'o';
    if (cp >= 0x00D9 && cp <= 0x00DC) return 'u'; // Ù Ú Û Ü
    if (cp == 0x00DD || cp == 0x00FD || cp == 0x00FF) return 'y';
    if (cp >= 0x00E0 && cp <= 0x00E5) return 'a'; // à á â ã ä å
    if (cp >= 0x00E8 && cp <= 0x00EB) return 'e';
    if (cp >= 0x00EC && cp <= 0x00EF) return 'i';
    if (cp >= 0x00F2 && cp <= 0x00F6) return 'o'; // ò ó ô õ ö
    if (cp >= 0x00F9 && cp <= 0x00FC) return 'u'; // ù ú û ü
    return 0;
}

void appendFolded(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>((cp >= 'A' && cp <= 'Z') ? cp + 32 : cp));
        return;
    }

    if (cp == 0x00DF || cp == 0x1E9E) { out += "ss"; return; }  // ß / ẞ
    if (cp == 0x00C6 || cp == 0x00E6) { out += "ae"; return; }  // Æ / æ
    if (cp == 0x00DE || cp == 0x00FE) { out += "th"; return; }  // Þ / þ

    if (const char base = latin1BaseLetter(cp); base != 0) {
        out.push_back(base);
        return;
    }

    appendUtf8(out, cp);
}

// Liefert den Codepoint ab text[pos] und rückt pos hinter die Sequenz.
// Bei ungültigem UTF-8 wird kInvalid zurückgegeben und pos bleibt stehen.
std::uint32_t decodeUtf8(std::string_view text, std::size_t& pos) {
    const auto first = static_cast<unsigned char>(text[pos]);
    if (first < 0x80) { ++pos; return first; }

    int extra = 0;
    std::uint32_t cp = 0;

    if ((first & 0xE0) == 0xC0)      { extra = 1; cp = first & 0x1F; }
    else if ((first & 0xF0) == 0xE0) { extra = 2; cp = first & 0x0F; }
    else if ((first & 0xF8) == 0xF0) { extra = 3; cp = first & 0x07; }
    else return kInvalid;

    if (pos + static_cast<std::size_t>(extra) >= text.size()) return kInvalid;

    for (int i = 1; i <= extra; ++i) {
        const auto cont = static_cast<unsigned char>(text[pos + static_cast<std::size_t>(i)]);
        if ((cont & 0xC0) != 0x80) return kInvalid;
        cp = (cp << 6) | (cont & 0x3F);
    }

    pos += static_cast<std::size_t>(extra) + 1;
    return cp;
}

} // namespace

std::string foldForSearch(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t next = pos;
        const std::uint32_t cp = decodeUtf8(text, next);

        if (cp == kInvalid) {
            out.push_back(text[pos]);
            ++pos;
            continue;
        }

        appendFolded(out, cp);
        pos = next;
    }

    return out;
}

bool containsFolded(std::string_view haystack, std::string_view foldedNeedle) {
    const std::string folded = foldForSearch(haystack);
    return std::string_view(folded).find(foldedNeedle) != std::string_view::npos;
}

} // namespace libster
