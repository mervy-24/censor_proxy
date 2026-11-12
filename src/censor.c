
#include <strings.h>
#include <ctype.h>

#include "common.h"

// Return positive if element 'a' should come after 'b' when 'a' is shorter:
static int cmp_len_desc(const void *a, const void *b) {
	size_t la = strlen(*(char * const *)a);
	size_t lb = strlen(*(char * const *)b);

	if (la < lb) return 1;	// a is shorter.
	if (la > lb) return -1;	// a is longer.

	return 0;
}

// Returns the number of words loaded or -1 on failure:
int wordlist_load(const char *path, wordlist_t *wl) {
	// Start from empty state:
	wl->words = NULL;
	wl->count = 0;
	FILE *f = fopen(path, "r");

	if (f == NULL) {
		perror(path);
		return -1;
	}
	char line[256]; // One line of file (reused every pass).
	size_t cap = 0; // How many slots the array has room for.

	// fgets loop on file:
	while (fgets(line, sizeof(line), f) != NULL) {
		size_t len = strcspn(line, "\r\n"); 	// Index of the new line.
		line[len] = '\0';			// Strip the new line.
		
		// Walk back from the end while the last character is a whitespace:
		while (len > 0 && isspace((unsigned char)line[len - 1]))
			len --;
		line[len] = '\0'; // Cut the string at the new end.

		if (len == 0) continue; // Skip lines that are empty.

		// Grow the array when full:
		if (wl->count == cap) {
			size_t new_cap = cap ? cap * 2 : 16; // Double size and allocate at 16 slots.
			char **tmp = realloc(wl->words, new_cap * sizeof(char *));

			if (tmp == NULL) {
				perror("realloc");
				fclose(f);
				wordlist_free(wl); // Release loaded words.
				return -1;
			}
			wl->words = tmp;
			cap = new_cap;
		}
		wl->words[wl->count] = strdup(line);
		
		if (wl-> words[wl->count] == NULL) {
			perror("strdup");
			fclose(f);
			wordlist_free(wl);
			return -1;
		}
		wl->count++;
	}

	// Sort longest words first:
	if (wl->count > 1) {
		qsort(wl->words, wl->count, sizeof(char *), cmp_len_desc);
	}

	fclose(f);
	return (int)wl->count; // Number of words loaded.
}

// Release everything allocated from wordlist_load:
void wordlist_free(wordlist_t *wl) {

	for (size_t i = 0; i < wl->count; i++)
		free(wl->words[i]);
	free(wl->words); // Free the array that held the pointers.

	// Reset to empty state so a second call is harmless:
	wl->words = NULL;
	wl->count = 0;
}

static int is_word_char(char c) {
	return isalnum((unsigned char)c);
}

size_t censor_line(const char *in, size_t in_len, char *out, size_t out_size, const wordlist_t *wl) {
	static const unsigned char block[3] = { 0xE2, 0x96, 0x88 }; // The three UTF-8 bytes of the full block character.
	size_t i = 0; // Read position in input.
	size_t o = 0; // Write position in output.

	while (i < in_len) {
		size_t match_len = 0; // 0 = no banned entry.
				      
		// Case 1: Only try match at the start of a word:
		if (i == 0 || !is_word_char(in[i - 1])) {

			for (size_t w = 0; w < wl->count; w++) {
				const char *entry = wl->words[w];
				size_t elen = strlen(entry);
				if (elen == 0 || elen > in_len - i) continue; // Skip empty entries.
				
				// Case 2: case-insensitive match of exactly elen bytes:
				if (strncasecmp(in + i, entry, elen) != 0) continue;

				// Case 3: the byte after the match must be end of input or a non-word character:
				if (i + elen < in_len && is_word_char(in[i + elen])) continue;
				match_len = elen;
				break; // First match wins.
			}
		}

		if (match_len > 0) {
			
			// One block per byte of entry (all-or-nothing):
			for (size_t k = 0; k < match_len; k++) {
				if (out_size - o < 3) return o;
				memcpy(out + o, block, 3);
				o += 3;
			}
			i += match_len; // Skip whole censored span.
		} else {

			// No match; copy one byte and move on:
			if (o >= out_size) return o;
			out[o++] = in[i++];
		}
	}
	return 0;
}
