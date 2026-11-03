#ifndef IB_BROWSER_DOM_SOURCE_H
#define IB_BROWSER_DOM_SOURCE_H

#include "ib_arena_source_owner.h"

#define IB_DOM_MAX_BYTES (2U * 1024U * 1024U)
#define IB_DOM_MAX_TARGETS 3U

typedef struct {
    uint32_t occurrence;
    char original_href[2049], requested_url[2049];
    char request_id[IB_READER_REQUEST_ID_CAP + 1U];
} ib_dom_target_reference;

/* UTF-8 content-DOM outerHTML serialization, with distinct browser-rendered provenance.
 * A cloned DOM removes script/style/template/svg/canvas; the live DOM is unchanged.
 * No field purports to observe a browser HTTP status/body/charset/redirect.
 * Parent and target IDs refer to independently verified ICU receipts. */
typedef struct {
    char request_id[IB_READER_REQUEST_ID_CAP + 1U];
    uint32_t source_generation;
    char requested_url[2049], resolved_url[2049];
    char parent_id[IB_READER_REQUEST_ID_CAP + 1U];
    uint32_t parent_generation;
    char parent_sha256[65];
    char provider_package[129], provider_version[129];
    uint64_t arena_offset;
    uint32_t byte_count;
    char sha256[65];
    unsigned target_count;
    ib_dom_target_reference targets[IB_DOM_MAX_TARGETS];
} ib_browser_dom_metadata;

/* Commit only the explicitly tagged DOM bytes. Parent ICU identity is checked
 * again; the caller must later seal a complete result-resolution receipt. */
int ib_browser_dom_append(ib_arena_source_owner *owner,
                         uint64_t navigation_generation,
                         ib_browser_dom_metadata *metadata,
                         const unsigned char *serialized, size_t count);

/* Atomic, immutable receipt after successful destination qualification. This
 * does not admit a view; checked Idric revalidates each source/link association. */
int ib_browser_dom_seal(ib_arena_source_owner *owner,
                       uint64_t navigation_generation,
                       const ib_browser_dom_metadata *metadata);

int ib_browser_dom_open(ib_arena_source_owner *owner,
                       const char *request_id,
                       uint32_t retained_source_generation,
                       uint64_t navigation_generation,
                       ib_browser_dom_metadata *metadata,
                       ib_reader_source_ticket *ticket, int *read_fd);

#endif
