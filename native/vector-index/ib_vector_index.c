#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FORMAT_HEADER "ib-vector-index 2"
#define BACKEND_NAME "flat-f32-exact"
#define PATH_BUFFER 4096
#define TEXT_SCALAR_BYTES 15
#define TEXT_SLOT_BYTES 16

enum metric_kind {
  METRIC_DOT,
  METRIC_COSINE
};

struct manifest {
  enum metric_kind metric;
  size_t dimensions;
  size_t count;
  char ids_file[256];
  char vectors_text_file[256];
  char vectors_cache_file[256];
};

struct match {
  float score;
  size_t row;
  char *id;
};

static int fail(const char *message) {
  fprintf(stderr, "ib-vector-index: %s\n", message);
  return 1;
}

static int fail_path(const char *action, const char *path) {
  fprintf(stderr, "ib-vector-index: %s %s: %s\n", action, path,
          strerror(errno));
  return 1;
}

static int path_join(char *out, size_t capacity, const char *directory,
                     const char *name) {
  int written = snprintf(out, capacity, "%s/%s", directory, name);
  if (written < 0 || (size_t)written >= capacity) {
    fprintf(stderr, "ib-vector-index: path is too long: %s/%s\n", directory,
            name);
    return 0;
  }
  return 1;
}

static int plain_filename(const char *name) {
  return name[0] != '\0' && strcmp(name, ".") != 0 &&
         strcmp(name, "..") != 0 && strchr(name, '/') == NULL;
}

static int make_directories(const char *path) {
  char copy[PATH_BUFFER];
  size_t length = strlen(path);
  size_t start = 1;

  if (length == 0 || length >= sizeof(copy)) {
    return fail("index directory is empty or too long");
  }

  memcpy(copy, path, length + 1);
  if (copy[length - 1] == '/' && length > 1) {
    copy[length - 1] = '\0';
  }
  if (copy[0] != '/') {
    start = 0;
  }

  for (size_t i = start; copy[i] != '\0'; ++i) {
    if (copy[i] != '/') {
      continue;
    }
    copy[i] = '\0';
    if (copy[0] != '\0' && mkdir(copy, 0700) != 0 && errno != EEXIST) {
      return fail_path("cannot create", copy);
    }
    copy[i] = '/';
  }

  if (mkdir(copy, 0700) != 0 && errno != EEXIST) {
    return fail_path("cannot create", copy);
  }
  return 0;
}

static int flush_file(FILE *file, const char *path) {
  if (fflush(file) != 0) {
    return fail_path("cannot flush", path);
  }
  if (fsync(fileno(file)) != 0) {
    return fail_path("cannot synchronize", path);
  }
  return 0;
}

static FILE *create_exclusive(const char *path) {
  int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (descriptor < 0) {
    return NULL;
  }
  FILE *file = fdopen(descriptor, "wb");
  if (file == NULL) {
    int saved_errno = errno;
    close(descriptor);
    unlink(path);
    errno = saved_errno;
  }
  return file;
}

static int parse_size(const char *text, size_t *value) {
  char *end = NULL;
  unsigned long long parsed;

  errno = 0;
  parsed = strtoull(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || parsed == 0 ||
      parsed > SIZE_MAX) {
    return 0;
  }
  *value = (size_t)parsed;
  return 1;
}

static const char *metric_text(enum metric_kind metric) {
  return metric == METRIC_COSINE ? "cosine" : "dot";
}

static int parse_metric(const char *text, enum metric_kind *metric) {
  if (strcmp(text, "dot") == 0) {
    *metric = METRIC_DOT;
    return 1;
  }
  if (strcmp(text, "cosine") == 0) {
    *metric = METRIC_COSINE;
    return 1;
  }
  return 0;
}

static int host_is_little_endian(void) {
  const uint16_t one = 1;
  return *((const unsigned char *)&one) == 1;
}

static int write_float32_le(FILE *file, float value) {
  unsigned char bytes[4];
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  bytes[0] = (unsigned char)(bits & 0xffu);
  bytes[1] = (unsigned char)((bits >> 8u) & 0xffu);
  bytes[2] = (unsigned char)((bits >> 16u) & 0xffu);
  bytes[3] = (unsigned char)((bits >> 24u) & 0xffu);
  return fwrite(bytes, sizeof(bytes), 1, file) == 1;
}

static int write_float32_text(FILE *file, float value, int last_in_row) {
  char slot[TEXT_SLOT_BYTES + 1];
  int written = snprintf(slot, sizeof(slot), "%+.8e%c", (double)value,
                         last_in_row ? '\n' : ' ');
  return written == TEXT_SLOT_BYTES &&
         fwrite(slot, TEXT_SLOT_BYTES, 1, file) == 1;
}

static int read_float32_text(const unsigned char *slot, float *value) {
  char number[TEXT_SCALAR_BYTES + 1];
  char *end = NULL;
  memcpy(number, slot, TEXT_SCALAR_BYTES);
  number[TEXT_SCALAR_BYTES] = '\0';
  errno = 0;
  *value = strtof(number, &end);
  return errno == 0 && end == number + TEXT_SCALAR_BYTES && isfinite(*value);
}

