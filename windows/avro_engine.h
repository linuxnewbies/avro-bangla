/*
 * Avro Bangla Phonetic Engine - Pure C Implementation
 * Compatible with Windows IME and FreeBSD/Go implementations
 */

#ifndef AVRO_ENGINE_H
#define AVRO_ENGINE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

/* Maximum lengths for buffers */
#define MAX_PATTERN_FIND_LEN 16
#define MAX_PATTERN_REPLACE_LEN 16
#define MAX_INPUT_BUFFER_LEN 512
#define MAX_OUTPUT_BUFFER_LEN 1024

/* Rule types for context-sensitive matching */
typedef enum {
    RULE_TYPE_PREFIX,
    RULE_TYPE_SUFFIX
} RuleType;

/* Rule scope types */
typedef enum {
    SCOPE_VOWEL,
    SCOPE_CONSONANT,
    SCOPE_PUNCTUATION,
    SCOPE_EXACT,
    SCOPE_NOT_VOWEL,
    SCOPE_NOT_CONSONANT,
    SCOPE_NOT_PUNCTUATION,
    SCOPE_NOT_EXACT
} RuleScope;

/* Single rule definition */
typedef struct {
    RuleType type;
    RuleScope scope;
    char value[MAX_PATTERN_FIND_LEN];  /* For exact match values */
} Rule;

/* Pattern definition */
typedef struct {
    char find[MAX_PATTERN_FIND_LEN];
    char replace[MAX_PATTERN_REPLACE_LEN];
    Rule* rules;
    int rule_count;
} Pattern;

/* Avro Engine state */
typedef struct {
    Pattern* patterns;
    int pattern_count;
    char input_buffer[MAX_INPUT_BUFFER_LEN];
    int buffer_len;
} AvroEngine;

/* Helper strings for rule evaluation */
extern const char* AVRO_VOWELS;
extern const char* AVRO_CONSONANTS;
extern const char* AVRO_CASE_SENSITIVE;

/* Initialize the Avro engine with patterns */
EXPORT void avro_engine_init(AvroEngine* engine);

/* Cleanup engine resources */
EXPORT void avro_engine_cleanup(AvroEngine* engine);

/* Process a single character input
 * Returns: number of characters in output (0 if no output yet)
 */
EXPORT int avro_engine_process_char(AvroEngine* engine, char ch, wchar_t* output, int output_max_len);

/* Get current pre-edit text (for display before committing) */
EXPORT int avro_engine_get_preedit(AvroEngine* engine, wchar_t* output, int output_max_len);

/* Reset the engine state (clear buffer) */
EXPORT void avro_engine_reset(AvroEngine* engine);

/* Check if a character is a vowel */
EXPORT bool avro_is_vowel(char ch);

/* Check if a character is a consonant */
EXPORT bool avro_is_consonant(char ch);

/* Check if a character is punctuation */
EXPORT bool avro_is_punctuation(char ch);

#endif /* AVRO_ENGINE_H */
