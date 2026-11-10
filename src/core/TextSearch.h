#pragma once

#include <string>
#include <string_view>

namespace libster {

// Normalisiert Text für die Suche:
//  - ASCII-Großbuchstaben werden kleingeschrieben
//  - Latin-1-Akzente und deutsche Umlaute werden auf den Grundbuchstaben
//    gefaltet (Ä/ä -> a, Ö/ö -> o, Ü/ü -> u, ß -> ss)
// Ungültige UTF-8-Bytes werden unverändert übernommen.
std::string foldForSearch(std::string_view text);

// Teilstring-Suche. haystack wird intern gefaltet, needle muss bereits mit
// foldForSearch() normalisiert worden sein (spart eine Faltung pro Datensatz).
bool containsFolded(std::string_view haystack, std::string_view foldedNeedle);

} // namespace libster
