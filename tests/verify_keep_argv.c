#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *readfile(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    assert(f);
    assert(!fseek(f, 0, SEEK_END));
    long length = ftell(f);
    assert(length >= 0);
    assert(!fseek(f, 0, SEEK_SET));
    unsigned char *buf = malloc((size_t)length + 1);
    assert(buf);
    assert(fread(buf, 1, (size_t)length, f) == (size_t)length);
    assert(!fclose(f));
    *size = (size_t)length;
    return buf;
}

int main(int argc, char **argv)
{
    const char *expected[] = {"start", "-a", "android.intent.action.SEND",
        "-t", "text/plain", "-p", "com.google.android.keep",
        "--es", "android.intent.extra.TEXT"};
    assert(argc == 3);
    size_t bytes, body_len, at = 0;
    unsigned char *actual = readfile(argv[1], &bytes);
    unsigned char *body = readfile(argv[2], &body_len);
    for (size_t i = 0; i < sizeof expected / sizeof expected[0]; ++i) {
        size_t n = strlen(expected[i]);
        assert(bytes - at >= n + 1);
        assert(!memcmp(actual + at, expected[i], n));
        assert(actual[at + n] == 0);
        at += n + 1;
    }
    assert(bytes - at == body_len + 1);
    assert(!memcmp(actual + at, body, body_len));
    assert(actual[at + body_len] == 0);
    free(actual);
    free(body);
    return 0;
}
