/*
 * Independent narrow Version 1 *consumer*, not a renderer or alternate HTML
 * parser. The producer/Idriç serializer is not linked into this executable.
 * It validates the published complete projection's grammar and unsafe targets.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

enum { LIMIT_BYTES = 4194304, LIMIT_BLOCKS = 4096, LIMIT_REVISIONS = 32 };
static int starts(const char *value, const char *prefix) {
  return strncmp(value, prefix, strlen(prefix)) == 0;
}
static int http_url(const char *value) {
  const char *host;
  if (starts(value, "https://")) host = value + 8;
  else if (starts(value, "http://")) host = value + 7;
  else return 0;
  if (!*host || *host == '/' || *host == '?' || *host == '#') return 0;
  for (const char *p = host; *p && *p != '/' && *p != '?' && *p != '#'; ++p) {
    if ((unsigned char)*p <= 32 || (unsigned char)*p >= 127 || *p == '@') return 0;
  }
  for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
    if (*p <= 32 || *p >= 127) return 0;
  return 1;
}
static int local_image(const char *value) {
  return starts(value, "content:") || starts(value, "asset:") ||
    starts(value, "resource:") || starts(value, "file:");
}
/* Reject malformed UTF-8, overlong sequences, surrogates and >U+10FFFF. */
static int valid_utf8(const unsigned char *p, size_t n) {
  for (size_t i = 0; i < n;) {
    unsigned c = p[i++];
    if (c < 0x80) { if (c == 0) return 0; continue; }
    unsigned need, min;
    if (c >= 0xc2 && c <= 0xdf) { need = 1; min = 0x80; c &= 0x1f; }
    else if (c >= 0xe0 && c <= 0xef) { need = 2; min = 0x800; c &= 0x0f; }
    else if (c >= 0xf0 && c <= 0xf4) { need = 3; min = 0x10000; c &= 0x07; }
    else return 0;
    if (i + need > n) return 0;
    for (unsigned j = 0; j < need; ++j) {
      unsigned d = p[i++];
      if ((d & 0xc0) != 0x80) return 0;
      c = (c << 6) | (d & 0x3f);
    }
    if (c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) return 0;
  }
  return 1;
}
static int number(const char *value, unsigned long *out) {
  if (!*value) return 0;
  unsigned long x = 0;
  for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
    if (*p < '0' || *p > '9' || x > (ULONG_MAX - (*p - '0')) / 10) return 0;
    x = x * 10 + *p - '0';
  }
  *out = x;
  return 1;
}
/* Every tab is a structural delimiter; literal tabs in values must be \\t. */
static int fields(char *line, char **parts, size_t *count) {
  size_t n = 0;
  parts[n++] = line;
  for (char *p = line; *p; ++p) {
    if (*p == '\r' || *p == '\n') return 0;
    if (*p == '\\') {
      if (!p[1] || !(p[1] == 'n' || p[1] == 'r' ||
                      p[1] == 't' || p[1] == '\\')) return 0;
      ++p;
    } else if (*p == '\t') {
      if (n >= LIMIT_BLOCKS + 5) return 0;
      *p = 0;
      parts[n++] = p + 1;
    }
  }
  *count = n;
  return 1;
}
static int check_file(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) return 0;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
  long length = ftell(f);
  if (length < 0 || length > LIMIT_BYTES || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f); return 0;
  }
  size_t n = (size_t)length;
  char *buf = malloc(n + 1);
  char **parts = malloc((LIMIT_BLOCKS + 5) * sizeof *parts);
  if (!buf || !parts) { free(buf); free(parts); fclose(f); return 0; }
  int ok = fread(buf, 1, n, f) == n && !ferror(f);
  fclose(f);
  if (!ok || !valid_utf8((const unsigned char *)buf, n)) {
    free(buf); free(parts); return 0;
  }
  buf[n] = 0;
  unsigned long previous = 0;
  unsigned revisions = 0, blocks = 0;
  /* state 0=header, 1=revision, 2=requested, 3=resolved,
   * 4=title, 5=blocks/end */
  int state = 0;
  size_t cursor = 0;
  for (; cursor < n && ok;) {
    char *line = buf + cursor;
    char *end = memchr(line, '\n', n - cursor);
    if (!end) { ok = 0; break; }
    *end = 0;
    cursor = (size_t)(end - buf) + 1;
    size_t fields_count = 0;
    if (!fields(line, parts, &fields_count)) { ok = 0; break; }
    const char *kind = parts[0];
    if (state == 0) {
      ok = fields_count == 2 && strcmp(kind, "ib-prepaint") == 0 &&
        strcmp(parts[1], "1") == 0;
      state = 1;
    } else if (state == 1) {
      unsigned long seq = 0;
      ok = fields_count == 3 && strcmp(kind, "revision") == 0 &&
        number(parts[1], &seq) && (revisions == 0 || seq > previous) &&
        strcmp(parts[2], "complete") == 0 &&
        ++revisions <= LIMIT_REVISIONS;
      previous = seq; blocks = 0; state = 2;
    } else if (state == 2) {
      ok = fields_count == 2 && strcmp(kind, "requested-url") == 0 &&
        http_url(parts[1]); state = 3;
    } else if (state == 3) {
      ok = fields_count == 2 && strcmp(kind, "resolved-url") == 0 &&
        http_url(parts[1]); state = 4;
    } else if (state == 4) {
      ok = fields_count == 2 && strcmp(kind, "title") == 0;
      state = 5;
    } else if (strcmp(kind, "end") == 0) {
      ok = fields_count == 1;
      state = 1;
    } else {
      ++blocks;
      ok = blocks <= LIMIT_BLOCKS;
      if (strcmp(kind, "heading") == 0) {
        ok = ok && fields_count == 3 && strlen(parts[1]) == 1 &&
          parts[1][0] >= '1' && parts[1][0] <= '6';
      } else if (strcmp(kind, "text") == 0) {
        ok = ok && fields_count == 2;
      } else if (strcmp(kind, "link") == 0) {
        ok = ok && fields_count == 3 && http_url(parts[2]);
      } else if (strcmp(kind, "row") == 0) {
        ok = ok && fields_count >= 2;
      } else if (strcmp(kind, "form") == 0) {
        /* The current converter emits non-interactive form descriptions. */
        ok = 0;
      } else if (strcmp(kind, "image") == 0) {
        ok = ok && (fields_count == 4 || fields_count == 5) &&
          local_image(parts[1]) &&
          (fields_count != 5 || http_url(parts[4]));
      } else ok = 0;
    }
  }
  ok = ok && revisions > 0 && state == 1 && cursor == n;
  free(buf);
  free(parts);
  return ok;
}
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: prepaint-v1-oracle FILE\n");
    return 2;
  }
  if (!check_file(argv[1])) {
    fprintf(stderr, "independent-prepaint-v1: REFUSED\n");
    return 1;
  }
  puts("independent-prepaint-v1: PASS");
  return 0;
}
