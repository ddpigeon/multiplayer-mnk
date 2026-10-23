#include "core/launch_settings.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace mnk::core {

std::string MacroConfig::label() const {
    return std::to_string(m) + "," + std::to_string(n) + "," + std::to_string(k) + ","
        + std::to_string(p);
}

namespace {

/// The parser inside this file handles exactly the closed two-level grammar.
/// One consumed settings line: "indent key: value" with the grammar rule in
/// the line (no inline comments, no quoted scalars, no anchors — they are not
/// part of the grammar).
struct SettingsLine {
    int lineno = 0;
    int depth = 0;      ///< leading two-space groups
    std::string key;    ///< trimmed text left of ':'
    std::string value;  ///< trimmed text right of ':'; empty for "key:" lines
};

[[noreturn]] void reject_line(const SettingsLine& line, const std::string& reason) {
    throw settings_error("line " + std::to_string(line.lineno) + ": " + reason);
}

void reject(int lineno, const std::string& reason) {
    throw settings_error("line " + std::to_string(lineno) + ": " + reason);
}

/// The exact macro key set the 'game' section may contain — the one
/// declaration both the structure phase (unknown/duplicate keys) and the
/// validation phase (required keys) read, so they can never drift apart.
const std::set<std::string>& macro_keys() {
    static const std::set<std::string> keys = {"m", "n", "k", "p"};
    return keys;
}

/// Trims spaces (and a stray CR from CRLF files) off both ends.
std::string trim(const std::string& text) {
    const auto not_space = [](unsigned char c) { return c != ' ' && c != '\r' && c != '\n'; };
    const auto first = std::find_if(text.begin(), text.end(), not_space);
    if (first == text.end()) {
        return {};
    }
    const auto last = std::find_if(text.rbegin(), text.rend(), not_space).base();
    return std::string(first, last);
}

/// Reads the whole file into one vector of raw lines; an unreadable file is a
/// load error, not a silent empty document.
std::vector<std::string> slurp_lines(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw settings_error("cannot open settings file '" + path + "'");
    }
    std::vector<std::string> lines;
    std::string line_text;
    while (std::getline(file, line_text)) {
        lines.push_back(line_text);
    }
    return lines;
}

/// Parses strictly: "key" / "key: value" after a run of spaces. A tab, an
/// indent not in whole two-space groups, or a line starting with '#' is
/// malformed — the closed grammar has no comments. Blank lines are skipped.
std::optional<SettingsLine> parse_line(int lineno, const std::string& raw) {
    const std::size_t content_start = raw.find_first_not_of(" \t\r\n");
    if (content_start == std::string::npos) {
        return std::nullopt; // blank line
    }
    if (raw[content_start] == '#') {
        reject(lineno, "malformed line '" + trim(raw)
                + "': comments are not part of the settings grammar");
    }

    std::size_t indent_end = 0;
    while (indent_end < raw.size() && (raw[indent_end] == ' ' || raw[indent_end] == '\t')) {
        if (raw[indent_end] == '\t') {
            reject(lineno, "indentation must be spaces, not tabs");
        }
        ++indent_end;
    }
    const int depth = static_cast<int>(indent_end) / 2;
    if (indent_end % 2 != 0) {
        reject(lineno, "indentation must be in whole two-space groups");
    }

    const std::size_t colon = raw.find(':');
    if (colon == std::string::npos) {
        reject(lineno, "expected 'key:' or 'key: value', found '" + trim(raw) + "'");
    }

    SettingsLine line;
    line.lineno = lineno;
    line.depth = depth;
    line.key = trim(raw.substr(indent_end, colon - indent_end));
    line.value = trim(raw.substr(colon + 1));
    if (line.key.empty()) {
        reject_line(line, "expected a key before ':'");
    }
    return line;
}

/// Parses an integer scalar: optional '-' then digits. The grammar keeps
/// integers integers — "10abc", "1.5", "+3" are type errors, not numbers.
int parse_int(const std::string& value, const std::string& what, int lineno) {
    int parsed = 0;
    const char* begin = value.c_str();
    const auto [ptr, ec] = std::from_chars(begin, begin + value.size(), parsed);
    if (ec == std::errc::result_out_of_range) {
        reject(lineno, what + " must be an integer within int range, found '" + value + "'");
    }
    if (ec != std::errc{} || ptr != begin + value.size() || value.empty()) {
        reject(lineno, what + " must be an integer, found '" + value + "'");
    }
    return parsed;
}

