# Actual response receipt publication repair

IB run 41273278580 at d893697329f7cbba85cb004fca2625b6d22150e1 failed the
installed native document on both API 29 and 34. The authorized worker exited
zero and supplied 873 bytes. The publisher reached stage 32 with errno 13,
after its own frame validation, canonical append/commit and reopened comparison.
This internal trace does not constitute independent source acceptance. The
precise failing syscall inside receipt publication was not captured.

Compact artifact 13070204978 has SHA-256
`3b43cfec83d6fcee7962b0084eda7e2d7ebf1d5e40cae6c052833dfae57fdf00`.
The failed run remains failed; no readable page or later retained/update/reboot
check is claimed.

Immutable receipt publication now uses `renameat2(RENAME_NOREPLACE)` instead of
hard linking. The file is still synchronized before publication and the
directory afterward. An existing target, unsupported syscall or denied access
refuses; it cannot fall back to overwriting a receipt. No storage permission,
owner, identity, metadata, content, digest or generation check is relaxed.
Receipt-only numeric stages 40–43 distinguish open, write/sync/close, publication
and final directory sync without printing source text or URLs.

Host canonical append and independent readback checks passed with qualified ICK
fbe86e23d55cfec2000c08e61deea2a407fd7175. A pre-existing immutable receipt remained
unchanged after a refused publication, and the previous source still reopened.
The separately compiled flag-zero candidate failed at the exact
`publication != 0` assertion. New Android execution is required to qualify this
repair and determine whether it resolves the observed installed-reader failure.
Physical TAB_P10 access remains BLOCKED.
