package main

import (
	"unicode"
)

// AvroEngine handles phonetic conversion from Latin to Bengali
type AvroEngine struct {
	patterns []Pattern
}

// NewAvroEngine creates a new AvroEngine instance
func NewAvroEngine() *AvroEngine {
	return &AvroEngine{
		patterns: AvroPatterns,
	}
}

// isVowel checks if a character is a vowel in Avro context
func (e *AvroEngine) isVowel(ch rune) bool {
	for _, v := range AvroVowel {
		if ch == v {
			return true
		}
	}
	return false
}

// isConsonant checks if a character is a consonant in Avro context
func (e *AvroEngine) isConsonant(ch rune) bool {
	for _, c := range AvroConsonant {
		if ch == c {
			return true
		}
	}
	return false
}

// isCaseSensitive checks if a character is case-sensitive in Avro context
func (e *AvroEngine) isCaseSensitive(ch rune) bool {
	for _, c := range AvroCaseSensitive {
		if ch == c {
			return true
		}
	}
	return false
}

// isPunctuation checks if a character is punctuation
func (e *AvroEngine) isPunctuation(ch rune) bool {
	return unicode.IsPunct(ch) || unicode.IsSymbol(ch)
}

// matchesCondition checks if a match condition is satisfied
func (e *AvroEngine) matchesCondition(match Match, buffer string, pos int, findLen int) bool {
	var checkChar rune
	var hasChar bool

	switch match.Type {
	case "prefix":
		// Check character before the pattern
		if pos > 0 {
			checkChar = rune(buffer[pos-1])
			hasChar = true
		} else {
			hasChar = false
		}
	case "suffix":
		// Check character after the pattern
		if pos+findLen < len(buffer) {
			checkChar = rune(buffer[pos+findLen])
			hasChar = true
		} else {
			hasChar = false
		}
	default:
		return false
	}

	// Evaluate based on scope
	var matches bool
	switch match.Scope {
	case "vowel":
		matches = hasChar && e.isVowel(checkChar)
	case "consonant":
		matches = hasChar && e.isConsonant(checkChar)
	case "punctuation":
		matches = hasChar && e.isPunctuation(checkChar)
	case "exact":
		matches = hasChar && string(checkChar) == match.Value
	default:
		matches = false
	}

	// Handle negation
	if match.Negative {
		return !matches
	}
	return matches
}

// evaluateRules checks if all rules for a pattern are satisfied
func (e *AvroEngine) evaluateRules(rules []Rule, buffer string, pos int, findLen int) (string, bool) {
	for _, rule := range rules {
		allMatches := true
		for _, match := range rule.Matches {
			if !e.matchesCondition(match, buffer, pos, findLen) {
				allMatches = false
				break
			}
		}
		if allMatches {
			return rule.Replace, true
		}
	}
	return "", false
}

// Parse converts Latin text to Bengali using Avro phonetic rules
func (e *AvroEngine) Parse(input string) string {
	if input == "" {
		return ""
	}

	result := ""
	buffer := input
	i := 0

	for i < len(buffer) {
		// Try to find the longest matching pattern starting at position i
		found := false
		maxLen := 0
		bestReplace := ""

		// Sort patterns by length (longest first) for longest-match behavior
		// We'll iterate through all patterns and find the longest match
		for _, pattern := range e.patterns {
			findLen := len(pattern.Find)
			if findLen <= maxLen {
				continue // Skip shorter patterns
			}

			// Check if pattern matches at current position
			if i+findLen <= len(buffer) && buffer[i:i+findLen] == pattern.Find {
				// Check if there are conditional rules
				if len(pattern.Rules) > 0 {
					replace, ok := e.evaluateRules(pattern.Rules, buffer, i, findLen)
					if ok {
						maxLen = findLen
						bestReplace = replace
						found = true
					}
				} else {
					// No rules, direct replacement
					maxLen = findLen
					bestReplace = pattern.Replace
					found = true
				}
			}
		}

		if found {
			result += bestReplace
			i += maxLen
		} else {
			// No pattern matched, keep the original character
			result += string(buffer[i])
			i++
		}
	}

	return result
}

// ParseWithBuffer maintains state for incremental parsing (for IBus integration)
func (e *AvroEngine) ParseWithBuffer(buffer string) (converted string, remaining string) {
	// For now, just parse the entire buffer
	// In a full implementation, this would handle partial matches at the end
	converted = e.Parse(buffer)
	remaining = ""
	return converted, remaining
}

// Helper function to check if a string ends with a vowel
func endsWithVowel(s string) bool {
	if len(s) == 0 {
		return false
	}
	lastChar := rune(s[len(s)-1])
	for _, v := range AvroVowel {
		if lastChar == v {
			return true
		}
	}
	return false
}

// Helper function to check if a string ends with a consonant
func endsWithConsonant(s string) bool {
	if len(s) == 0 {
		return false
	}
	lastChar := rune(s[len(s)-1])
	for _, c := range AvroConsonant {
		if lastChar == c {
			return true
		}
	}
	return false
}

// Helper function to check if a string ends with punctuation
func endsWithPunctuation(s string) bool {
	if len(s) == 0 {
		return false
	}
	lastChar := rune(s[len(s)-1])
	return unicode.IsPunct(lastChar) || unicode.IsSymbol(lastChar)
}

// CommitTrigger checks if the current input should trigger a commit
func (e *AvroEngine) CommitTrigger(buffer string, key rune) bool {
	// Space triggers commit
	if key == ' ' {
		return true
	}
	// Punctuation triggers commit
	if unicode.IsPunct(key) || unicode.IsSymbol(key) {
		return true
	}
	return false
}