/// One parsed seat block's keys, with the line each key appeared on so
/// validation errors can name the offending line.
struct RawSeat {
    int number = 0;
    int lineno = 0;
    std::string type = "UNSET";
    std::string name;
    std::string strength; ///< empty = absent
    std::map<std::string, int> key_lines;
};

/// The two-phase strict read: file → structure → validation. The structure
/// phase rejects everything outside the closed grammar shape.
struct ParsedDocument {
    std::map<std::string, int> game;        ///< raw macro key → int value
    std::vector<RawSeat> seats;             ///< file arrival order
    std::set<std::string> top_level_keys;
};

ParsedDocument parse_structure(const std::vector<std::string>& raw_lines) {
    ParsedDocument doc;
    enum class Section { none, game, players };
    Section section = Section::none;
    RawSeat* current_seat = nullptr;

    for (int lineno = 1; lineno <= static_cast<int>(raw_lines.size()); ++lineno) {
        const auto line = parse_line(lineno, raw_lines[lineno - 1]);
        if (!line) {
            continue;
        }
        const bool takes_value = !line->value.empty();

        if (line->depth == 0) {
            if (line->key != "game" && line->key != "players") {
                reject_line(*line, "unknown key '" + line->key + "' (expected 'game' or 'players')");
            }
            if (takes_value) {
                reject_line(*line, "'" + line->key + "' must be a section taking an indented block, not '"
                        + line->key + ": " + line->value + "'");
            }
            if (!doc.top_level_keys.insert(line->key).second) {
                reject_line(*line, "duplicate key '" + line->key + "'");
            }
            section = line->key == "game" ? Section::game : Section::players;
            current_seat = nullptr;
            continue;
        }
        if (line->depth == 1) {
            if (section == Section::none) {
                reject_line(*line,
                    "indented key '" + line->key + "' has no parent section");
            }
            if (section == Section::game) {
                if (!takes_value) {
                    // a block under a game macro would be a third grammar level
                    reject_line(*line, "'game' macro '" + line->key
                            + "' must have an integer value (nesting is two levels deep at most)");
                }
                const std::set<std::string>& game_macro_keys = macro_keys();
                if (game_macro_keys.find(line->key) == game_macro_keys.end()) {
                    reject_line(*line, "unknown key '" + line->key
                            + "' in the 'game' section (expected m, n, k, p)");
                }
                if (!doc.game.insert({line->key, 0}).second) {
                    reject_line(*line, "duplicate key '" + line->key + "' in the 'game' section");
                }
                doc.game[line->key] = parse_int(line->value, "game macro '" + line->key + "'", lineno);
                continue;
            }
            // seat label
            if (takes_value) {
                reject_line(*line, "seat '" + line->key + "' must open an indented block");
            }
            const int seat_number = parse_int(line->key, "seat number", lineno);
            if (seat_number < 1) {
                reject_line(*line, "seat numbers start at 1, found '" + line->key + "'");
            }
            for (const RawSeat& seat : doc.seats) {
                if (seat.number == seat_number) {
                    reject_line(*line, "duplicate seat number " + std::to_string(seat_number));
                }
            }
            doc.seats.push_back(RawSeat{seat_number, lineno, "UNSET", "", "", {}});
            current_seat = &doc.seats.back();
            continue;
        }
        if (line->depth == 2) {
            if (section != Section::players || current_seat == nullptr) {
                reject_line(*line, "key '" + line->key + "' has no seat block above it");
            }
            static const std::set<std::string> seat_keys = {"type", "name", "strength"};
            if (seat_keys.find(line->key) == seat_keys.end()) {
                reject_line(*line, "unknown key '" + line->key
                        + "' in seat " + std::to_string(current_seat->number));
            }
            if (!current_seat->key_lines.insert({line->key, lineno}).second) {
                reject_line(*line, "duplicate key '" + line->key + "' in seat "
                        + std::to_string(current_seat->number));
            }
            if (!takes_value) {
                reject_line(*line, "seat key '" + line->key + "' must have a value");
            }
            if (line->key == "type") {
                current_seat->type = line->value;
            } else if (line->key == "name") {
                current_seat->name = line->value;
            } else {
                current_seat->strength = line->value;
            }
            continue;
        }
        reject_line(*line, "nesting is two levels deep at most; unexpected key '"
                + line->key + "'");
    }
    return doc;
}

