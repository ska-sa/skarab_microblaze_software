#building and linking compilation targets below were taken and adapted from Xilinx SDK generated Makefiles

#################################################################
# First run the ./configure script to configure the environment #
# This will generate the Makefile.inc file                      #
#################################################################

-include Makefile.inc

#version info
VERSION=$(shell git describe --dirty=-uncommitted-changes --always --tags --long 2> /dev/null || echo unknown)

#Additional build flags

LDFLAGS +=

#Compile in consistency checks?
CPPFLAGS += -DDO_SANITY_CHECKS

CFLAGS += -DGITVERSION=\"$(VERSION)\" -DGITVERSION_SIZE=$(shell echo -n $(VERSION) | wc -c)

SRCDIR := src/

HDR :=
INC := -I$(INCDIR)

ELF := EMB123701U1R1-$(VERSION).elf

# the cross-compiler details
CROSS_COMPILE := mb-
CC := $(CCPATH)$(CROSS_COMPILE)gcc

RM := rm -rf

LIBS := -lxil,-lgcc,-lc

SRC := $(shell  cd $(SRCDIR); find . -name '*.c' -printf "%f ")

QUIET := $(shell test -d output || mkdir output)
OBJDIR := output/

#OBJ := $(patsubst %.c,%.o,$(SRC))
OBJ := $(SRC:.c=.o)

#compiler flags:
#microblaze cpu specific
MBFLAGS := -mlittle-endian -mcpu=v9.4 -mxl-soft-mul -mno-xl-soft-div
#common flags
CFLAGS += -Wall -Wl,--no-relax -Wuninitialized -Wpedantic
#optimization flags

#CFLAGS += -Os
#  OR
CFLAGS += -O2

#optimiztions: use with \-Wl,--gc-sections\ linker option below
#CFLAGS += -ffunction-sections -fdata-sections

#extra
CFLAGS += $(MBFLAGS) -MMD -MP $(INC)

#linker flags
LDLIBS := -Wl,--start-group,$(LIBS),--end-group
LDSCRIPT := $(SRCDIR)lscript.ld
LDSYM := -Wl,--defsym,CRC_VALUE=0
LDFLAGS += -Wl,-T -Wl,$(LDSCRIPT) -L$(LIBDIR) $(MBFLAGS) -Wl,--no-relax $(LDSYM)

#optimizations: use with \-ffunction-sections -fdata-sections\ above
#LDFLAGS += -Wl,--gc-sections

EXISTS := $(shell test -f Makefile.inc || echo 'NO')

all: check .build

check: .FORCE
ifeq ("$(EXISTS)","NO")
	$(error "First run ./configure to set up environment")
else
	@cat Makefile.inc
endif

.build: $(ELF)

$(ELF): $(addprefix $(OBJDIR), $(OBJ))
	@echo 'Building target: $@'
	@echo 'Invoking: MicroBlaze gcc linker'
	$(CC) $(LDFLAGS) -o "$@" $? $(LDLIBS)
	@echo 'Finished building: $@'
	@echo ' '

$(OBJDIR)%.o: $(SRCDIR)%.c .FORCE
	@echo 'Compiling file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -fmessage-length=0  -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo ' '

copy: $(ELF)
	@echo 'Copying file: $< to $(BUILD_ELF_DIR)'
	$(shell cp $(ELF) $(BUILD_ELF_DIR))

clean-all: clean
	$(RM) *.elf

clean:
	$(RM) $(OBJDIR)*.o $(OBJDIR)*.d $(ELF)

.PHONY: all clean copy check clean-all

.FORCE:
