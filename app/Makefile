TITLE         := PS4 Remote Detector
TITLE_ID      := REMD00001
CONTENT_ID    := IV0000-REMD00001_00-PS4REMOTED000001
PKG_LABEL     := 1.0.0
APP_VER_VALUE := 01.15
MASTER_VERSION := 01.15
PAID          := 0x3800000000000011
AUTHINFO      := 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

REPO_ROOT     := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../ps4-linux-pro)
TOOLCHAIN     ?= $(REPO_ROOT)/vendor/OpenOrbis/PS4Toolchain

LIBS          := -lc -lkernel -lc++ -lSceVideoOut -lSceGnmDriver -lSceSysmodule -lSceFreeType -lScePad -lSceKeyboard -lSceUserService -lSceSystemService -lSceLibcInternal -Llibs -lLog

INCLUDES      := -Iinclude -Iexternal
CFLAGS        := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include $(INCLUDES) -Wall -Wextra -std=c17 -O2 -D_DEFAULT_SOURCE
CXXFLAGS      := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=c++17
LDFLAGS       := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(TOOLCHAIN)/lib/crt1.o

ODIR          := build
SDIR          := src
CFILES        := $(SDIR)/BootTrace.c
CPPFILES      := $(SDIR)/main.cpp $(SDIR)/Graphics.cpp $(SDIR)/Utf8Util.cpp $(SDIR)/PadProbe.cpp $(SDIR)/PadBitNames.cpp $(SDIR)/EventLog.cpp $(SDIR)/SystemServiceRuntime.cpp
OBJS          := $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(CFILES)) $(patsubst $(SDIR)/%.cpp,$(ODIR)/%.o,$(CPPFILES))

ASSETS        := $(shell find assets -type f 2>/dev/null)
SCEMODULES    := $(shell find sce_module -type f 2>/dev/null)
SCESYS        := $(shell find sce_sys -type f 2>/dev/null)

UNAME_S       := $(shell uname -s)
FSELF         := create-fself
CDIR          := linux
ifeq ($(UNAME_S),Darwin)
FSELF         := create-fself-macos
CDIR          := macos
endif

PS4LINUX_FSELF := $(REPO_ROOT)/tools/ps4-linux-fself.sh

.PHONY: all clean pkg
all: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin sce_sys/param.sfo $(SCESYS) $(SCEMODULES) $(ASSETS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(APP_VER_VALUE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 32
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gde'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(MASTER_VERSION)'

LLVM_BIN      := $(if $(wildcard /usr/local/opt/llvm/bin/clang++),/usr/local/opt/llvm/bin,$(REPO_ROOT)/vendor/llvm-macos/bin)

eboot.bin: $(OBJS)
	$(LLVM_BIN)/ld.lld $(OBJS) -o $(ODIR)/eboot.elf $(LDFLAGS) $(LIBS)
	chmod +x $(PS4LINUX_FSELF)
	OO_PS4_TOOLCHAIN=$(TOOLCHAIN) $(PS4LINUX_FSELF) -in=$(ODIR)/eboot.elf -out=$(ODIR)/eboot.oelf --eboot "eboot.bin" --paid $(PAID) --authinfo=$(AUTHINFO)

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $(ODIR)
	$(LLVM_BIN)/clang $(CFLAGS) -o $@ $<

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(ODIR)
	$(LLVM_BIN)/clang++ $(CXXFLAGS) -o $@ $<

$(ODIR):
	@mkdir -p $(ODIR)

clean:
	rm -rf build eboot.bin pkg.gp4 sce_sys/param.sfo *.pkg $(ODIR)