static float read_float32_le(const unsigned char *bytes) {
  uint32_t bits = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u) |
                  ((uint32_t)bytes[2] << 16u) |
                  ((uint32_t)bytes[3] << 24u);
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static int normalize(float *values, size_t dimensions) {
  float squared_norm = 0.0f;
  for (size_t i = 0; i < dimensions; ++i) {
    squared_norm += values[i] * values[i];
  }
  if (!(squared_norm > 0.0f) || !isfinite(squared_norm)) {
    return 0;
  }
  float scale = 1.0f / sqrtf(squared_norm);
  for (size_t i = 0; i < dimensions; ++i) {
    values[i] *= scale;
  }
  return 1;
}

static int parse_vector(char *text, size_t dimensions, float *values) {
  char *cursor = text;
  for (size_t i = 0; i < dimensions; ++i) {
    char *end = NULL;
    while (*cursor == ' ' || *cursor == '\t') {
      ++cursor;
    }
    errno = 0;
    values[i] = strtof(cursor, &end);
    if (errno != 0 || end == cursor || !isfinite(values[i])) {
      return 0;
    }
    cursor = end;
  }
  while (*cursor == ' ' || *cursor == '\t') {
    ++cursor;
  }
  return *cursor == '\0';
}

static void trim_line_end(char *line) {
  size_t length = strlen(line);
  while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
    line[--length] = '\0';
  }
}

static int write_manifest_file(const char *path, const struct manifest *manifest) {
  FILE *file = fopen(path, "wb");
  if (file == NULL) {
    return fail_path("cannot open", path);
  }
  int bad = fprintf(file,
                    FORMAT_HEADER "\n"
                    "backend " BACKEND_NAME "\n"
                    "scalar f32\n"
                    "byte_order little\n"
                    "metric %s\n"
                    "dimensions %zu\n"
                    "count %zu\n"
                    "ids %s\n"
                    "vectors_text %s\n"
                    "text_scalar scientific-e8\n"
                    "text_slot_bytes %d\n"
                    "vectors_cache %s\n",
                    metric_text(manifest->metric), manifest->dimensions,
                    manifest->count, manifest->ids_file,
                    manifest->vectors_text_file, TEXT_SLOT_BYTES,
                    manifest->vectors_cache_file) < 0;
  if (!bad && flush_file(file, path) != 0) {
    bad = 1;
  }
  if (fclose(file) != 0) {
    bad = 1;
  }
  return bad ? fail("cannot write index manifest") : 0;
}

static int manifest_field(char *line, const char *name, char **value) {
  size_t length = strlen(name);
  if (strncmp(line, name, length) != 0 || line[length] != ' ') {
    return 0;
  }
  *value = line + length + 1;
  return **value != '\0';
}

static int read_manifest(const char *directory, struct manifest *manifest) {
  char path[PATH_BUFFER];
  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  int line_number = 0;
  int seen_backend = 0, seen_scalar = 0, seen_order = 0;
  int seen_metric = 0, seen_dimensions = 0, seen_count = 0;
  int seen_ids = 0, seen_vectors_text = 0, seen_text_scalar = 0;
  int seen_text_slot_bytes = 0, seen_vectors_cache = 0;

  memset(manifest, 0, sizeof(*manifest));
  if (!path_join(path, sizeof(path), directory, "format.txt")) {
    return 1;
  }
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    return fail_path("cannot open", path);
  }

  while ((length = getline(&line, &capacity, file)) >= 0) {
    char *value = NULL;
    (void)length;
    ++line_number;
    trim_line_end(line);
    if (line_number == 1) {
      if (strcmp(line, FORMAT_HEADER) != 0) {
        goto invalid;
      }
    } else if (manifest_field(line, "backend", &value)) {
      if (seen_backend) {
        goto invalid;
      }
      seen_backend = strcmp(value, BACKEND_NAME) == 0;
    } else if (manifest_field(line, "scalar", &value)) {
      if (seen_scalar) {
        goto invalid;
      }
      seen_scalar = strcmp(value, "f32") == 0;
    } else if (manifest_field(line, "byte_order", &value)) {
      if (seen_order) {
        goto invalid;
      }
      seen_order = strcmp(value, "little") == 0;
    } else if (manifest_field(line, "metric", &value)) {
      if (seen_metric) {
        goto invalid;
      }
      seen_metric = parse_metric(value, &manifest->metric);
    } else if (manifest_field(line, "dimensions", &value)) {
      if (seen_dimensions) {
        goto invalid;
      }
      seen_dimensions = parse_size(value, &manifest->dimensions);
    } else if (manifest_field(line, "count", &value)) {
      if (seen_count) {
        goto invalid;
      }
      char *end = NULL;
      unsigned long long parsed;
      errno = 0;
      parsed = strtoull(value, &end, 10);
      seen_count = errno == 0 && end != value && *end == '\0' &&
                   parsed <= SIZE_MAX;
      manifest->count = (size_t)parsed;
    } else if (manifest_field(line, "ids", &value)) {
      if (seen_ids) {
        goto invalid;
      }
      seen_ids = plain_filename(value) &&
                 snprintf(manifest->ids_file, sizeof(manifest->ids_file), "%s",
                          value) < (int)sizeof(manifest->ids_file);
    } else if (manifest_field(line, "vectors_text", &value)) {
      if (seen_vectors_text) {
        goto invalid;
      }
      seen_vectors_text =
          plain_filename(value) &&
          snprintf(manifest->vectors_text_file,
                   sizeof(manifest->vectors_text_file), "%s", value) <
              (int)sizeof(manifest->vectors_text_file);
    } else if (manifest_field(line, "text_scalar", &value)) {
      if (seen_text_scalar) {
        goto invalid;
      }
      seen_text_scalar = strcmp(value, "scientific-e8") == 0;
    } else if (manifest_field(line, "text_slot_bytes", &value)) {
      if (seen_text_slot_bytes) {
        goto invalid;
      }
      size_t parsed = 0;
      seen_text_slot_bytes = parse_size(value, &parsed) &&
                             parsed == TEXT_SLOT_BYTES;
    } else if (manifest_field(line, "vectors_cache", &value)) {
      if (seen_vectors_cache) {
        goto invalid;
      }
      seen_vectors_cache =
          plain_filename(value) &&
          snprintf(manifest->vectors_cache_file,
                   sizeof(manifest->vectors_cache_file), "%s", value) <
              (int)sizeof(manifest->vectors_cache_file);
    } else {
      goto invalid;
    }
  }

  free(line);
  if (ferror(file)) {
    fclose(file);
    return fail_path("cannot read", path);
  }
  fclose(file);
  if (line_number != 12 || !seen_backend || !seen_scalar || !seen_order ||
      !seen_metric || !seen_dimensions || !seen_count || !seen_ids ||
      !seen_vectors_text || !seen_text_scalar || !seen_text_slot_bytes ||
      !seen_vectors_cache) {
    return fail("index manifest is incomplete or unsupported");
  }
  if (strcmp(manifest->ids_file, manifest->vectors_text_file) == 0 ||
      strcmp(manifest->ids_file, manifest->vectors_cache_file) == 0 ||
      strcmp(manifest->vectors_text_file, manifest->vectors_cache_file) == 0) {
    return fail("index manifest data filenames must be distinct");
  }
  return 0;

