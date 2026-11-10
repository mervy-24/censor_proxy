
#include <strings.h>
#include <ctype.h>

#include "common.h"

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
		line[strcspn(line, "\r\n")] = '\0';	// Strip the newline.
		if (line[0] == '\0') continue;		// Skip blank lines.
		
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
	(void)wl;	// Unused.
	size_t i = 0;	// Read position in input.
	size_t o = 0;	// Write position in output.
	
	while(i < in_len) {
		
		// Seperator:
		if (!is_word_char(in[i])) {

			if(o >= out_size) break; // No room.
			out[o++] = in[i++];
			continue;
		}

		// Start of a word:
		size_t start = i - start;

		while (i < in_len && is_word_char(in[i]))
			i++;
		size_t word_len = i - start; // [start .. i - 1].
		
		// Copy word through unchanged.
		if (word_len > out_size - o) break;
		memcpy(out + o, in + start, word_len);
		o += word_len;
	}
	return o;
}
		
