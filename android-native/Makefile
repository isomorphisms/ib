# Fixed build-tool interface: ICK owns first-party C compilation; the NDK
# assembles and links it. No on-device build and no compiler fallback.
IB_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
AICI_ROOT ?= $(IB_ROOT)/.tools/reader-acceptance
APPENDFAT ?= $(IB_ROOT)/.tools/appendfat
include $(AICI_ROOT)/ick-android/Makefile
.DEFAULT_GOAL := reader-native
READER_BUILD ?= $(IB_ROOT)/build/reader-native-$(ABI)
READER_OUTPUT ?= $(READER_BUILD)/libib_reader.so
READER_CLIENT_SOURCE ?= $(IB_ROOT)/native/android/ib_shizuku_client.c
READER_NDK_CC = $(NDK_BIN)/$(NDK_TARGET_$(ABI))29-clang
# The pinned ARM64 producer prints sign-extended byte MOVI immediates for an
# auto-vectorized validator, which the pinned NDK assembler correctly refuses.
# Declare a scalar ARM64 optimization profile; never rewrite its assembly or
# silently invoke another C frontend. Preserve that failed build as evidence.
READER_OPT_arm64-v8a = -fno-tree-vectorize
READER_FLAGS = $(ICK_NDK_BASE_FLAGS) -D__ANDROID_API__=29 -D__ANDROID_MIN_SDK_VERSION__=29 -std=c11 -O2 $(READER_OPT_$(ABI)) -fPIC -fstack-protector-strong -Wall -Wextra -Wpedantic -Werror -I"$(APPENDFAT)/lib" -I"$(IB_ROOT)/native/android"
READER_C_NAMES = ib_shizuku_parcelable ib_shizuku_client ib_shizuku_worker ib_shizuku_ingress_binder ib_shizuku_ingress_runner ib_shizuku_arena_ingress ib_shizuku_arena_publish ib_reader_pfd ib_arena_pfd_provider ib_arena_source_owner ib_reader_index ib_reader_activity ib_reader_view
READER_OBJECTS = $(addprefix $(READER_BUILD)/,$(addsuffix .o,$(READER_C_NAMES))) $(READER_BUILD)/appendfat_arena.o
READER_EXTRA_LIBS_armeabi-v7a = -latomic

.PHONY: reader-native reader-preflight reader-receipt
reader-native: reader-preflight $(READER_OUTPUT) reader-receipt

reader-preflight:
	test -x "$(ICK_COMPILER)"
	test "$$($(ICK_COMPILER) $(ICK_EXTRA_FLAGS) -dumpmachine)" = "$(GNU_TARGET)"
	test -x "$(READER_NDK_CC)"
	rg -x -F 'Pkg.Revision = 27.2.12479018' "$(NDK)/source.properties"
	test "$$(git -C "$(APPENDFAT)" rev-parse HEAD)" = 05a0cb7246c5a9f46abfbe92925812e7b607f454
	rg -x -F 'source	fbe86e23d55cfec2000c08e61deea2a407fd7175' "$(ICK_STAGE)/qualification/receipt.tsv"
	rg -x -F 'c_frontend	ick' "$(ICK_STAGE)/qualification/receipt.tsv"
	mkdir -p "$(READER_BUILD)" "$(dir $(READER_OUTPUT))"

$(READER_BUILD)/%.s: $(IB_ROOT)/native/android/%.c FORCE | reader-preflight
	"$(ICK_COMPILER)" $(ICK_EXTRA_FLAGS) $(READER_FLAGS) -S "$<" -o "$@"

$(READER_BUILD)/ib_shizuku_client.s: $(READER_CLIENT_SOURCE) FORCE | reader-preflight
	"$(ICK_COMPILER)" $(ICK_EXTRA_FLAGS) $(READER_FLAGS) -S "$<" -o "$@"

$(READER_BUILD)/%.o: $(READER_BUILD)/%.s
	"$(READER_NDK_CC)" $(TARGET_FLAGS) -fPIC -c "$<" -o "$@"

$(READER_BUILD)/appendfat_arena.s: $(APPENDFAT)/lib/appendfat_arena.c FORCE | reader-preflight
	"$(ICK_COMPILER)" $(ICK_EXTRA_FLAGS) $(READER_FLAGS) -S "$<" -o "$@"

$(READER_OUTPUT): $(READER_OBJECTS)
	"$(READER_NDK_CC)" $(TARGET_FLAGS) -shared -Wl,--no-undefined -Wl,-z,max-page-size=16384 -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack $(READER_OBJECTS) -landroid -lbinder_ndk -llog -lm $(READER_EXTRA_LIBS_$(ABI)) -o "$@"

reader-receipt: $(READER_OUTPUT)
	"$(NDK_READELF)" -h -r -l -d "$(READER_OUTPUT)" > "$(READER_BUILD)/elf.txt"
	rg 'Class:.*$(ELF_CLASS)' "$(READER_BUILD)/elf.txt"
	rg -F 'Machine:                           $(ELF_MACHINE)' "$(READER_BUILD)/elf.txt"
	! rg 'R_[A-Z0-9_]+_COPY' "$(READER_BUILD)/elf.txt"
	sha256sum "$(ICK_COMPILER)" "$$($(ICK_COMPILER) $(ICK_EXTRA_FLAGS) -print-prog-name=cc1)" "$(READER_OUTPUT)" > "$(READER_BUILD)/sha256.txt"
	printf 'schema\tib-ICK-native-reader-build-v1\nabi\t%s\napi_floor\t29\nC_frontend\tICK fbe86e23d55cfec2000c08e61deea2a407fd7175\nassembler_linker\tNDK 27.2.12479018\nprofile\tC11 O2 PIC stack-protector-strong, Fortify undefined\nexplicit_ABI_optimization_flags\t%s\nAndroid_execution\tNOT_RUN\nphysical_TAB_P10\tNOT_RUN\n' "$(ABI)" "$(READER_OPT_$(ABI))" > "$(READER_BUILD)/receipt.tsv"
