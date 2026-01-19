# Cross-platform build system detection and configuration
#
# Supported OS:
#   - Windows 10/11 (32/64-bit)
#   - macOS
#   - Linux (Most of the distros)
#
# Supported build tools:
#   - mingw32-make, make/gmake, nmake, cmake
#
# Usage examples:
#   Windows (PowerShell/CMD):   mingw32-make all
#   macOS/Linux (sh/bash):      make all
#   Detection overview:         make detect
#   Validate toolchain:         make validate
#
# Notes:
#   - Detection uses environment variables on Windows and uname/os-release on Unix.
#   - Fallbacks provide warnings when preferred tools are missing.
#   - Platform-specific flags are applied to CFLAGS and LDFLAGS.
#	
#	Copyright (c) 2024-2026 SharpScript Programming Language
#	
#	Licensed under the MIT License

SRCDIR := src
BINDIR := bin
OBJDIR := $(BINDIR)/obj

CFLAGS_BASE := -Wall -Wextra -std=c99 -Isrc
LDFLAGS_BASE := -lm

ifeq ($(OS),Windows_NT)
  HOST_OS := windows
  TARGET_EXT := .exe
  FIXPATH = $(subst /,\,$1)
  RM := del /Q /F
  RMDIR := rmdir /S /Q
  MKDIR := mkdir
  # architecture (prefer WOW64 value when present)
  HOST_ARCH := $(PROCESSOR_ARCHITECTURE)
  ifneq ($(PROCESSOR_ARCHITEW6432),)
    HOST_ARCH := $(PROCESSOR_ARCHITEW6432)
  endif
  HOST_VERSION := $(strip $(shell ver))
  HOST_DISTRO := windows
else
  UNAME_S := $(shell uname -s)
  UNAME_M := $(shell uname -m)
  HOST_ARCH := $(UNAME_M)
  FIXPATH = $1
  RM := rm -f
  RMDIR := rm -rf
  MKDIR := mkdir -p
  ifeq ($(UNAME_S),Darwin)
    HOST_OS := macos
    TARGET_EXT :=
    HOST_VERSION := $(shell sw_vers -productVersion 2>/dev/null || uname -r)
    HOST_DISTRO := macos
  else
    HOST_OS := linux
    TARGET_EXT :=
    HOST_VERSION := $(shell uname -r)
    HOST_DISTRO := $(shell sh -c 'if [ -f /etc/os-release ]; then . /etc/os-release; echo $$ID; elif command -v lsb_release >/dev/null 2>&1; then lsb_release -si | tr "[:upper:]" "[:lower:]"; else echo unknown; fi')
    LINUX_ID_LIKE := $(shell sh -c 'if [ -f /etc/os-release ]; then . /etc/os-release; echo $$ID_LIKE; else echo; fi')
    LINUX_VERSION_ID := $(shell sh -c 'if [ -f /etc/os-release ]; then . /etc/os-release; echo $$VERSION_ID; else echo; fi')
  endif
endif

ifeq ($(HOST_OS),macos)
  CC ?= clang
else
  CC ?= gcc
endif

CFLAGS := $(CFLAGS_BASE)
LDFLAGS := $(LDFLAGS_BASE)

ifneq (,$(findstring 64,$(HOST_ARCH)))
  CFLAGS += -DARCH_X64
else
  CFLAGS += -DARCH_X86
endif

# platform-specific defines
ifeq ($(HOST_OS),windows)
  CFLAGS += -D_WIN32
  ifneq ($(findstring 6.1,$(HOST_VERSION)),)
    CFLAGS += -DWINVER_7
  else ifneq ($(findstring 6.3,$(HOST_VERSION)),)
    CFLAGS += -DWINVER_8_1
  else ifneq ($(findstring 10.,$(HOST_VERSION)),)
    CFLAGS += -DWINVER_10_PLUS
  else
    CFLAGS += -DWINVER_UNKNOWN
  endif
else ifeq ($(HOST_OS),macos)
  CFLAGS += -D_DARWIN_C_SOURCE
  MACOS_VER := $(shell sw_vers -productVersion 2>/dev/null)
  MACOS_MAJOR := $(shell sh -c 'v="$(MACOS_VER)"; echo $$v | cut -d. -f1')
  MACOS_MINOR := $(shell sh -c 'v="$(MACOS_VER)"; echo $$v | cut -d. -f2')
  CFLAGS += -DMACOS_VERSION_MAJOR=$(MACOS_MAJOR) -DMACOS_VERSION_MINOR=$(MACOS_MINOR)
  ifeq ($(HOST_ARCH),arm64)
    CFLAGS += -DAPPLE_SILICON -arch arm64
  else
    CFLAGS += -DAPPLE_INTEL -arch x86_64
  endif