invalid:
  free(line);
  fclose(file);
  return fail("index manifest has an unsupported format");
}

static int checked_value_count(const struct manifest *manifest, size_t *values) {
  if (manifest->count != 0 && manifest->dimensions > SIZE_MAX / manifest->count) {
    return 0;
  }
  *values = manifest->count * manifest->dimensions;
  return 1;
}

static int checked_vector_bytes(const struct manifest *manifest, size_t *bytes) {
  size_t values;
  if (!checked_value_count(manifest, &values) ||
      values > SIZE_MAX / sizeof(float)) {
    return 0;
  }
  *bytes = values * sizeof(float);
  return 1;
}

static int checked_text_bytes(const struct manifest *manifest, size_t *bytes) {
  size_t values;
  if (!checked_value_count(manifest, &values) ||
      values > SIZE_MAX / TEXT_SLOT_BYTES) {
    return 0;
  }
  *bytes = values * TEXT_SLOT_BYTES;
  return 1;
}

static int validate_text_vectors(const char *path,
                                 const struct manifest *manifest,
                                 size_t expected_bytes) {
  int descriptor = -1;
  const unsigned char *mapped = MAP_FAILED;
  int status = 1;

  if (expected_bytes == 0) {
    return 0;
  }
  descriptor = open(path, O_RDONLY);
  if (descriptor < 0) {
    return fail_path("cannot open", path);
  }
  mapped = mmap(NULL, expected_bytes, PROT_READ, MAP_PRIVATE, descriptor, 0);
  if (mapped == MAP_FAILED) {
    fail_path("cannot map", path);
    goto cleanup;
  }
  size_t values = expected_bytes / TEXT_SLOT_BYTES;
  for (size_t index = 0; index < values; ++index) {
    const unsigned char *slot = mapped + index * TEXT_SLOT_BYTES;
    float value;
    unsigned char expected_delimiter =
        (index + 1) % manifest->dimensions == 0 ? '\n' : ' ';
    if (slot[TEXT_SCALAR_BYTES] != expected_delimiter ||
        !read_float32_text(slot, &value)) {
      fail("plain-text vector file has an invalid fixed-width slot");
      goto cleanup;
    }
  }
  status = 0;

cleanup:
  if (mapped != MAP_FAILED) {
    munmap((void *)mapped, expected_bytes);
  }
  close(descriptor);
  return status;
}

