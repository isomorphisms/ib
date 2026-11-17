#!/bin/sh
set -eu
repository=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
mkdir -p "${IB_ADMIN_BUILD:-/tmp/ib-admin-host-build}"
build=${IB_ADMIN_BUILD:-/tmp/ib-admin-host-build}
cc=${IB_ADMIN_CC:-clang}
"$cc" -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -fstack-protector-strong \
    -I"$repository/native/android" "$repository/native/android/ib_admin_transaction.c" \
    "$repository/tests/admin/test_journal.c" -o "$build/test_journal"
"$build/test_journal"
"$cc" -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -fstack-protector-strong \
    -I"$repository/native/android" "$repository/native/android/ib_admin_transaction.c" \
    "$repository/tests/admin/journal_driver.c" -o "$build/journal_driver"