else ifeq ($(HOST_OS),linux)
  CFLAGS += -D_GNU_SOURCE
  ifneq (,$(filter ubuntu debian,$(HOST_DISTRO)))
    CFLAGS += -DLINUX_DEBIAN_FAMILY
  else ifneq (,$(filter fedora,$(HOST_DISTRO)))
    CFLAGS += -DLINUX_FEDORA
  else ifneq (,$(filter centos rhel rocky almalinux,$(HOST_DISTRO) $(LINUX_ID_LIKE)))
    CFLAGS += -DLINUX_RHEL_FAMILY
  else ifneq (,$(filter arch manjaro,$(HOST_DISTRO)))
    CFLAGS += -DLINUX_ARCH_FAMILY
  else ifneq (,$(filter opensuse tumbleweed,$(HOST_DISTRO)))
    CFLAGS += -DLINUX_SUSE
  else ifneq (,$(filter alpine,$(HOST_DISTRO)))
    CFLAGS += -DLINUX_ALPINE
  else
    CFLAGS += -DLINUX_UNKNOWN
  endif
endif

# build tool detection (mingw32-make, make/gmake, nmake, cmake...)
ifeq ($(HOST_OS),windows)
  define tool_exists
$(strip $(shell where $(1) 2>nul))
  endef
else
  define tool_exists
$(strip $(shell command -v $(1) >/dev/null 2>&1 && echo yes || echo))
  endef
endif

HAVE_MINGW32_MAKE := $(call tool_exists,mingw32-make)
HAVE_MAKE         := $(call tool_exists,make)
HAVE_GMAKE        := $(call tool_exists,gmake)
HAVE_NMAKE        := $(call tool_exists,nmake)

ifeq ($(HOST_OS),windows)
  ifneq ($(HAVE_MINGW32_MAKE),)
    BUILD_TOOL := mingw32-make
  else ifneq ($(HAVE_NMAKE),)
    BUILD_TOOL := nmake
  else ifneq ($(HAVE_MAKE),)
    BUILD_TOOL := make
  else
    $(warning No build tool found. Install mingw32-make, nmake, or make.)
    BUILD_TOOL :=
  endif
else
  ifneq ($(HAVE_GMAKE),)
    BUILD_TOOL := gmake
  else ifneq ($(HAVE_MAKE),)
    BUILD_TOOL := make
  else
    $(warning No build tool found. Install make or gmake.)
    BUILD_TOOL :=
  endif
endif

# tool version (best-effort; may be verbose on windows)
BUILD_TOOL_VERSION := $(if $(BUILD_TOOL),$(shell $(BUILD_TOOL) --version 2>&1),unknown)

# package management
ifeq ($(HOST_OS),linux)
  SUDO := $(shell command -v sudo >/dev/null 2>&1 && echo sudo || echo)
  ifneq (,$(filter ubuntu debian,$(HOST_DISTRO)))
    INSTALL_DEPS := $(SUDO) apt-get update && $(SUDO) apt-get install -y build-essential
  else ifneq (,$(filter fedora,$(HOST_DISTRO)))
    INSTALL_DEPS := $(SUDO) dnf -y groupinstall "Development Tools"
  else ifneq (,$(filter centos rhel rocky almalinux,$(HOST_DISTRO) $(LINUX_ID_LIKE)))
    INSTALL_DEPS := $(SUDO) yum -y groupinstall "Development Tools"
  else ifneq (,$(filter arch manjaro,$(HOST_DISTRO)))
    INSTALL_DEPS := $(SUDO) pacman -Sy --needed --noconfirm base-devel
  else ifneq (,$(filter opensuse tumbleweed,$(HOST_DISTRO)))
    INSTALL_DEPS := $(SUDO) zypper -n install -t pattern devel_basis || $(SUDO) zypper -n install gcc make
  else ifneq (,$(filter alpine,$(HOST_DISTRO)))
    INSTALL_DEPS := $(SUDO) apk add --no-cache build-base
  else
    INSTALL_DEPS := echo "Unknown Linux distro '$(HOST_DISTRO)'. Please install gcc and make manually."
  endif
else ifeq ($(HOST_OS),macos)
  HAVE_BREW := $(shell command -v brew >/dev/null 2>&1 && echo yes || echo)
  ifneq ($(HAVE_BREW),)
    INSTALL_DEPS := brew install make gcc || true
  else
    INSTALL_DEPS := xcode-select --install || echo "Install Homebrew and developer tools manually."
  endif
