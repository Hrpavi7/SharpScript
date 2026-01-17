# WARNING:
# THIS ONLY WORKS ON WINDOWS 10/11.
# IT USES A MINGW32 COMPILER.
# USE MINGW32-MAKE TO MAKE THE PROJECT.

# licensed under the MIT license

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# windows
ifeq ($(OS),Windows_NT)
	TARGET_EXT = .exe
	FIXPATH = $(subst /,\,$1)
	RM = del /Q /F
	RMDIR = rmdir /S /Q
	MKDIR = mkdir
else
	TARGET_EXT =
	FIXPATH = $1
	RM = rm -f
	RMDIR = rm -rf
	MKDIR = mkdir -p
endif

SRCDIRS = $(SRCDIR) $(SRCDIR)/builtins
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJECTS = $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SOURCES)))
TARGET = $(BINDIR)/sharpscript$(TARGET_EXT)

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) -lm

vpath %.c $(SRCDIRS)
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
ifeq ($(OS),Windows_NT)
	@if not exist $(OBJDIR) $(MKDIR) $(OBJDIR)
else
	$(MKDIR) $(OBJDIR)
endif

$(BINDIR):
ifeq ($(OS),Windows_NT)
	@if not exist $(BINDIR) $(MKDIR) $(BINDIR)
else
	$(MKDIR) $(BINDIR)
endif

clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJDIR) $(RMDIR) $(OBJDIR)
	@if exist $(BINDIR) $(RMDIR) $(BINDIR)
else
	$(RMDIR) $(OBJDIR) $(BINDIR)
endif

.PHONY: all clean