static int validate_cache_vectors(const char *text_path, const char *cache_path,
                                  const struct manifest *manifest,
                                  size_t text_bytes, size_t cache_bytes) {
  int text_descriptor = -1, cache_descriptor = -1;
  const unsigned char *text = MAP_FAILED, *cache = MAP_FAILED;
  int status = 1;

  if (text_bytes == 0) {
    return 0;
  }
  text_descriptor = open(text_path, O_RDONLY);
  if (text_descriptor < 0) {
    return fail_path("cannot open", text_path);
  }
  cache_descriptor = open(cache_path, O_RDONLY);
  if (cache_descriptor < 0) {
    fail_path("cannot open", cache_path);
    goto cleanup;
  }
  text = mmap(NULL, text_bytes, PROT_READ, MAP_PRIVATE, text_descriptor, 0);
  if (text == MAP_FAILED) {
    fail_path("cannot map", text_path);
    goto cleanup;
  }
  cache = mmap(NULL, cache_bytes, PROT_READ, MAP_PRIVATE, cache_descriptor, 0);
  if (cache == MAP_FAILED) {
    fail_path("cannot map", cache_path);
    goto cleanup;
  }

  size_t values = text_bytes / TEXT_SLOT_BYTES;
  for (size_t index = 0; index < values; ++index) {
    const unsigned char *slot = text + index * TEXT_SLOT_BYTES;
    float text_value, cache_value;
    uint32_t text_bits, cache_bits;
    unsigned char expected_delimiter =
        (index + 1) % manifest->dimensions == 0 ? '\n' : ' ';
    if (slot[TEXT_SCALAR_BYTES] != expected_delimiter ||
        !read_float32_text(slot, &text_value)) {
      fail("plain-text vector file has an invalid fixed-width slot");
      goto cleanup;
    }
    cache_value = read_float32_le(cache + index * sizeof(float));
    memcpy(&text_bits, &text_value, sizeof(text_bits));
    memcpy(&cache_bits, &cache_value, sizeof(cache_bits));
    if (text_bits != cache_bits) {
      fail("Float32 cache does not match the plain-text vectors");
      goto cleanup;
    }
  }
  status = 0;

cleanup:
  if (cache != MAP_FAILED) {
    munmap((void *)cache, cache_bytes);
  }
  if (text != MAP_FAILED) {
    munmap((void *)text, text_bytes);
  }
  if (cache_descriptor >= 0) {
    close(cache_descriptor);
  }
  if (text_descriptor >= 0) {
    close(text_descriptor);
  }
  return status;
}

static int check_index(const char *directory, struct manifest *manifest,
                       int announce, int require_cache, int validate_values) {
  char ids_path[PATH_BUFFER], text_path[PATH_BUFFER], cache_path[PATH_BUFFER];
  struct stat text_stat, cache_stat;
  size_t expected_text_bytes, expected_cache_bytes;
  size_t ids = 0;
  char *line = NULL;
  size_t capacity = 0;

  if (read_manifest(directory, manifest) != 0) {
    return 1;
  }
  if (!path_join(ids_path, sizeof(ids_path), directory, manifest->ids_file) ||
      !path_join(text_path, sizeof(text_path), directory,
                 manifest->vectors_text_file) ||
      !path_join(cache_path, sizeof(cache_path), directory,
                 manifest->vectors_cache_file)) {
    return 1;
  }
  if (!checked_text_bytes(manifest, &expected_text_bytes) ||
      !checked_vector_bytes(manifest, &expected_cache_bytes)) {
    return fail("index dimensions overflow file size");
  }
  if (stat(text_path, &text_stat) != 0) {
    return fail_path("cannot inspect", text_path);
  }
  if (text_stat.st_size < 0 ||
      (uintmax_t)text_stat.st_size != expected_text_bytes) {
    return fail("plain-text vector size does not match the manifest");
  }
  if (require_cache) {
    if (stat(cache_path, &cache_stat) != 0) {
      return fail_path("cannot inspect", cache_path);
    }
    if (cache_stat.st_size < 0 ||
        (uintmax_t)cache_stat.st_size != expected_cache_bytes) {
      return fail("Float32 cache size does not match the manifest");
    }
  }
  if (validate_values) {
    int invalid = require_cache
                      ? validate_cache_vectors(text_path, cache_path, manifest,
                                               expected_text_bytes,
                                               expected_cache_bytes)
                      : validate_text_vectors(text_path, manifest,
                                              expected_text_bytes);
    if (invalid) {
      return 1;
    }
  }

  FILE *ids_file = fopen(ids_path, "rb");
  if (ids_file == NULL) {
    return fail_path("cannot open", ids_path);
  }
  while (getline(&line, &capacity, ids_file) >= 0) {
    trim_line_end(line);
    if (line[0] == '\0' || strchr(line, '\t') != NULL) {
      free(line);
      fclose(ids_file);
      return fail("ID file contains an empty or tabbed ID");
    }
    ++ids;
  }
  free(line);
  if (ferror(ids_file)) {
    fclose(ids_file);
    return fail_path("cannot read", ids_path);
  }
  fclose(ids_file);
  if (ids != manifest->count) {
    return fail("ID count does not match the manifest");
  }

  if (announce) {
    printf("check=ok\nbackend=%s\nscalar=f32\ntext_slot_bytes=%d\nmetric=%s\ndimensions=%zu\ncount=%zu\n",
           BACKEND_NAME, TEXT_SLOT_BYTES, metric_text(manifest->metric),
           manifest->dimensions, manifest->count);
  }
  return 0;
}

