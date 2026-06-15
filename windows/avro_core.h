#ifndef AVRO_CORE_H
#define AVRO_CORE_H

#include <wchar.h>

/**
 * Avro Bangla Phonetic Parser
 * Converts Latin input to Bengali text using Avro rules
 */

// Main parsing function - converts Latin input to Bengali
// Returns newly allocated wide string (caller must free)
wchar_t* avro_parse(const wchar_t* input);

// Helper to check if character is a vowel
int avro_is_vowel(wchar_t c);

// Helper to check if character is a consonant  
int avro_is_consonant(wchar_t c);

// Helper to check if character is punctuation
int avro_is_punctuation(wchar_t c);

#endif // AVRO_CORE_H
