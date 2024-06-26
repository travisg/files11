
TARGET := files11
BUILDDIR := build-$(TARGET)

# compiler flags, default libs to link against
COMPILEFLAGS := -g -O -Wall -Wextra -Werror
CFLAGS :=
CXXFLAGS := -std=c++20 -fno-exceptions
ASMFLAGS :=
LDFLAGS :=
LDLIBS :=

UNAME := $(shell uname -s)
ARCH := $(shell uname -m)

# switch any platform specific stuff here
# ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# ...
# endif
ifeq ($(UNAME),Darwin)
CC := clang
CPLUSPLUS := clang++
COMPILEFLAGS += -I/opt/local/include
LDFLAGS += -L/opt/local/lib
LDLIBS +=
OTOOL := otool
endif
ifeq ($(UNAME),Linux)
CC := cc
CPLUSPLUS := c++
OBJDUMP := objdump
LDLIBS +=
endif
NOECHO ?= @

CFLAGS += $(COMPILEFLAGS)
CXXFLAGS += $(COMPILEFLAGS)
ASMFLAGS += $(COMPILEFLAGS)

OBJS := \
	main.o \
	disk.o \
	file.o \
	filesystem.o \
	utils.o

OBJS := $(addprefix $(BUILDDIR)/,$(OBJS))

DEPS := $(OBJS:.o=.d)

.PHONY: all
all: $(BUILDDIR)/$(TARGET) $(BUILDDIR)/$(TARGET).lst

$(BUILDDIR)/$(TARGET): $(OBJS)
	$(CPLUSPLUS) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(BUILDDIR)/$(TARGET).lst: $(BUILDDIR)/$(TARGET)
ifeq ($(UNAME),Darwin)
	$(OTOOL) -Vt $< | c++filt > $@
else
	$(OBJDUMP) -Cd $< > $@
endif

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

spotless:
	rm -rf build-*

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

$(BUILDDIR)/%.o: %.c
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.cpp
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CPLUSPLUS) $(CXXFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.S
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(ASMFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