static int build_index(const char *directory, const char *dimension_text,
                       const char *metric_name) {
  struct manifest manifest;
  char generation[96];
  char ids_path[PATH_BUFFER], text_path[PATH_BUFFER], cache_path[PATH_BUFFER];
  char manifest_path[PATH_BUFFER], manifest_temp[PATH_BUFFER];
  char *line = NULL;
  size_t line_capacity = 0;
  ssize_t line_length;
  float *values = NULL;
  FILE *ids_file = NULL, *text_file = NULL, *cache_file = NULL;
  int status = 1;

  memset(&manifest, 0, sizeof(manifest));
  if (!parse_size(dimension_text, &manifest.dimensions)) {
    return fail("dimensions must be a positive integer");
  }
  if (manifest.dimensions > SIZE_MAX / sizeof(*values)) {
    return fail("dimensions are too large for this process");
  }
  if (!parse_metric(metric_name, &manifest.metric)) {
    return fail("metric must be cosine or dot");
  }
  if (make_directories(directory) != 0) {
    return 1;
  }

  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
    return fail_path("cannot read clock for", directory);
  }
  snprintf(generation, sizeof(generation), "%lld-%09ld-%ld",
           (long long)now.tv_sec, now.tv_nsec, (long)getpid());
  snprintf(manifest.ids_file, sizeof(manifest.ids_file), "ids-%s.txt",
           generation);
  snprintf(manifest.vectors_text_file, sizeof(manifest.vectors_text_file),
           "vectors-%s.txt", generation);
  snprintf(manifest.vectors_cache_file, sizeof(manifest.vectors_cache_file),
           "vectors-%s.f32", generation);

  if (!path_join(ids_path, sizeof(ids_path), directory, manifest.ids_file) ||
      !path_join(text_path, sizeof(text_path), directory,
                 manifest.vectors_text_file) ||
      !path_join(cache_path, sizeof(cache_path), directory,
                 manifest.vectors_cache_file) ||
      !path_join(manifest_path, sizeof(manifest_path), directory, "format.txt") ||
      snprintf(manifest_temp, sizeof(manifest_temp), "%s.tmp.%ld", manifest_path,
               (long)getpid()) >= (int)sizeof(manifest_temp)) {
    return 1;
  }

  ids_file = create_exclusive(ids_path);
  if (ids_file == NULL) {
    return fail_path("cannot create", ids_path);
  }
  text_file = create_exclusive(text_path);
  if (text_file == NULL) {
    fail_path("cannot create", text_path);
    goto cleanup;
  }
  cache_file = create_exclusive(cache_path);
  if (cache_file == NULL) {
    fail_path("cannot create", cache_path);
    goto cleanup;
  }
  values = malloc(manifest.dimensions * sizeof(*values));
  if (values == NULL) {
    fail("out of memory while reading vectors");
    goto cleanup;
  }

  while ((line_length = getline(&line, &line_capacity, stdin)) >= 0) {
    char *tab;
    (void)line_length;
    trim_line_end(line);
    if (line[0] == '\0') {
      continue;
    }
    tab = strchr(line, '\t');
    if (tab == NULL || tab == line) {
      fail("each input row must be ID, tab, then vector values");
      goto cleanup;
    }
    *tab = '\0';
    if (strchr(tab + 1, '\n') != NULL ||
        !parse_vector(tab + 1, manifest.dimensions, values)) {
      fail("an input row has the wrong vector dimension or a non-finite value");
      goto cleanup;
    }
    if (manifest.metric == METRIC_COSINE &&
        !normalize(values, manifest.dimensions)) {
      fail("cosine vectors must have a finite, nonzero norm");
      goto cleanup;
    }
    if (fprintf(ids_file, "%s\n", line) < 0) {
      fail_path("cannot write", ids_path);
      goto cleanup;
    }
    for (size_t i = 0; i < manifest.dimensions; ++i) {
      if (!write_float32_text(text_file, values[i],
                              i + 1 == manifest.dimensions)) {
        fail_path("cannot write", text_path);
        goto cleanup;
      }
      if (!write_float32_le(cache_file, values[i])) {
        fail_path("cannot write", cache_path);
        goto cleanup;
      }
    }
    if (manifest.count == SIZE_MAX) {
      fail("too many vector rows");
      goto cleanup;
    }
    ++manifest.count;
  }
  if (ferror(stdin)) {
    fail("cannot read vector rows from standard input");
    goto cleanup;
  }
  if (flush_file(ids_file, ids_path) != 0 ||
      flush_file(text_file, text_path) != 0 ||
      flush_file(cache_file, cache_path) != 0) {
    goto cleanup;
  }
  int ids_close_failed = fclose(ids_file) != 0;
  ids_file = NULL;
  int text_close_failed = fclose(text_file) != 0;
  text_file = NULL;
  int cache_close_failed = fclose(cache_file) != 0;
  cache_file = NULL;
  if (ids_close_failed || text_close_failed || cache_close_failed) {
    fail("cannot close new index files");
    goto cleanup;
  }

  if (write_manifest_file(manifest_temp, &manifest) != 0) {
    goto cleanup;
  }
  if (rename(manifest_temp, manifest_path) != 0) {
    fail_path("cannot install", manifest_path);
    goto cleanup;
  }

  printf("build=ok\nbackend=%s\nscalar=f32\nmetric=%s\ndimensions=%zu\ncount=%zu\n",
         BACKEND_NAME, metric_text(manifest.metric), manifest.dimensions,
         manifest.count);
  status = 0;

cleanup:
  free(values);
  free(line);
  if (ids_file != NULL) {
    fclose(ids_file);
  }
  if (text_file != NULL) {
    fclose(text_file);
  }
  if (cache_file != NULL) {
    fclose(cache_file);
  }
  if (status != 0) {
    unlink(ids_path);
    unlink(text_path);
    unlink(cache_path);
    unlink(manifest_temp);
  }
  return status;
}

