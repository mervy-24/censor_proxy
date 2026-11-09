
#ifndef CENSOR_H
#define CENSOR_H

#include <stddef.h>

typedef struct {
	char **words; // Array of strings.
	size_t count; // Word count in wordlist.
} wordlist_t;

int wordlist_load(const char *path, wordlist_t *wl); // Returns the number of words loaded, or -1 on failure.

size_t censor_line(const char *in, size_t in_len, char *out, size_t out_size, const wordlist_t *wl); // Returns the num,ber of bytes written out.

void wordlist_free(wordlist_t *wl);

#endif

