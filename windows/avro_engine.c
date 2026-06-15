#define _CRT_SECURE_NO_WARNINGS
#include "avro_core.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// ------------------------------------------------------------------
// Data structures
// ------------------------------------------------------------------

typedef enum MatchType { PREFIX, SUFFIX } MatchType;

typedef struct Match {
    MatchType type;
    const wchar_t* scope;    // may start with '!' for negative
    const wchar_t* value;    // used for "exact"
    bool negative;           // explicit negative flag
} Match;

typedef struct Rule {
    Match* matches;
    int match_count;
    const wchar_t* replace;
} Rule;

typedef struct Pattern {
    const wchar_t* find;
    const wchar_t* replace;
    Rule* rules;
    int rule_count;
} Pattern;

// ------------------------------------------------------------------
// Helper sets
// ------------------------------------------------------------------
static const wchar_t* VOWELS = L"aeiou";
static const wchar_t* CONSONANTS = L"bcdfghjklmnpqrstvwxyz";
static const wchar_t* CASE_SENSITIVE = L"oiudgjnrstyz";

int avro_is_vowel(wchar_t c) {
    for (const wchar_t* p = VOWELS; *p; p++) if (*p == c) return 1;
    return 0;
}

int avro_is_consonant(wchar_t c) {
    for (const wchar_t* p = CONSONANTS; *p; p++) if (*p == c) return 1;
    return 0;
}

int avro_is_punctuation(wchar_t c) {
    return !(avro_is_vowel(c) || avro_is_consonant(c));
}

static bool is_case_sensitive(wchar_t c) {
    for (const wchar_t* p = CASE_SENSITIVE; *p; p++) if (*p == c) return true;
    return false;
}

// Fix string case: if character is case-sensitive, keep original, else lowercase
static wchar_t* fix_string(const wchar_t* input) {
    size_t len = wcslen(input);
    wchar_t* out = malloc((len + 1) * sizeof(wchar_t));
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) {
        wchar_t ch = input[i];
        if (is_case_sensitive(ch))
            out[i] = ch;
        else
            out[i] = towlower(ch);
    }
    out[len] = L'\0';
    return out;
}

// Exact match with negative flag
static bool is_exact(const wchar_t* needle, const wchar_t* haystack, int start, int end, bool not_flag) {
    size_t needle_len = wcslen(needle);
    if (end - start != (int)needle_len) return false ^ not_flag;
    for (int i = 0; i < (int)needle_len; i++) {
        if (haystack[start + i] != needle[i]) return false ^ not_flag;
    }
    return true ^ not_flag;
}

// ------------------------------------------------------------------
// Pattern data – converted from your JSON
// ------------------------------------------------------------------

// Helper macro to define simple patterns
#define SIMPLE_PATTERN(find_str, replace_str) { find_str, replace_str, NULL, 0 }