static float dot_product(const unsigned char *stored, const float *query,
                         size_t dimensions) {
  float score = 0.0f;
  if (host_is_little_endian()) {
    for (size_t i = 0; i < dimensions; ++i) {
      float value;
      memcpy(&value, stored + i * sizeof(float), sizeof(value));
      score += value * query[i];
    }
  } else {
    for (size_t i = 0; i < dimensions; ++i) {
      score += read_float32_le(stored + i * sizeof(float)) * query[i];
    }
  }
  return score;
}

static int dot_product_text(const unsigned char *stored, const float *query,
                            size_t dimensions, float *score) {
  *score = 0.0f;
  for (size_t i = 0; i < dimensions; ++i) {
    float value;
    const unsigned char *slot = stored + i * TEXT_SLOT_BYTES;
    unsigned char expected_delimiter = i + 1 == dimensions ? '\n' : ' ';
    if (slot[TEXT_SCALAR_BYTES] != expected_delimiter ||
        !read_float32_text(slot, &value)) {
      return 0;
    }
    *score += value * query[i];
  }
  return 1;
}

static int write_query_report(const char *path, const struct manifest *manifest,
                              int use_text, size_t dot_products) {
  if (path == NULL || path[0] == '\0') {
    return 0;
  }
  FILE *file = fopen(path, "wb");
  if (file == NULL) {
    return fail_path("cannot open query report", path);
  }
  int bad = fprintf(file,
                    "operation=exact-dot-product-scan\n"
                    "compute=cpu\n"
                    "dot-product-used-gpu=False\n"
                    "storage=%s\n"
                    "metric=%s\n"
                    "dimensions=%zu\n"
                    "dot-products=%zu\n",
                    use_text ? "readable-text" : "float32-cache",
                    metric_text(manifest->metric), manifest->dimensions,
                    dot_products) < 0;
  if (!bad && flush_file(file, path) != 0) {
    bad = 1;
  }
  if (fclose(file) != 0) {
    bad = 1;
  }
  return bad ? fail("cannot write query report") : 0;
}

static int match_before(float score, size_t row, const struct match *other) {
  return score > other->score || (score == other->score && row < other->row);
}

static void consider_match(struct match *matches, size_t *used, size_t limit,
                           float score, size_t row, const char *id) {
  size_t position = 0;
  while (position < *used && !match_before(score, row, &matches[position])) {
    ++position;
  }
  if (position >= limit) {
    return;
  }
  size_t new_used = *used < limit ? *used + 1 : *used;
  if (*used == limit) {
    free(matches[limit - 1].id);
  }
  for (size_t i = new_used - 1; i > position; --i) {
    matches[i] = matches[i - 1];
  }
  matches[position].score = score;
  matches[position].row = row;
  matches[position].id = strdup(id);
  if (matches[position].id == NULL) {
    fail("out of memory while retaining matches");
    exit(1);
  }
  *used = new_used;
}

static int query_index(const char *directory, const char *limit_text,
                       int use_text) {
  struct manifest manifest;
  char ids_path[PATH_BUFFER], data_path[PATH_BUFFER];
  char *line = NULL;
  size_t line_capacity = 0, limit, used = 0, data_bytes = 0;
  float *query = NULL;
  struct match *matches = NULL;
  FILE *ids_file = NULL;
  int data_fd = -1;
  unsigned char *mapped = MAP_FAILED;
  int status = 1;
  const char *query_report_path = getenv("IB_VECTOR_QUERY_REPORT");

  if (!parse_size(limit_text, &limit)) {
    return fail("result count must be a positive integer");
  }
  if (check_index(directory, &manifest, 0, !use_text, 0) != 0) {
    return 1;
  }
  if (limit > manifest.count) {
    limit = manifest.count;
  }
  query = malloc(manifest.dimensions * sizeof(*query));
  if (query == NULL) {
    return fail("out of memory while reading query");
  }
  if (getline(&line, &line_capacity, stdin) < 0) {
    fail("query vector is missing on standard input");
    goto cleanup;
  }
  trim_line_end(line);
  if (!parse_vector(line, manifest.dimensions, query)) {
    fail("query has the wrong vector dimension or a non-finite value");
    goto cleanup;
  }
  if (manifest.metric == METRIC_COSINE &&
      !normalize(query, manifest.dimensions)) {
    fail("cosine query must have a finite, nonzero norm");
    goto cleanup;
  }
  while (getline(&line, &line_capacity, stdin) >= 0) {
    trim_line_end(line);
    if (line[0] != '\0') {
      fail("query accepts exactly one vector");
      goto cleanup;
    }
  }

  if (limit == 0) {
    status = write_query_report(query_report_path, &manifest, use_text, 0);
    goto cleanup;
  }
  matches = calloc(limit, sizeof(*matches));
  if (matches == NULL) {
    fail("out of memory while allocating matches");
    goto cleanup;
  }
  int valid_size = use_text ? checked_text_bytes(&manifest, &data_bytes)
                            : checked_vector_bytes(&manifest, &data_bytes);
  const char *data_file = use_text ? manifest.vectors_text_file
                                   : manifest.vectors_cache_file;
  if (!valid_size ||
      !path_join(ids_path, sizeof(ids_path), directory, manifest.ids_file) ||
      !path_join(data_path, sizeof(data_path), directory, data_file)) {
    goto cleanup;
  }
  ids_file = fopen(ids_path, "rb");
  if (ids_file == NULL) {
    fail_path("cannot open", ids_path);
    goto cleanup;
  }
  data_fd = open(data_path, O_RDONLY);
  if (data_fd < 0) {
    fail_path("cannot open", data_path);
    goto cleanup;
  }
  mapped = mmap(NULL, data_bytes, PROT_READ, MAP_PRIVATE, data_fd, 0);
  if (mapped == MAP_FAILED) {
    fail_path("cannot map", data_path);
    goto cleanup;
  }

  for (size_t row = 0; row < manifest.count; ++row) {
    if (getline(&line, &line_capacity, ids_file) < 0) {
      fail("ID file ended during query");
      goto cleanup;
    }
    trim_line_end(line);
    size_t scalar_bytes = use_text ? TEXT_SLOT_BYTES : sizeof(float);
    const unsigned char *stored =
        mapped + row * manifest.dimensions * scalar_bytes;
    float score;
    if (use_text) {
      if (!dot_product_text(stored, query, manifest.dimensions, &score)) {
        fail("plain-text vector file changed during query");
        goto cleanup;
      }
    } else {
      score = dot_product(stored, query, manifest.dimensions);
    }
    if (!isfinite(score)) {
      fail("similarity score overflowed or the cache is corrupt");
      goto cleanup;
    }
    if (manifest.metric == METRIC_COSINE) {
      if (score > 1.0f) {
        score = 1.0f;
      } else if (score < -1.0f) {
        score = -1.0f;
      }
    }
    consider_match(matches, &used, limit, score, row, line);
  }
  for (size_t i = 0; i < used; ++i) {
    printf("%s\t%.9g\n", matches[i].id, (double)matches[i].score);
  }
  status = write_query_report(query_report_path, &manifest, use_text,
                              manifest.count);

cleanup:
  if (mapped != MAP_FAILED) {
    munmap(mapped, data_bytes);
  }
  if (data_fd >= 0) {
    close(data_fd);
  }
  if (ids_file != NULL) {
    fclose(ids_file);
  }
  if (matches != NULL) {
    for (size_t i = 0; i < used; ++i) {
      free(matches[i].id);
    }
  }
  free(matches);
  free(query);
  free(line);
  return status;
}