else ifeq ($(HOST_OS),windows)
  HAVE_WINGET := $(strip $(shell where winget 2>nul))
  HAVE_CHOCO := $(strip $(shell where choco 2>nul))
  ifneq ($(HAVE_WINGET),)
    INSTALL_DEPS := winget install -e --id MSYS2.MSYS2 || winget install -e --id GnuWin32.Make || echo "Install mingw32-make/gcc via MSYS2 manually."
  else ifneq ($(HAVE_CHOCO),)
    INSTALL_DEPS := choco install -y make mingw || echo "Ensure mingw32-make is available in PATH."
  else
    INSTALL_DEPS := echo "Install MSYS2/MinGW or appropriate build tools (mingw32-make, gcc) and ensure they are in PATH."
  endif
else
  INSTALL_DEPS := echo "Unsupported OS for automatic dependency installation."
endif

SRCDIRS := $(SRCDIR) $(SRCDIR)/builtins
SOURCES := $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SOURCES)))
TARGET  := $(BINDIR)/sharpscript$(TARGET_EXT)

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

vpath %.c $(SRCDIRS)
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
ifeq ($(HOST_OS),windows)
	@if not exist $(OBJDIR) $(MKDIR) $(OBJDIR)
else
	$(MKDIR) $(OBJDIR)
endif

$(BINDIR):
ifeq ($(HOST_OS),windows)
	@if not exist $(BINDIR) $(MKDIR) $(BINDIR)
else
	$(MKDIR) $(BINDIR)
endif

# clean, detect, validate, install-deps, test

clean:
ifeq ($(HOST_OS),windows)
	@if exist $(OBJDIR) $(RMDIR) $(OBJDIR)
	@if exist $(BINDIR) $(RMDIR) $(BINDIR)
else
	$(RMDIR) $(OBJDIR) $(BINDIR)
endif

# print detection info
ifeq ($(HOST_OS),windows)
detect:
	@echo OS=$(HOST_OS)
	@echo ARCH=$(HOST_ARCH)
	@echo DISTRO=$(HOST_DISTRO)
	@echo VERSION=$(HOST_VERSION)
	@echo CC=$(CC)
	@echo CFLAGS=$(CFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo BUILD_TOOL=$(BUILD_TOOL)
	@IF NOT "$(BUILD_TOOL)"=="" ($(BUILD_TOOL) --version) ELSE (echo unknown)
else
detect:
	@echo OS=$(HOST_OS)
	@echo ARCH=$(HOST_ARCH)
	@echo DISTRO=$(HOST_DISTRO)
	@echo VERSION=$(HOST_VERSION)
	@echo CC=$(CC)
	@echo CFLAGS=$(CFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo BUILD_TOOL=$(BUILD_TOOL)
	@if [ -n "$(BUILD_TOOL)" ]; then $(BUILD_TOOL) --version || true; else echo unknown; fi
endif

# validate toolchain
ifeq ($(HOST_OS),windows)
validate:
	@echo Validating toolchain on Windows...
	@where $(CC) >nul 2>&1 || (echo ERROR: Compiler $(CC) not found && exit /b 1)
	@IF NOT "$(BUILD_TOOL)"=="" (echo Build tool detected: $(BUILD_TOOL)) ELSE (echo WARNING: No build tool detected)
	@if not "$(WINVER_UNKNOWN)"=="" echo WARNING: Unrecognized Windows version string: $(HOST_VERSION)
else
validate:
	@echo "Validating toolchain on $(HOST_OS)..."
	@command -v $(CC) >/dev/null 2>&1 || (echo "ERROR: Compiler $(CC) not found" && exit 1)
	@if [ -n "$(BUILD_TOOL)" ]; then echo "Build tool detected: $(BUILD_TOOL)"; else echo "WARNING: No build tool detected"; fi
	@if [ "$(HOST_OS)" = "linux" ] && [ "$(HOST_DISTRO)" = "unknown" ]; then echo "WARNING: Unknown Linux distro"; fi
endif

install-deps:
	@$(INSTALL_DEPS)

test:
	@$(MAKE) -s detect
	@$(MAKE) -s validate
	@$(MAKE) -s all
ifeq ($(HOST_OS),windows)
	@IF EXIST "$(TARGET)" (echo Smoke run... & "$(TARGET)" || (echo Program exited with code %ERRORLEVEL% & exit /b 0)) ELSE (echo Build output missing)
else
	@echo "Smoke run..."
	@$(TARGET) || true
endif

.PHONY: all clean detect validate install-deps test
