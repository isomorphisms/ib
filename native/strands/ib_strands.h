#ifndef IB_STRANDS_H
#define IB_STRANDS_H
#include <stddef.h>
#include <stdint.h>

/* Native byte projection of an admitted IB.FragmentStore graph. The canonical
 * source bytes and source/representation identities remain owned upstream.
 * SourceFragment's Unicode code-point ranges must be mapped to explicit UTF-8
 * byte boundaries by the admitting adapter; never reinterpret them here. */
#define IB_MAX_WINDOW 4096u
#define IB_ID_SIZE 65u
#define IB_KIND_SIZE 33u

typedef struct {
    char id[IB_ID_SIZE];
    char expected_sha256[65];
    int fd;
    uint64_t bytes;
    uint64_t stamp_size, stamp_mtime_ns, stamp_ctime_ns;
    uint64_t stamp_dev, stamp_ino;
} ib_source;

typedef struct {
    char id[IB_ID_SIZE];
    uint32_t source; /* index into the admitted ib_source array */
    uint64_t start, end; /* half-open original-byte range; empty is permitted */
} ib_fragment;

typedef struct {
    uint32_t fragment; /* index into fragments, never a persistent identity */
} ib_visit;

typedef struct {
    uint32_t from, to; /* indexes into fragments */
    char kind[IB_KIND_SIZE];
} ib_link;

typedef struct {
    ib_source *sources;
    size_t source_count;
    ib_fragment *fragments;
    size_t fragment_count;
    ib_visit *visits;
    size_t visit_count;
    ib_link *links;
    size_t link_count;
    uint64_t *prefix; /* visit_count+1; cumulative assembled bytes */
    uint32_t *links_out; /* link indexes sorted by from, kind, to */
    uint32_t *links_in;  /* link indexes sorted by to, kind, from */
} ib_strand;

/* Exact SHA-256 and fstat admission. The caller must own immutable, private
 * snapshot files during the lifetime of this descriptor. */
int ib_source_open(ib_source *out, const char *path, const char *id,
                   const char *expected_digest, char *error, size_t error_length);
void ib_source_close(ib_source *source);
int ib_source_window(ib_source *source, uint64_t start, unsigned char *out,
                     size_t capacity, size_t *received);

/* Owns only index allocations, not arrays supplied by caller. */
int ib_strand_index(ib_strand *strand, char *error, size_t error_length);
void ib_strand_release(ib_strand *strand);
uint64_t ib_strand_bytes(const ib_strand *strand);
int ib_strand_window(ib_strand *strand, uint64_t offset, unsigned char *out,
                     size_t capacity, size_t *received);

/* Stable-id lookups; these are for cold debugging and import validation. Hot
 * window traversal uses prefix binary search and direct visit ordinals. */
int ib_fragment_index(const ib_strand *strand, const char *id, uint32_t *result);
size_t ib_links_from(const ib_strand *strand, uint32_t fragment, const char *kind,
                     uint32_t *out, size_t capacity);
size_t ib_links_to(const ib_strand *strand, uint32_t fragment, const char *kind,
                   uint32_t *out, size_t capacity);

/* Rebuildable binary index: writes a temporary file then renameat2 NOREPLACE;
 * loader verifies source identity, original canonical metadata fingerprint,
 * record dimensions, monotonic prefixes, and exact link projections. */
int ib_index_write(const ib_strand *strand, const char *path,
                   const char *canonical_digest, char *error, size_t error_length);
int ib_index_check(const ib_strand *strand, const char *path,
                   const char *canonical_digest, char *error, size_t error_length);

/* Reopen a saved index only when its entire serialization matches the
 * independently admitted fragment graph. A digest printed inside an index
 * cannot by itself authorize changing the selected reading traversal.
 * Index arrays are owned by the returned view; source FDs stay caller-owned. */
int ib_index_load_checked(ib_strand *out, const ib_strand *canonical,
                          const char *path, const char *canonical_digest,
                          char *error, size_t error_length);
void ib_index_loaded_free(ib_strand *loaded);

/* A reading position belongs outside the immutable snapshot. It is always
 * bound to the exact snapshot digest and a stable logical object ID (a
 * conversation message or a document fragment), not an OS inode. */
typedef struct {
    char snapshot[65];
    char object[IB_ID_SIZE];
    uint64_t offset;
} ib_read_position;

/* 0 success, 1 absent, -1 malformed/I-O. The caller must separately check
 * object presence and the exact admitted object's byte length before resume. */
int ib_position_read(const char *path, ib_read_position *position);
int ib_position_write(const char *path, const ib_read_position *position);
int ib_position_matches(const ib_read_position *position,
                        const char *snapshot, const char *object, uint64_t bytes);

/* The checked Idriç FragmentStore uses Unicode code-point source coordinates.
 * This explicit, single sequential UTF-8 boundary pass maps disjoint sorted
 * source spans to original-byte spans without pretending that character
 * offsets are byte offsets. O(source bytes + span count) and O(4096) scratch.
 * Conversation raw/opaque bodies instead use direct original-byte ranges. */
typedef struct { uint64_t start, end; } ib_offset_span;
int ib_utf8_map_spans(ib_source *source, const ib_offset_span *codepoints,
                      size_t count, ib_offset_span *bytes,
                      char *error, size_t error_length);

#endif