static int compile_cache(const char *directory) {
  struct manifest manifest;
  char text_path[PATH_BUFFER], cache_path[PATH_BUFFER], temporary_path[PATH_BUFFER];
  size_t text_bytes = 0, cache_bytes = 0;
  int text_fd = -1;
  const unsigned char *mapped = MAP_FAILED;
  FILE *cache_file = NULL;
  int status = 1;

  if (check_index(directory, &manifest, 0, 0, 1) != 0 ||
      !checked_text_bytes(&manifest, &text_bytes) ||
      !checked_vector_bytes(&manifest, &cache_bytes) ||
      !path_join(text_path, sizeof(text_path), directory,
                 manifest.vectors_text_file) ||
      !path_join(cache_path, sizeof(cache_path), directory,
                 manifest.vectors_cache_file) ||
      snprintf(temporary_path, sizeof(temporary_path), "%s.tmp.%ld", cache_path,
               (long)getpid()) >= (int)sizeof(temporary_path)) {
    return 1;
  }
  cache_file = create_exclusive(temporary_path);
  if (cache_file == NULL) {
    return fail_path("cannot create", temporary_path);
  }
  if (text_bytes != 0) {
    text_fd = open(text_path, O_RDONLY);
    if (text_fd < 0) {
      fail_path("cannot open", text_path);
      goto cleanup;
    }
    mapped = mmap(NULL, text_bytes, PROT_READ, MAP_PRIVATE, text_fd, 0);
    if (mapped == MAP_FAILED) {
      fail_path("cannot map", text_path);
      goto cleanup;
    }
  }

  size_t values = cache_bytes / sizeof(float);
  for (size_t index = 0; index < values; ++index) {
    float value;
    if (!read_float32_text(mapped + index * TEXT_SLOT_BYTES, &value) ||
        !write_float32_le(cache_file, value)) {
      fail("cannot compile Float32 cache from plain-text vectors");
      goto cleanup;
    }
  }
  if (flush_file(cache_file, temporary_path) != 0) {
    goto cleanup;
  }
  if (fclose(cache_file) != 0) {
    cache_file = NULL;
    fail_path("cannot close", temporary_path);
    goto cleanup;
  }
  cache_file = NULL;
  if (rename(temporary_path, cache_path) != 0) {
    fail_path("cannot install", cache_path);
    goto cleanup;
  }
  printf("compile-cache=ok\nbytes=%zu\n", cache_bytes);
  status = 0;

cleanup:
  if (mapped != MAP_FAILED) {
    munmap((void *)mapped, text_bytes);
  }
  if (text_fd >= 0) {
    close(text_fd);
  }
  if (cache_file != NULL) {
    fclose(cache_file);
  }
  if (status != 0) {
    unlink(temporary_path);
  }
  return status;
}