/// Validation over the closed structure: present sections, required keys,
/// legal values, seat coverage, mode legality, macro bounds.
LaunchSettings validate(const ParsedDocument& doc, LaunchMode mode) {
    if (doc.top_level_keys.find("game") == doc.top_level_keys.end()) {
        reject(0, "the 'game' section is missing");
    }
    for (const std::string& key : macro_keys()) {
        if (doc.game.find(key) == doc.game.end()) {
            reject(0, "'" + std::string(key) + "' is missing: the 'game' section needs m, n, k, p");
        }
    }

    if (doc.top_level_keys.find("players") == doc.top_level_keys.end()
        && doc.seats.empty()) {
        reject(0, "the 'players' section is missing");
    }

    LaunchSettings settings;
    settings.macro_config.m = doc.game.at("m");
    settings.macro_config.n = doc.game.at("n");
    settings.macro_config.k = doc.game.at("k");
    settings.macro_config.p = doc.game.at("p");

    const MacroConfig& game = settings.macro_config;
    if (game.m <= 0) {
        reject(0, "m must be positive, found " + std::to_string(game.m));
    }
    if (game.n <= 0) {
        reject(0, "n must be positive, found " + std::to_string(game.n));
    }
    if (game.k < 2) {
        reject(0, "k must be at least 2, found " + std::to_string(game.k));
    }
    if (game.p < 2) {
        reject(0, "p must be at least 2, found " + std::to_string(game.p));
    }

    // Seat coverage: exactly 1..p, no duplicates (structure phase already
    // rejects duplicates), no gaps, no number outside 1..p.
    std::set<int> covered;
    for (const RawSeat& seat : doc.seats) {
        if (seat.number > game.p) {
            reject(seat.lineno, "seat " + std::to_string(seat.number)
                    + " exceeds p = " + std::to_string(game.p));
        }
        if (!covered.insert(seat.number).second) {
            // duplicate seat numbers already failed in the structure phase
            continue;
        }
    }
    std::vector<std::string> missing;
    for (int number = 1; number <= game.p; ++number) {
        if (covered.count(number) == 0) {
            missing.push_back(std::to_string(number));
        }
    }
    if (!missing.empty()) {
        std::string list = missing.front();
        for (std::size_t i = 1; i < missing.size(); ++i) {
            list += ", " + missing[i];
        }
        reject(0, "seat number(s) " + list
                + " missing: seats must cover exactly 1..p with no gaps");
    }

    const bool is_training = mode == LaunchMode::training;
    for (const RawSeat& raw : doc.seats) {
        Seat seat;
        seat.number = raw.number;
        const int type_line = raw.key_lines.count("type") != 0u ? raw.key_lines.at("type") : raw.lineno;
        if (raw.key_lines.count("type") == 0u) {
            reject(raw.lineno, "'type' is required in seat " + std::to_string(raw.number));
        }
        if (raw.type == "human") {
            seat.type = SeatType::human;
        } else if (raw.type == "bot") {
            seat.type = SeatType::bot;
        } else {
            reject(type_line, "unknown seat type '" + raw.type
                    + "' for seat " + std::to_string(raw.number)
                    + " (expected 'human' or 'bot')");
        }
        if (raw.key_lines.count("strength") != 0u) {
            if (seat.type == SeatType::human) {
                reject(raw.key_lines.at("strength"),
                    "unknown key 'strength' for human seat "
                        + std::to_string(raw.number) + " (only bot seats take strength)");
            }
            seat.strength = parse_int(raw.strength,
                "strength of seat " + std::to_string(raw.number),
                raw.key_lines.at("strength"));
            if (seat.strength < 0) {
                reject(raw.key_lines.at("strength"),
                    "strength of seat " + std::to_string(raw.number)
                        + " must be non-negative, found " + std::to_string(seat.strength));
            }
        } else if (seat.type == SeatType::bot) {
            reject(raw.lineno, "seat " + std::to_string(raw.number)
                    + " is a bot: 'strength' is required");
        }
        if (is_training && seat.type == SeatType::human) {
            reject(type_line, "seat " + std::to_string(raw.number)
                    + " is human: training mode requires every seat to be a bot");
        }
        seat.name = raw.name;
        settings.seats.push_back(seat);
    }
    return settings;
}

} // namespace

LaunchSettings load(const std::string& path, LaunchMode mode) {
    const ParsedDocument doc = parse_structure(slurp_lines(path));
    return validate(doc, mode);
}

} // namespace mnk::core
