/*
 * Thin IB <-> SURFER host evidence adapter.  Formula parsing, sparse terms,
 * differentiation, and ray tracing come from a PINNED SURFER PR #33 checkout.
 * This tool neither supplies an illustration nor an Android deep link.
 */
#include "formula.h"
#include "surfer_raytracer.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const surfer_term expected[4] = {
    { 1.0, 4, 0, 0 },
    {-1.0, 1, 3, 0 },
    {-1.0, 0, 0, 4 },
    { 1.0, 0, 0, 1 }
};

static int same_terms(const ave_formula *parsed)
{
    if (parsed->surface.surface.term_count != 4) return 0;
    for (size_t i = 0; i != 4; ++i) {
        size_t matches = 0;
        for (size_t j = 0; j != 4; ++j) {
            const surfer_term got = parsed->terms[j];
            const surfer_term want = expected[i];
            if (got.x_exponent == want.x_exponent &&
                got.y_exponent == want.y_exponent &&
                got.z_exponent == want.z_exponent &&
                got.coefficient == want.coefficient) ++matches;
        }
        if (matches != 1) return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: ib-surfer-probe EXACT_SOURCE_EXPRESSION\n", stderr);
        return 2;
    }
    ave_formula parsed = {0};
    char error[128] = {0};
    if (!ave_formula_compile(argv[1], &parsed, error, sizeof error)) {
        puts("UNSUPPORTED\tparse_error");
        return 0;
    }
    if (!same_terms(&parsed)) {
        puts("UNSUPPORTED\tchanged_polynomial");
        return 0;
    }

    /* Test the genuine numerical engine, not a hand-drawn replacement.
     * Clipping: unit sphere; 32x32 orthographic camera; fixed yaw/pitch/zoom.
     * This is a host smoke, NOT a finished still or certified geometry.
     */
    enum { side = 32 };
    uint32_t pixels[side * side];
    const surfer_scene scene = surfer_default_scene(&parsed.surface);
    if (!surfer_render_orthographic(
          &scene, side, side, 2.4, 0.25, 0.25, 1.0, pixels)) {
        puts("EXACT\t+1:4,0,0;-1:1,3,0;-1:0,0,4;+1:0,0,1\tENGINE_FAILED");
        return 0;
    }
    const uint32_t background = surfer_color_to_argb(scene.background);
    size_t foreground = 0;
    uint64_t digest = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < side * side; ++i) {
        if (pixels[i] != background) ++foreground;
        for (unsigned shift = 0; shift < 32; shift += 8) {
            digest ^= (pixels[i] >> shift) & 255U;
            digest *= UINT64_C(1099511628211);
        }
    }
    if (foreground == 0) {
        puts("EXACT\t+1:4,0,0;-1:1,3,0;-1:0,0,4;+1:0,0,1\tENGINE_EMPTY");
        return 0;
    }
    printf("EXACT\t+1:4,0,0;-1:1,3,0;-1:0,0,4;+1:0,0,1\t"
           "ENGINE_HOST_PROBE_ONLY;pixels=%d;foreground=%zu;fnv64=%016" PRIx64 "\n",
           side * side, foreground, digest);
    return 0;
}