static int print_column(const char *directory, const char *coordinate_text) {
  struct manifest manifest;
  char ids_path[PATH_BUFFER], text_path[PATH_BUFFER];
  char *line = NULL;
  size_t line_capacity = 0, coordinate, text_bytes = 0;
  FILE *ids_file = NULL;
  int text_fd = -1;
  const unsigned char *mapped = MAP_FAILED;
  int status = 1;

  if (!parse_size(coordinate_text, &coordinate)) {
    return fail("coordinate must be a positive one-based integer");
  }
  if (check_index(directory, &manifest, 0, 0, 0) != 0) {
    return 1;
  }
  if (coordinate > manifest.dimensions) {
    return fail("coordinate exceeds the vector dimensions");
  }
  if (!checked_text_bytes(&manifest, &text_bytes) ||
      !path_join(ids_path, sizeof(ids_path), directory, manifest.ids_file) ||
      !path_join(text_path, sizeof(text_path), directory,
                 manifest.vectors_text_file)) {
    return 1;
  }
  ids_file = fopen(ids_path, "rb");
  if (ids_file == NULL) {
    return fail_path("cannot open", ids_path);
  }
  if (text_bytes != 0) {
    text_fd = open(text_path, O_RDONLY);
    if (text_fd < 0) {
      fail_path("cannot open", text_path);
      goto cleanup;
    }
    mapped = mmap(NULL, text_bytes, PROT_READ, MAP_PRIVATE, text_fd, 0);
    if (mapped == MAP_FAILED) {
      fail_path("cannot map", text_path);
      goto cleanup;
    }
  }

  size_t coordinate_index = coordinate - 1;
  for (size_t row = 0; row < manifest.count; ++row) {
    if (getline(&line, &line_capacity, ids_file) < 0) {
      fail("ID file ended during column scan");
      goto cleanup;
    }
    trim_line_end(line);
    size_t slot_index = row * manifest.dimensions + coordinate_index;
    const unsigned char *slot = mapped + slot_index * TEXT_SLOT_BYTES;
    float value;
    unsigned char expected_delimiter =
        coordinate == manifest.dimensions ? '\n' : ' ';
    if (slot[TEXT_SCALAR_BYTES] != expected_delimiter ||
        !read_float32_text(slot, &value)) {
      fail("plain-text vector file has an invalid selected coordinate");
      goto cleanup;
    }
    printf("%s\t%.*s\n", line, TEXT_SCALAR_BYTES, (const char *)slot);
  }
  status = 0;

cleanup:
  if (mapped != MAP_FAILED) {
    munmap((void *)mapped, text_bytes);
  }
  if (text_fd >= 0) {
    close(text_fd);
  }
  if (ids_file != NULL) {
    fclose(ids_file);
  }
  free(line);
  return status;
}

static int inspect_index(const char *directory) {
  struct manifest manifest;
  if (check_index(directory, &manifest, 0, 0, 0) != 0) {
    return 1;
  }
  printf(FORMAT_HEADER "\n"
         "backend " BACKEND_NAME "\n"
         "scalar f32\n"
         "byte_order little\n"
         "metric %s\n"
         "dimensions %zu\n"
         "count %zu\n"
         "ids %s\n"
         "vectors_text %s\n"
         "text_scalar scientific-e8\n"
         "text_slot_bytes %d\n"
         "vectors_cache %s\n",
         metric_text(manifest.metric), manifest.dimensions, manifest.count,
         manifest.ids_file, manifest.vectors_text_file, TEXT_SLOT_BYTES,
         manifest.vectors_cache_file);
  return 0;
}

static void usage(FILE *out) {
  fprintf(out,
          "usage:\n"
          "  ib-vector-index build INDEX_DIRECTORY DIMENSIONS cosine|dot < rows.tsv\n"
          "  ib-vector-index query INDEX_DIRECTORY RESULT_COUNT < vector.txt\n"
          "  ib-vector-index query-text INDEX_DIRECTORY RESULT_COUNT < vector.txt\n"
          "  ib-vector-index column INDEX_DIRECTORY ONE_BASED_COORDINATE\n"
          "  ib-vector-index compile-cache INDEX_DIRECTORY\n"
          "  ib-vector-index check INDEX_DIRECTORY\n"
          "  ib-vector-index inspect INDEX_DIRECTORY\n\n"
          "build rows are: ID<TAB>number number ...\n"
          "query results are: ID<TAB>score\n");
}

int main(int argc, char **argv) {
  if (sizeof(float) != 4 || FLT_RADIX != 2 || FLT_MANT_DIG != 24 ||
      FLT_MAX_EXP != 128) {
    return fail("this backend requires IEEE 754 binary32 float");
  }
  if (argc == 6 && strcmp(argv[1], "build") == 0) {
    return fail("build received too many arguments");
  }
  if (argc == 5 && strcmp(argv[1], "build") == 0) {
    return build_index(argv[2], argv[3], argv[4]);
  }
  if (argc == 4 && strcmp(argv[1], "query") == 0) {
    return query_index(argv[2], argv[3], 0);
  }
  if (argc == 4 && strcmp(argv[1], "query-text") == 0) {
    return query_index(argv[2], argv[3], 1);
  }
  if (argc == 4 && strcmp(argv[1], "column") == 0) {
    return print_column(argv[2], argv[3]);
  }
  if (argc == 3 && strcmp(argv[1], "compile-cache") == 0) {
    return compile_cache(argv[2]);
  }
  if (argc == 3 && strcmp(argv[1], "check") == 0) {
    struct manifest manifest;
    return check_index(argv[2], &manifest, 1, 1, 1);
  }
  if (argc == 3 && strcmp(argv[1], "inspect") == 0) {
    return inspect_index(argv[2]);
  }
  usage(stderr);
  return 2;
}