// ----- Simple patterns (no rules) in the exact order from JSON -----
static Pattern simple_patterns[] = {
    SIMPLE_PATTERN(L"bhl", L"ভ্ল"),
    SIMPLE_PATTERN(L"psh", L"পশ"),
    SIMPLE_PATTERN(L"bdh", L"ব্ধ"),
    SIMPLE_PATTERN(L"bj", L"ব্জ"),
    SIMPLE_PATTERN(L"bd", L"ব্দ"),
    SIMPLE_PATTERN(L"bb", L"ব্ব"),
    SIMPLE_PATTERN(L"bl", L"ব্ল"),
    SIMPLE_PATTERN(L"bh", L"ভ"),
    SIMPLE_PATTERN(L"vl", L"ভ্ল"),
    SIMPLE_PATTERN(L"b", L"ব"),
    SIMPLE_PATTERN(L"v", L"ভ"),
    SIMPLE_PATTERN(L"cNG", L"চ্ঞ"),
    SIMPLE_PATTERN(L"cch", L"চ্ছ"),
    SIMPLE_PATTERN(L"cc", L"চ্চ"),
    SIMPLE_PATTERN(L"ch", L"ছ"),
    SIMPLE_PATTERN(L"c", L"চ"),
    SIMPLE_PATTERN(L"dhn", L"ধ্ন"),
    SIMPLE_PATTERN(L"dhm", L"ধ্ম"),
    SIMPLE_PATTERN(L"dgh", L"দ্ঘ"),
    SIMPLE_PATTERN(L"ddh", L"দ্ধ"),
    SIMPLE_PATTERN(L"dbh", L"দ্ভ"),
    SIMPLE_PATTERN(L"dv", L"দ্ভ"),
    SIMPLE_PATTERN(L"dm", L"দ্ম"),
    SIMPLE_PATTERN(L"DD", L"ড্ড"),
    SIMPLE_PATTERN(L"Dh", L"ঢ"),
    SIMPLE_PATTERN(L"dh", L"ধ"),
    SIMPLE_PATTERN(L"dg", L"দ্গ"),
    SIMPLE_PATTERN(L"dd", L"দ্দ"),
    SIMPLE_PATTERN(L"D", L"ড"),
    SIMPLE_PATTERN(L"d", L"দ"),
    SIMPLE_PATTERN(L"...", L"..."),
    SIMPLE_PATTERN(L".`", L"."),
    SIMPLE_PATTERN(L"..", L"।।"),
    SIMPLE_PATTERN(L".", L"।"),
    SIMPLE_PATTERN(L"ghn", L"ঘ্ন"),
    SIMPLE_PATTERN(L"Ghn", L"ঘ্ন"),
    SIMPLE_PATTERN(L"gdh", L"গ্ধ"),
    SIMPLE_PATTERN(L"Gdh", L"গ্ধ"),
    SIMPLE_PATTERN(L"gN", L"গ্ণ"),
    SIMPLE_PATTERN(L"GN", L"গ্ণ"),
    SIMPLE_PATTERN(L"gn", L"গ্ন"),
    SIMPLE_PATTERN(L"Gn", L"গ্ন"),
    SIMPLE_PATTERN(L"gm", L"গ্ম"),
    SIMPLE_PATTERN(L"Gm", L"গ্ম"),
    SIMPLE_PATTERN(L"gl", L"গ্ল"),
    SIMPLE_PATTERN(L"Gl", L"গ্ল"),
    SIMPLE_PATTERN(L"gg", L"জ্ঞ"),
    SIMPLE_PATTERN(L"GG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"Gg", L"জ্ঞ"),
    SIMPLE_PATTERN(L"gG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"gh", L"ঘ"),
    SIMPLE_PATTERN(L"Gh", L"ঘ"),
    SIMPLE_PATTERN(L"g", L"গ"),
    SIMPLE_PATTERN(L"G", L"গ"),
    SIMPLE_PATTERN(L"hN", L"হ্ণ"),
    SIMPLE_PATTERN(L"hn", L"হ্ন"),
    SIMPLE_PATTERN(L"hm", L"হ্ম"),
    SIMPLE_PATTERN(L"hl", L"হ্ল"),
    SIMPLE_PATTERN(L"h", L"হ"),
    SIMPLE_PATTERN(L"jjh", L"জ্ঝ"),
    SIMPLE_PATTERN(L"jNG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"jh", L"ঝ"),
    SIMPLE_PATTERN(L"jj", L"জ্জ"),
    SIMPLE_PATTERN(L"j", L"জ"),
    SIMPLE_PATTERN(L"J", L"জ"),
    SIMPLE_PATTERN(L"kkhN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kShN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kkhm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kShm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kxN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kxm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kkh", L"ক্ষ"),
    SIMPLE_PATTERN(L"kSh", L"ক্ষ"),
    SIMPLE_PATTERN(L"ksh", L"কশ"),
    SIMPLE_PATTERN(L"kx", L"ক্ষ"),
    SIMPLE_PATTERN(L"kk", L"ক্ক"),
    SIMPLE_PATTERN(L"kT", L"ক্ট"),
    SIMPLE_PATTERN(L"kt", L"ক্ত"),
    SIMPLE_PATTERN(L"kl", L"ক্ল"),
    SIMPLE_PATTERN(L"ks", L"ক্স"),
    SIMPLE_PATTERN(L"kh", L"খ"),
    SIMPLE_PATTERN(L"k", L"ক"),
    SIMPLE_PATTERN(L"lbh", L"ল্ভ"),
    SIMPLE_PATTERN(L"ldh", L"ল্ধ"),
    SIMPLE_PATTERN(L"lkh", L"লখ"),
    SIMPLE_PATTERN(L"lgh", L"লঘ"),
    SIMPLE_PATTERN(L"lph", L"লফ"),
    SIMPLE_PATTERN(L"lk", L"ল্ক"),
    SIMPLE_PATTERN(L"lg", L"ল্গ"),
    SIMPLE_PATTERN(L"lT", L"ল্ট"),
    SIMPLE_PATTERN(L"lD", L"ল্ড"),
    SIMPLE_PATTERN(L"lp", L"ল্প"),
    SIMPLE_PATTERN(L"lv", L"ল্ভ"),
    SIMPLE_PATTERN(L"lm", L"ল্ম"),
    SIMPLE_PATTERN(L"ll", L"ল্ল"),
    SIMPLE_PATTERN(L"lb", L"ল্ব"),
    SIMPLE_PATTERN(L"l", L"ল"),
    SIMPLE_PATTERN(L"mth", L"ম্থ"),
    SIMPLE_PATTERN(L"mph", L"ম্ফ"),
    SIMPLE_PATTERN(L"mbh", L"ম্ভ"),
    SIMPLE_PATTERN(L"mpl", L"মপ্ল"),
    SIMPLE_PATTERN(L"mn", L"ম্ন"),
    SIMPLE_PATTERN(L"mp", L"ম্প"),
    SIMPLE_PATTERN(L"mv", L"ম্ভ"),
    SIMPLE_PATTERN(L"mm", L"ম্ম"),
    SIMPLE_PATTERN(L"ml", L"ম্ল"),
    SIMPLE_PATTERN(L"mb", L"ম্ব"),
    SIMPLE_PATTERN(L"mf", L"ম্ফ"),
    SIMPLE_PATTERN(L"m", L"ম"),
    SIMPLE_PATTERN(L"0", L"০"),
    SIMPLE_PATTERN(L"1", L"১"),
    SIMPLE_PATTERN(L"2", L"২"),
    SIMPLE_PATTERN(L"3", L"৩"),
    SIMPLE_PATTERN(L"4", L"৪"),
    SIMPLE_PATTERN(L"5", L"৫"),
    SIMPLE_PATTERN(L"6", L"৬"),
    SIMPLE_PATTERN(L"7", L"৭"),
    SIMPLE_PATTERN(L"8", L"৮"),
    SIMPLE_PATTERN(L"9", L"৯"),
    SIMPLE_PATTERN(L"NgkSh", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"Ngkkh", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"NGch", L"ঞ্ছ"),
    SIMPLE_PATTERN(L"Nggh", L"ঙ্ঘ"),
    SIMPLE_PATTERN(L"Ngkh", L"ঙ্খ"),
    SIMPLE_PATTERN(L"NGjh", L"ঞ্ঝ"),
    SIMPLE_PATTERN(L"ngOU", L"ঙ্গৌ"),
    SIMPLE_PATTERN(L"ngOI", L"ঙ্গৈ"),
    SIMPLE_PATTERN(L"Ngkx", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"NGc", L"ঞ্চ"),
    SIMPLE_PATTERN(L"nch", L"ঞ্ছ"),
    SIMPLE_PATTERN(L"njh", L"ঞ্ঝ"),
    SIMPLE_PATTERN(L"ngh", L"ঙ্ঘ"),
    SIMPLE_PATTERN(L"Ngk", L"ঙ্ক"),
    SIMPLE_PATTERN(L"Ngx", L"ঙ্ষ"),
    SIMPLE_PATTERN(L"Ngg", L"ঙ্গ"),
    SIMPLE_PATTERN(L"Ngm", L"ঙ্ম"),
    SIMPLE_PATTERN(L"NGj", L"ঞ্জ"),
    SIMPLE_PATTERN(L"ndh", L"ন্ধ"),
    SIMPLE_PATTERN(L"nTh", L"ন্ঠ"),
    SIMPLE_PATTERN(L"NTh", L"ণ্ঠ"),
    SIMPLE_PATTERN(L"nth", L"ন্থ"),
    SIMPLE_PATTERN(L"nkh", L"ঙ্খ"),
    SIMPLE_PATTERN(L"ngo", L"ঙ্গ"),
    SIMPLE_PATTERN(L"nga", L"ঙ্গা"),
    SIMPLE_PATTERN(L"ngi", L"ঙ্গি"),
    SIMPLE_PATTERN(L"ngI", L"ঙ্গী"),
    SIMPLE_PATTERN(L"ngu", L"ঙ্গু"),
    SIMPLE_PATTERN(L"ngU", L"ঙ্গূ"),
    SIMPLE_PATTERN(L"nge", L"ঙ্গে"),
    SIMPLE_PATTERN(L"ngO", L"ঙ্গো"),
    SIMPLE_PATTERN(L"NDh", L"ণ্ঢ"),
    SIMPLE_PATTERN(L"nsh", L"নশ"),
    SIMPLE_PATTERN(L"Ngr", L"ঙর"),
    SIMPLE_PATTERN(L"NGr", L"ঞর"),
    SIMPLE_PATTERN(L"ngr", L"ংর"),
    SIMPLE_PATTERN(L"nj", L"ঞ্জ"),
    SIMPLE_PATTERN(L"Ng", L"ঙ"),
    SIMPLE_PATTERN(L"NG", L"ঞ"),
    SIMPLE_PATTERN(L"nk", L"ঙ্ক"),
    SIMPLE_PATTERN(L"ng", L"ং"),
    SIMPLE_PATTERN(L"nn", L"ন্ন"),
    SIMPLE_PATTERN(L"NN", L"ণ্ণ"),
    SIMPLE_PATTERN(L"Nn", L"ণ্ন"),
    SIMPLE_PATTERN(L"nm", L"ন্ম"),
    SIMPLE_PATTERN(L"Nm", L"ণ্ম"),
    SIMPLE_PATTERN(L"nd", L"ন্দ"),
    SIMPLE_PATTERN(L"nT", L"ন্ট"),
    SIMPLE_PATTERN(L"NT", L"ণ্ট"),
    SIMPLE_PATTERN(L"nD", L"ন্ড"),
    SIMPLE_PATTERN(L"ND", L"ণ্ড"),
    SIMPLE_PATTERN(L"nt", L"ন্ত"),
    SIMPLE_PATTERN(L"ns", L"ন্স"),
    SIMPLE_PATTERN(L"nc", L"ঞ্চ"),
    SIMPLE_PATTERN(L"n", L"ন"),
    SIMPLE_PATTERN(L"N", L"ণ"),
    SIMPLE_PATTERN(L"OI`", L"ৈ"),
    SIMPLE_PATTERN(L"OU`", L"ৌ"),
    SIMPLE_PATTERN(L"O`", L"ো"),
    SIMPLE_PATTERN(L"phl", L"ফ্ল"),
    SIMPLE_PATTERN(L"pT", L"প্ট"),
    SIMPLE_PATTERN(L"pt", L"প্ত"),
    SIMPLE_PATTERN(L"pn", L"প্ন"),
    SIMPLE_PATTERN(L"pp", L"প্প"),
    SIMPLE_PATTERN(L"pl", L"প্ল"),
    SIMPLE_PATTERN(L"ps", L"প্স"),
    SIMPLE_PATTERN(L"ph", L"ফ"),
    SIMPLE_PATTERN(L"fl", L"ফ্ল"),
    SIMPLE_PATTERN(L"f", L"ফ"),
    SIMPLE_PATTERN(L"p", L"প"),
    SIMPLE_PATTERN(L"rri`", L"ৃ"),
    SIMPLE_PATTERN(L"rrZ", L"রর‍্য"),
    SIMPLE_PATTERN(L"rry", L"রর‍্য"),
    SIMPLE_PATTERN(L"Rg", L"ড়্গ"),
    SIMPLE_PATTERN(L"Rh", L"ঢ়"),
    SIMPLE_PATTERN(L"R", L"ড়"),
    SIMPLE_PATTERN(L"shch", L"শ্ছ"),
    SIMPLE_PATTERN(L"ShTh", L"ষ্ঠ"),
    SIMPLE_PATTERN(L"Shph", L"ষ্ফ"),
    SIMPLE_PATTERN(L"Sch", L"শ্ছ"),
    SIMPLE_PATTERN(L"skl", L"স্ক্ল"),
    SIMPLE_PATTERN(L"skh", L"স্খ"),
    SIMPLE_PATTERN(L"sth", L"স্থ"),
    SIMPLE_PATTERN(L"sph", L"স্ফ"),
    SIMPLE_PATTERN(L"shc", L"শ্চ"),
    SIMPLE_PATTERN(L"sht", L"শ্ত"),
    SIMPLE_PATTERN(L"shn", L"শ্ন"),
    SIMPLE_PATTERN(L"shm", L"শ্ম"),
    SIMPLE_PATTERN(L"shl", L"শ্ল"),
    SIMPLE_PATTERN(L"Shk", L"ষ্ক"),
    SIMPLE_PATTERN(L"ShT", L"ষ্ট"),
    SIMPLE_PATTERN(L"ShN", L"ষ্ণ"),
    SIMPLE_PATTERN(L"Shp", L"ষ্প"),
    SIMPLE_PATTERN(L"Shf", L"ষ্ফ"),
    SIMPLE_PATTERN(L"Shm", L"ষ্ম"),
    SIMPLE_PATTERN(L"spl", L"স্প্ল"),
    SIMPLE_PATTERN(L"sk", L"স্ক"),
    SIMPLE_PATTERN(L"Sc", L"শ্চ"),
    SIMPLE_PATTERN(L"sT", L"স্ট"),
    SIMPLE_PATTERN(L"st", L"স্ত"),
    SIMPLE_PATTERN(L"sn", L"স্ন"),
    SIMPLE_PATTERN(L"sp", L"স্প"),
    SIMPLE_PATTERN(L"sf", L"স্ফ"),
    SIMPLE_PATTERN(L"sm", L"স্ম"),
    SIMPLE_PATTERN(L"sl", L"স্ল"),
    SIMPLE_PATTERN(L"sh", L"শ"),
    SIMPLE_PATTERN(L"Sc", L"শ্চ"),
    SIMPLE_PATTERN(L"St", L"শ্ত"),
    SIMPLE_PATTERN(L"Sn", L"শ্ন"),
    SIMPLE_PATTERN(L"Sm", L"শ্ম"),
    SIMPLE_PATTERN(L"Sl", L"শ্ল"),
    SIMPLE_PATTERN(L"Sh", L"ষ"),
    SIMPLE_PATTERN(L"s", L"স"),
    SIMPLE_PATTERN(L"S", L"শ"),
    SIMPLE_PATTERN(L"oo`", L"ু"),
    SIMPLE_PATTERN(L"o`", L""),
    SIMPLE_PATTERN(L"oZ", L"অ্য"),
    SIMPLE_PATTERN(L"tth", L"ত্থ"),
    SIMPLE_PATTERN(L"t``", L"ৎ"),
    SIMPLE_PATTERN(L"TT", L"ট্ট"),
    SIMPLE_PATTERN(L"Tm", L"ট্ম"),
    SIMPLE_PATTERN(L"Th", L"ঠ"),
    SIMPLE_PATTERN(L"tn", L"ত্ন"),
    SIMPLE_PATTERN(L"tm", L"ত্ম"),
    SIMPLE_PATTERN(L"th", L"থ"),
    SIMPLE_PATTERN(L"tt", L"ত্ত"),
    SIMPLE_PATTERN(L"T", L"ট"),
    SIMPLE_PATTERN(L"t", L"ত"),
    SIMPLE_PATTERN(L"aZ", L"অ্যা"),
    SIMPLE_PATTERN(L"AZ", L"অ্যা"),
    SIMPLE_PATTERN(L"a`", L"া"),
    SIMPLE_PATTERN(L"A`", L"া"),
    SIMPLE_PATTERN(L"i`", L"ি"),
    SIMPLE_PATTERN(L"I`", L"ী"),
    SIMPLE_PATTERN(L"u`", L"ু"),
    SIMPLE_PATTERN(L"U`", L"ূ"),
    SIMPLE_PATTERN(L"ee`", L"ী"),
    SIMPLE_PATTERN(L"e`", L"ে"),
    SIMPLE_PATTERN(L"z", L"য"),
    SIMPLE_PATTERN(L"Z", L"্য"),
    SIMPLE_PATTERN(L"Y", L"য়"),
    SIMPLE_PATTERN(L"q", L"ক"),
    SIMPLE_PATTERN(L":`", L":"),
    SIMPLE_PATTERN(L":", L"ঃ"),
    SIMPLE_PATTERN(L"^`", L"^"),
    SIMPLE_PATTERN(L"^", L"ঁ"),
    SIMPLE_PATTERN(L",,", L"্‌"),
    SIMPLE_PATTERN(L",", L","),
    SIMPLE_PATTERN(L"$", L"৳"),
    SIMPLE_PATTERN(L"`", L"")
};

// ----- Rule definitions for patterns that have rules -----

// Rule for "OI"
static Match oi_matches1[] = { { PREFIX, L"!consonant", NULL, false } };
static Match oi_matches2[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule oi_rules[] = {
    { oi_matches1, 1, L"ঐ" },
    { oi_matches2, 1, L"ঐ" }
};

// Rule for "OU"
static Match ou_matches1[] = { { PREFIX, L"!consonant", NULL, false } };
static Match ou_matches2[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule ou_rules[] = {
    { ou_matches1, 1, L"ঔ" },
    { ou_matches2, 1, L"ঔ" }
};

// Rule for "O"
static Match o_matches1[] = { { PREFIX, L"!consonant", NULL, false } };
static Match o_matches2[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule o_rules[] = {
    { o_matches1, 1, L"ও" },
    { o_matches2, 1, L"ও" }
};

// Rule for "rri"
static Match rri_matches1[] = { { PREFIX, L"!consonant", NULL, false } };
static Match rri_matches2[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule rri_rules[] = {
    { rri_matches1, 1, L"ঋ" },
    { rri_matches2, 1, L"ঋ" }
};

// Rule for "rZ"
static Match rZ_matches[] = {
    { PREFIX, L"consonant", NULL, false },
    { PREFIX, L"!exact", L"r", false },
    { PREFIX, L"!exact", L"y", false },
    { PREFIX, L"!exact", L"w", false },
    { PREFIX, L"!exact", L"x", false }
};
static Rule rZ_rules[] = { { rZ_matches, 5, L"্র্য" } };

// Rule for "ry"
static Match ry_matches[] = {
    { PREFIX, L"consonant", NULL, false },
    { PREFIX, L"!exact", L"r", false },
    { PREFIX, L"!exact", L"y", false },
    { PREFIX, L"!exact", L"w", false },
    { PREFIX, L"!exact", L"x", false }
};
static Rule ry_rules[] = { { ry_matches, 5, L"্র্য" } };

// Rule for "rr"
static Match rr_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!vowel", NULL, false },
    { SUFFIX, L"!exact", L"r", false },
    { SUFFIX, L"!punctuation", NULL, false }
};
static Match rr_matches2[] = {
    { PREFIX, L"consonant", NULL, false },
    { PREFIX, L"!exact", L"r", false }
};
static Rule rr_rules[] = {
    { rr_matches1, 4, L"র্" },
    { rr_matches2, 2, L"্রর" }
};

// Rule for "r"
static Match r_matches[] = {
    { PREFIX, L"consonant", NULL, false },
    { PREFIX, L"!exact", L"r", false },
    { PREFIX, L"!exact", L"y", false },
    { PREFIX, L"!exact", L"w", false },
    { PREFIX, L"!exact", L"x", false },
    { PREFIX, L"!exact", L"Z", false }
};
static Rule r_rules[] = { { r_matches, 6, L"্র" } };

// Rule for "oo"
static Match oo_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match oo_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule oo_rules[] = {
    { oo_matches1, 2, L"উ" },
    { oo_matches2, 2, L"উ" }
};

// Rule for "o"
static Match o2_matches1[] = {
    { PREFIX, L"vowel", NULL, false },
    { PREFIX, L"!exact", L"o", false }
};
static Match o2_matches2[] = {
    { PREFIX, L"vowel", NULL, false },
    { PREFIX, L"exact", L"o", false }
};
static Match o2_matches3[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule o2_rules[] = {
    { o2_matches1, 2, L"ও" },
    { o2_matches2, 2, L"অ" },
    { o2_matches3, 1, L"অ" }
};

// Rule for "a"
static Match a_matches1[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match a_matches2[] = {
    { PREFIX, L"!consonant", NULL, false },
    { PREFIX, L"!exact", L"a", false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match a_matches3[] = {
    { PREFIX, L"exact", L"a", false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule a_rules[] = {
    { a_matches1, 2, L"আ" },
    { a_matches2, 3, L"য়া" },
    { a_matches3, 2, L"আ" }
};

// Rule for "i"
static Match i_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match i_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule i_rules[] = {
    { i_matches1, 2, L"ই" },
    { i_matches2, 2, L"ই" }
};

// Rule for "I"
static Match I_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match I_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule I_rules[] = {
    { I_matches1, 2, L"ঈ" },
    { I_matches2, 2, L"ঈ" }
};

// Rule for "u"
static Match u_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match u_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule u_rules[] = {
    { u_matches1, 2, L"উ" },
    { u_matches2, 2, L"উ" }
};

// Rule for "U"
static Match U_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match U_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule U_rules[] = {
    { U_matches1, 2, L"ঊ" },
    { U_matches2, 2, L"ঊ" }
};

// Rule for "ee"
static Match ee_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match ee_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule ee_rules[] = {
    { ee_matches1, 2, L"ঈ" },
    { ee_matches2, 2, L"ঈ" }
};

// Rule for "e"
static Match e_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Match e_matches2[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"!exact", L"`", false }
};
static Rule e_rules[] = {
    { e_matches1, 2, L"এ" },
    { e_matches2, 2, L"এ" }
};

// Rule for "y"
static Match y_matches1[] = {
    { PREFIX, L"!consonant", NULL, false },
    { PREFIX, L"!punctuation", NULL, false }
};
static Match y_matches2[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule y_rules[] = {
    { y_matches1, 2, L"য়" },
    { y_matches2, 1, L"ইয়" }
};

// Rule for "w"
static Match w_matches1[] = {
    { PREFIX, L"punctuation", NULL, false },
    { SUFFIX, L"vowel", NULL, false }
};
static Match w_matches2[] = { { PREFIX, L"consonant", NULL, false } };
static Rule w_rules[] = {
    { w_matches1, 2, L"ওয়" },
    { w_matches2, 1, L"্ব" }
};

// Rule for "x"
static Match x_matches[] = { { PREFIX, L"punctuation", NULL, false } };
static Rule x_rules[] = { { x_matches, 1, L"এক্স" } };

// ----- Patterns with rules (in the same order as JSON) -----
static Pattern rule_patterns[] = {
    { L"OI", L"ৈ", oi_rules, 2 },
    { L"OU", L"ৌ", ou_rules, 2 },
    { L"O", L"ো", o_rules, 2 },
    { L"rri", L"ৃ", rri_rules, 2 },
    { L"rZ", L"র‍্য", rZ_rules, 1 },
    { L"ry", L"র‍্য", ry_rules, 1 },
    { L"rr", L"রর", rr_rules, 2 },
    { L"r", L"র", r_rules, 1 },
    { L"oo", L"ু", oo_rules, 2 },
    { L"o", L"", o2_rules, 3 },
    { L"a", L"া", a_rules, 3 },
    { L"i", L"ি", i_rules, 2 },
    { L"I", L"ী", I_rules, 2 },
    { L"u", L"ু", u_rules, 2 },
    { L"U", L"ূ", U_rules, 2 },
    { L"ee", L"ী", ee_rules, 2 },
    { L"e", L"ে", e_rules, 2 },
    { L"y", L"্য", y_rules, 2 },
    { L"w", L"ও", w_rules, 2 },
    { L"x", L"ক্স", x_rules, 1 }
};

// ----- Combine all patterns into a single array in the correct order -----
// The JSON order: first all simple patterns (as listed above), then the rule patterns.
static Pattern all_patterns[] = {
    // Simple patterns
    SIMPLE_PATTERN(L"bhl", L"ভ্ল"),
    SIMPLE_PATTERN(L"psh", L"পশ"),
    SIMPLE_PATTERN(L"bdh", L"ব্ধ"),
    SIMPLE_PATTERN(L"bj", L"ব্জ"),
    SIMPLE_PATTERN(L"bd", L"ব্দ"),
    SIMPLE_PATTERN(L"bb", L"ব্ব"),
    SIMPLE_PATTERN(L"bl", L"ব্ল"),
    SIMPLE_PATTERN(L"bh", L"ভ"),
    SIMPLE_PATTERN(L"vl", L"ভ্ল"),
    SIMPLE_PATTERN(L"b", L"ব"),
    SIMPLE_PATTERN(L"v", L"ভ"),
    SIMPLE_PATTERN(L"cNG", L"চ্ঞ"),
    SIMPLE_PATTERN(L"cch", L"চ্ছ"),
    SIMPLE_PATTERN(L"cc", L"চ্চ"),
    SIMPLE_PATTERN(L"ch", L"ছ"),
    SIMPLE_PATTERN(L"c", L"চ"),
    SIMPLE_PATTERN(L"dhn", L"ধ্ন"),
    SIMPLE_PATTERN(L"dhm", L"ধ্ম"),
    SIMPLE_PATTERN(L"dgh", L"দ্ঘ"),
    SIMPLE_PATTERN(L"ddh", L"দ্ধ"),
    SIMPLE_PATTERN(L"dbh", L"দ্ভ"),
    SIMPLE_PATTERN(L"dv", L"দ্ভ"),
    SIMPLE_PATTERN(L"dm", L"দ্ম"),
    SIMPLE_PATTERN(L"DD", L"ড্ড"),
    SIMPLE_PATTERN(L"Dh", L"ঢ"),
    SIMPLE_PATTERN(L"dh", L"ধ"),
    SIMPLE_PATTERN(L"dg", L"দ্গ"),
    SIMPLE_PATTERN(L"dd", L"দ্দ"),
    SIMPLE_PATTERN(L"D", L"ড"),
    SIMPLE_PATTERN(L"d", L"দ"),
    SIMPLE_PATTERN(L"...", L"..."),
    SIMPLE_PATTERN(L".`", L"."),
    SIMPLE_PATTERN(L"..", L"।।"),
    SIMPLE_PATTERN(L".", L"।"),
    SIMPLE_PATTERN(L"ghn", L"ঘ্ন"),
    SIMPLE_PATTERN(L"Ghn", L"ঘ্ন"),
    SIMPLE_PATTERN(L"gdh", L"গ্ধ"),
    SIMPLE_PATTERN(L"Gdh", L"গ্ধ"),
    SIMPLE_PATTERN(L"gN", L"গ্ণ"),
    SIMPLE_PATTERN(L"GN", L"গ্ণ"),
    SIMPLE_PATTERN(L"gn", L"গ্ন"),
    SIMPLE_PATTERN(L"Gn", L"গ্ন"),
    SIMPLE_PATTERN(L"gm", L"গ্ম"),
    SIMPLE_PATTERN(L"Gm", L"গ্ম"),
    SIMPLE_PATTERN(L"gl", L"গ্ল"),
    SIMPLE_PATTERN(L"Gl", L"গ্ল"),
    SIMPLE_PATTERN(L"gg", L"জ্ঞ"),
    SIMPLE_PATTERN(L"GG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"Gg", L"জ্ঞ"),
    SIMPLE_PATTERN(L"gG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"gh", L"ঘ"),
    SIMPLE_PATTERN(L"Gh", L"ঘ"),
    SIMPLE_PATTERN(L"g", L"গ"),
    SIMPLE_PATTERN(L"G", L"গ"),
    SIMPLE_PATTERN(L"hN", L"হ্ণ"),
    SIMPLE_PATTERN(L"hn", L"হ্ন"),
    SIMPLE_PATTERN(L"hm", L"হ্ম"),
    SIMPLE_PATTERN(L"hl", L"হ্ল"),
    SIMPLE_PATTERN(L"h", L"হ"),
    SIMPLE_PATTERN(L"jjh", L"জ্ঝ"),
    SIMPLE_PATTERN(L"jNG", L"জ্ঞ"),
    SIMPLE_PATTERN(L"jh", L"ঝ"),
    SIMPLE_PATTERN(L"jj", L"জ্জ"),
    SIMPLE_PATTERN(L"j", L"জ"),
    SIMPLE_PATTERN(L"J", L"জ"),
    SIMPLE_PATTERN(L"kkhN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kShN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kkhm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kShm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kxN", L"ক্ষ্ণ"),
    SIMPLE_PATTERN(L"kxm", L"ক্ষ্ম"),
    SIMPLE_PATTERN(L"kkh", L"ক্ষ"),
    SIMPLE_PATTERN(L"kSh", L"ক্ষ"),
    SIMPLE_PATTERN(L"ksh", L"কশ"),
    SIMPLE_PATTERN(L"kx", L"ক্ষ"),
    SIMPLE_PATTERN(L"kk", L"ক্ক"),
    SIMPLE_PATTERN(L"kT", L"ক্ট"),
    SIMPLE_PATTERN(L"kt", L"ক্ত"),
    SIMPLE_PATTERN(L"kl", L"ক্ল"),
    SIMPLE_PATTERN(L"ks", L"ক্স"),
    SIMPLE_PATTERN(L"kh", L"খ"),
    SIMPLE_PATTERN(L"k", L"ক"),
    SIMPLE_PATTERN(L"lbh", L"ল্ভ"),
    SIMPLE_PATTERN(L"ldh", L"ল্ধ"),
    SIMPLE_PATTERN(L"lkh", L"লখ"),
    SIMPLE_PATTERN(L"lgh", L"লঘ"),
    SIMPLE_PATTERN(L"lph", L"লফ"),
    SIMPLE_PATTERN(L"lk", L"ল্ক"),
    SIMPLE_PATTERN(L"lg", L"ল্গ"),
    SIMPLE_PATTERN(L"lT", L"ল্ট"),
    SIMPLE_PATTERN(L"lD", L"ল্ড"),
    SIMPLE_PATTERN(L"lp", L"ল্প"),
    SIMPLE_PATTERN(L"lv", L"ল্ভ"),
    SIMPLE_PATTERN(L"lm", L"ল্ম"),
    SIMPLE_PATTERN(L"ll", L"ল্ল"),
    SIMPLE_PATTERN(L"lb", L"ল্ব"),
    SIMPLE_PATTERN(L"l", L"ল"),
    SIMPLE_PATTERN(L"mth", L"ম্থ"),
    SIMPLE_PATTERN(L"mph", L"ম্ফ"),
    SIMPLE_PATTERN(L"mbh", L"ম্ভ"),
    SIMPLE_PATTERN(L"mpl", L"মপ্ল"),
    SIMPLE_PATTERN(L"mn", L"ম্ন"),
    SIMPLE_PATTERN(L"mp", L"ম্প"),
    SIMPLE_PATTERN(L"mv", L"ম্ভ"),
    SIMPLE_PATTERN(L"mm", L"ম্ম"),
    SIMPLE_PATTERN(L"ml", L"ম্ল"),
    SIMPLE_PATTERN(L"mb", L"ম্ব"),
    SIMPLE_PATTERN(L"mf", L"ম্ফ"),
    SIMPLE_PATTERN(L"m", L"ম"),
    SIMPLE_PATTERN(L"0", L"০"),
    SIMPLE_PATTERN(L"1", L"১"),
    SIMPLE_PATTERN(L"2", L"২"),
    SIMPLE_PATTERN(L"3", L"৩"),
    SIMPLE_PATTERN(L"4", L"৪"),
    SIMPLE_PATTERN(L"5", L"৫"),
    SIMPLE_PATTERN(L"6", L"৬"),
    SIMPLE_PATTERN(L"7", L"৭"),
    SIMPLE_PATTERN(L"8", L"৮"),
    SIMPLE_PATTERN(L"9", L"৯"),
    SIMPLE_PATTERN(L"NgkSh", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"Ngkkh", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"NGch", L"ঞ্ছ"),
    SIMPLE_PATTERN(L"Nggh", L"ঙ্ঘ"),
    SIMPLE_PATTERN(L"Ngkh", L"ঙ্খ"),
    SIMPLE_PATTERN(L"NGjh", L"ঞ্ঝ"),
    SIMPLE_PATTERN(L"ngOU", L"ঙ্গৌ"),
    SIMPLE_PATTERN(L"ngOI", L"ঙ্গৈ"),
    SIMPLE_PATTERN(L"Ngkx", L"ঙ্ক্ষ"),
    SIMPLE_PATTERN(L"NGc", L"ঞ্চ"),
    SIMPLE_PATTERN(L"nch", L"ঞ্ছ"),
    SIMPLE_PATTERN(L"njh", L"ঞ্ঝ"),
    SIMPLE_PATTERN(L"ngh", L"ঙ্ঘ"),
    SIMPLE_PATTERN(L"Ngk", L"ঙ্ক"),
    SIMPLE_PATTERN(L"Ngx", L"ঙ্ষ"),
    SIMPLE_PATTERN(L"Ngg", L"ঙ্গ"),
    SIMPLE_PATTERN(L"Ngm", L"ঙ্ম"),
    SIMPLE_PATTERN(L"NGj", L"ঞ্জ"),
    SIMPLE_PATTERN(L"ndh", L"ন্ধ"),
    SIMPLE_PATTERN(L"nTh", L"ন্ঠ"),
    SIMPLE_PATTERN(L"NTh", L"ণ্ঠ"),
    SIMPLE_PATTERN(L"nth", L"ন্থ"),
    SIMPLE_PATTERN(L"nkh", L"ঙ্খ"),
    SIMPLE_PATTERN(L"ngo", L"ঙ্গ"),
    SIMPLE_PATTERN(L"nga", L"ঙ্গা"),
    SIMPLE_PATTERN(L"ngi", L"ঙ্গি"),
    SIMPLE_PATTERN(L"ngI", L"ঙ্গী"),
    SIMPLE_PATTERN(L"ngu", L"ঙ্গু"),
    SIMPLE_PATTERN(L"ngU", L"ঙ্গূ"),
    SIMPLE_PATTERN(L"nge", L"ঙ্গে"),
    SIMPLE_PATTERN(L"ngO", L"ঙ্গো"),
    SIMPLE_PATTERN(L"NDh", L"ণ্ঢ"),
    SIMPLE_PATTERN(L"nsh", L"নশ"),
    SIMPLE_PATTERN(L"Ngr", L"ঙর"),
    SIMPLE_PATTERN(L"NGr", L"ঞর"),
    SIMPLE_PATTERN(L"ngr", L"ংর"),
    SIMPLE_PATTERN(L"nj", L"ঞ্জ"),
    SIMPLE_PATTERN(L"Ng", L"ঙ"),
    SIMPLE_PATTERN(L"NG", L"ঞ"),
    SIMPLE_PATTERN(L"nk", L"ঙ্ক"),
    SIMPLE_PATTERN(L"ng", L"ং"),
    SIMPLE_PATTERN(L"nn", L"ন্ন"),
    SIMPLE_PATTERN(L"NN", L"ণ্ণ"),
    SIMPLE_PATTERN(L"Nn", L"ণ্ন"),
    SIMPLE_PATTERN(L"nm", L"ন্ম"),
    SIMPLE_PATTERN(L"Nm", L"ণ্ম"),
    SIMPLE_PATTERN(L"nd", L"ন্দ"),
    SIMPLE_PATTERN(L"nT", L"ন্ট"),
    SIMPLE_PATTERN(L"NT", L"ণ্ট"),
    SIMPLE_PATTERN(L"nD", L"ন্ড"),
    SIMPLE_PATTERN(L"ND", L"ণ্ড"),
    SIMPLE_PATTERN(L"nt", L"ন্ত"),
    SIMPLE_PATTERN(L"ns", L"ন্স"),
    SIMPLE_PATTERN(L"nc", L"ঞ্চ"),
    SIMPLE_PATTERN(L"n", L"ন"),
    SIMPLE_PATTERN(L"N", L"ণ"),
    SIMPLE_PATTERN(L"OI`", L"ৈ"),
    SIMPLE_PATTERN(L"OU`", L"ৌ"),
    SIMPLE_PATTERN(L"O`", L"ো"),
    SIMPLE_PATTERN(L"phl", L"ফ্ল"),
    SIMPLE_PATTERN(L"pT", L"প্ট"),
    SIMPLE_PATTERN(L"pt", L"প্ত"),
    SIMPLE_PATTERN(L"pn", L"প্ন"),
    SIMPLE_PATTERN(L"pp", L"প্প"),
    SIMPLE_PATTERN(L"pl", L"প্ল"),
    SIMPLE_PATTERN(L"ps", L"প্স"),
    SIMPLE_PATTERN(L"ph", L"ফ"),
    SIMPLE_PATTERN(L"fl", L"ফ্ল"),
    SIMPLE_PATTERN(L"f", L"ফ"),
    SIMPLE_PATTERN(L"p", L"প"),
    SIMPLE_PATTERN(L"rri`", L"ৃ"),
    SIMPLE_PATTERN(L"rrZ", L"রর‍্য"),
    SIMPLE_PATTERN(L"rry", L"রর‍্য"),
    SIMPLE_PATTERN(L"Rg", L"ড়্গ"),
    SIMPLE_PATTERN(L"Rh", L"ঢ়"),
    SIMPLE_PATTERN(L"R", L"ড়"),
    SIMPLE_PATTERN(L"shch", L"শ্ছ"),
    SIMPLE_PATTERN(L"ShTh", L"ষ্ঠ"),
    SIMPLE_PATTERN(L"Shph", L"ষ্ফ"),
    SIMPLE_PATTERN(L"Sch", L"শ্ছ"),
    SIMPLE_PATTERN(L"skl", L"স্ক্ল"),
    SIMPLE_PATTERN(L"skh", L"স্খ"),
    SIMPLE_PATTERN(L"sth", L"স্থ"),
    SIMPLE_PATTERN(L"sph", L"স্ফ"),
    SIMPLE_PATTERN(L"shc", L"শ্চ"),
    SIMPLE_PATTERN(L"sht", L"শ্ত"),
    SIMPLE_PATTERN(L"shn", L"শ্ন"),
    SIMPLE_PATTERN(L"shm", L"শ্ম"),
    SIMPLE_PATTERN(L"shl", L"শ্ল"),
    SIMPLE_PATTERN(L"Shk", L"ষ্ক"),
    SIMPLE_PATTERN(L"ShT", L"ষ্ট"),
    SIMPLE_PATTERN(L"ShN", L"ষ্ণ"),
    SIMPLE_PATTERN(L"Shp", L"ষ্প"),
    SIMPLE_PATTERN(L"Shf", L"ষ্ফ"),
    SIMPLE_PATTERN(L"Shm", L"ষ্ম"),
    SIMPLE_PATTERN(L"spl", L"স্প্ল"),
    SIMPLE_PATTERN(L"sk", L"স্ক"),
    SIMPLE_PATTERN(L"Sc", L"শ্চ"),
    SIMPLE_PATTERN(L"sT", L"স্ট"),
    SIMPLE_PATTERN(L"st", L"স্ত"),
    SIMPLE_PATTERN(L"sn", L"স্ন"),
    SIMPLE_PATTERN(L"sp", L"স্প"),
    SIMPLE_PATTERN(L"sf", L"স্ফ"),
    SIMPLE_PATTERN(L"sm", L"স্ম"),
    SIMPLE_PATTERN(L"sl", L"স্ল"),
    SIMPLE_PATTERN(L"sh", L"শ"),
    SIMPLE_PATTERN(L"Sc", L"শ্চ"),
    SIMPLE_PATTERN(L"St", L"শ্ত"),
    SIMPLE_PATTERN(L"Sn", L"শ্ন"),
    SIMPLE_PATTERN(L"Sm", L"শ্ম"),
    SIMPLE_PATTERN(L"Sl", L"শ্ল"),
    SIMPLE_PATTERN(L"Sh", L"ষ"),
    SIMPLE_PATTERN(L"s", L"স"),
    SIMPLE_PATTERN(L"S", L"শ"),
    SIMPLE_PATTERN(L"oo`", L"ু"),
    SIMPLE_PATTERN(L"o`", L""),
    SIMPLE_PATTERN(L"oZ", L"অ্য"),
    SIMPLE_PATTERN(L"tth", L"ত্থ"),
    SIMPLE_PATTERN(L"t``", L"ৎ"),
    SIMPLE_PATTERN(L"TT", L"ট্ট"),
    SIMPLE_PATTERN(L"Tm", L"ট্ম"),
    SIMPLE_PATTERN(L"Th", L"ঠ"),
    SIMPLE_PATTERN(L"tn", L"ত্ন"),
    SIMPLE_PATTERN(L"tm", L"ত্ম"),
    SIMPLE_PATTERN(L"th", L"থ"),
    SIMPLE_PATTERN(L"tt", L"ত্ত"),
    SIMPLE_PATTERN(L"T", L"ট"),
    SIMPLE_PATTERN(L"t", L"ত"),
    SIMPLE_PATTERN(L"aZ", L"অ্যা"),
    SIMPLE_PATTERN(L"AZ", L"অ্যা"),
    SIMPLE_PATTERN(L"a`", L"া"),
    SIMPLE_PATTERN(L"A`", L"া"),
    SIMPLE_PATTERN(L"i`", L"ি"),
    SIMPLE_PATTERN(L"I`", L"ী"),
    SIMPLE_PATTERN(L"u`", L"ু"),
    SIMPLE_PATTERN(L"U`", L"ূ"),
    SIMPLE_PATTERN(L"ee`", L"ী"),
    SIMPLE_PATTERN(L"e`", L"ে"),
    SIMPLE_PATTERN(L"z", L"য"),
    SIMPLE_PATTERN(L"Z", L"্য"),
    SIMPLE_PATTERN(L"Y", L"য়"),
    SIMPLE_PATTERN(L"q", L"ক"),
    SIMPLE_PATTERN(L":`", L":"),
    SIMPLE_PATTERN(L":", L"ঃ"),
    SIMPLE_PATTERN(L"^`", L"^"),
    SIMPLE_PATTERN(L"^", L"ঁ"),
    SIMPLE_PATTERN(L",,", L"্‌"),
    SIMPLE_PATTERN(L",", L","),
    SIMPLE_PATTERN(L"$", L"৳"),
    SIMPLE_PATTERN(L"`", L""),
    // Then rule patterns
    { L"OI", L"ৈ", oi_rules, 2 },
    { L"OU", L"ৌ", ou_rules, 2 },
    { L"O", L"ো", o_rules, 2 },
    { L"rri", L"ৃ", rri_rules, 2 },
    { L"rZ", L"র‍্য", rZ_rules, 1 },
    { L"ry", L"র‍্য", ry_rules, 1 },
    { L"rr", L"রর", rr_rules, 2 },
    { L"r", L"র", r_rules, 1 },
    { L"oo", L"ু", oo_rules, 2 },
    { L"o", L"", o2_rules, 3 },
    { L"a", L"া", a_rules, 3 },
    { L"i", L"ি", i_rules, 2 },
    { L"I", L"ী", I_rules, 2 },
    { L"u", L"ু", u_rules, 2 },
    { L"U", L"ূ", U_rules, 2 },
    { L"ee", L"ী", ee_rules, 2 },
    { L"e", L"ে", e_rules, 2 },
    { L"y", L"্য", y_rules, 2 },
    { L"w", L"ও", w_rules, 2 },
    { L"x", L"ক্স", x_rules, 1 }
};

// ------------------------------------------------------------------
// Main parsing function (ported from JS/Go)
// ------------------------------------------------------------------
wchar_t* avro_parse(const wchar_t* input) {
    wchar_t* fixed = fix_string(input);
    if (!fixed) return NULL;

    size_t len = wcslen(fixed);
    // Output buffer – overallocate (worst case: each input char becomes multiple output chars)
    wchar_t* output = malloc((len * 4 + 1) * sizeof(wchar_t));
    if (!output) { free(fixed); return NULL; }
    wchar_t* out_ptr = output;

    size_t cur = 0;
    while (cur < len) {
        int start = (int)cur;
        int matched = 0;
        // Try patterns in order
        size_t num_patterns = sizeof(all_patterns) / sizeof(Pattern);
        for (size_t i = 0; i < num_patterns; i++) {
            const Pattern* pat = &all_patterns[i];
            int end = start + (int)wcslen(pat->find);
            if (end > (int)len) continue;
            if (wcsncmp(fixed + start, pat->find, end - start) == 0) {
                int prev = start - 1;
                // If rules exist, evaluate them
                if (pat->rule_count > 0) {
                    for (int r = 0; r < pat->rule_count; r++) {
                        const Rule* rule = &pat->rules[r];
                        bool replace_ok = true;
                        for (int m = 0; m < rule->match_count; m++) {
                            const Match* match = &rule->matches[m];
                            int chk = (match->type == SUFFIX) ? end : prev;

                            // Determine scope and negative
                            bool negative = match->negative;
                            const wchar_t* scope = match->scope;
                            if (scope[0] == L'!' && !negative) {
                                negative = true;
                                scope++;
                            }
                            const wchar_t* value = match->value ? match->value : L"";

                            bool cond = false;
                            if (wcscmp(scope, L"punctuation") == 0) {
                                if ((chk < 0 && match->type == PREFIX) || (chk >= (int)len && match->type == SUFFIX))
                                    cond = true;
                                else if (chk >= 0 && chk < (int)len)
                                    cond = avro_is_punctuation(fixed[chk]);
                            } else if (wcscmp(scope, L"vowel") == 0) {
                                if ((chk >= 0 && match->type == PREFIX) || (chk < (int)len && match->type == SUFFIX)) {
                                    if (chk >= 0 && chk < (int)len)
                                        cond = avro_is_vowel(fixed[chk]);
                                }
                            } else if (wcscmp(scope, L"consonant") == 0) {
                                if ((chk >= 0 && match->type == PREFIX) || (chk < (int)len && match->type == SUFFIX)) {
                                    if (chk >= 0 && chk < (int)len)
                                        cond = avro_is_consonant(fixed[chk]);
                                }
                            } else if (wcscmp(scope, L"exact") == 0) {
                                int s, e;
                                if (match->type == SUFFIX) {
                                    s = end;
                                    e = end + (int)wcslen(value);
                                } else {
                                    s = start - (int)wcslen(value);
                                    e = start;
                                }
                                cond = is_exact(value, fixed, s, e, negative);
                                if (!cond) { replace_ok = false; break; }
                                continue; // skip negative flip for exact
                            }
                            if (negative && wcscmp(scope, L"exact") != 0) cond = !cond;
                            if (!cond) { replace_ok = false; break; }
                        }
                        if (replace_ok) {
                            wcscpy(out_ptr, rule->replace);
                            out_ptr += wcslen(rule->replace);
                            cur = end;
                            matched = 1;
                            break;
                        }
                    }
                    if (matched) break;
                }
                // No rules or no rule matched -> use default replace
                if (!matched) {
                    wcscpy(out_ptr, pat->replace);
                    out_ptr += wcslen(pat->replace);
                    cur = end;
                    matched = 1;
                    break;
                }
            }
        }
        if (!matched) {
            *out_ptr = fixed[cur];
            out_ptr++;
            cur++;
        }
    }
    *out_ptr = L'\0';
    free(fixed);
    return output;
}
