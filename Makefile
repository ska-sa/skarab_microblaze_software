#building and linking compilation targets below were taken and adapted from Xilinx SDK generated Makefiles

#################################################################
# First run the ./configure script to configure the environment #
# This will generate the Makefile.inc file                      #
#################################################################

-include Makefile.inc Makefile.config

#version info
GIT_BRANCH=$(shell git rev-parse --verify -q --abbrev-ref HEAD 2>/dev/null || echo unknown)
GIT_DESCRIBE=$(shell git describe --dirty=-uncommitted-changes --always --tags --long 2> /dev/null || echo unknown)

VERSION=$(GIT_DESCRIBE)-$(GIT_BRANCH)

#Additional build flags

LDFLAGS +=

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

QUIET := $(shell test -d elf || mkdir elf)
ELFDIR := elf/

#OBJ := $(patsubst %.c,%.o,$(SRC))
OBJ := $(SRC:.c=.o)

#compiler flags:
#microblaze cpu specific
MBFLAGS := -mlittle-endian -mcpu=v9.4 -mxl-soft-mul -mno-xl-soft-div
#common flags
CFLAGS += -Wall -Wl,--no-relax -Wuninitialized -Wpedantic
#some extra flags to check switch statements (not automatically enabled by -Wall)
CFLAGS += -Wswitch-enum -Wswitch-default
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
LDSYM1 := -Wl,--defsym,CRC_VALUE=0
LDFLAGS += -Wl,-T -Wl,$(LDSCRIPT) -L$(LIBDIR) $(MBFLAGS) -Wl,--no-relax

#optimizations: use with \-ffunction-sections -fdata-sections\ above
#LDFLAGS += -Wl,--gc-sections

EXISTS := $(shell test -f Makefile.inc || echo 'NO')

ADLER32 := utils/adler32/adler32

#some decorative stuff
RED_COLOUR ="\033[0;31m"
END_COLOUR = "\033[0m"

all: check .build

check: .FORCE
ifeq ("$(EXISTS)","NO")
	$(error "First run ./configure to set up environment")
else
	@cat Makefile.inc
endif

.build: $(ELF)

#link second time to insert checksum into elf
$(ELF): $(addprefix $(OBJDIR), $(OBJ)) .temp.elf $(ADLER32)
	@echo 'Building target: $@'
	@echo 'Invoking: MicroBlaze gcc linker'
	$(eval CALC = $(shell $(CCPATH)/mb-objcopy --only-section .text .temp.elf -O binary temp.bin 2>/dev/null; ./$(ADLER32) -f temp.bin 2>/dev/null | grep \^checksum | cut -f3 -d' '))
	@$(RM) .temp.elf temp.bin
	@test $(CALC)
	$(eval CALC_FMT = $(shell printf 0x%08x 0x$(CALC)))
	$(eval LDSYM2 = -Wl,--defsym,CRC_VALUE=$(CALC_FMT))
	$(eval DEPS = $(filter-out .temp.elf $(ADLER32), $?))
	$(CC) $(LDFLAGS) $(LDSYM2) -o "$(ELFDIR)$@" $(DEPS) $(LDLIBS)
	@echo $(RED_COLOUR)
	@echo 'Finished building: $@ in $(ELFDIR) directory'
	@echo $(END_COLOUR)

#link first time to compile elf in order to calculate adler checksum
.temp.elf: $(addprefix $(OBJDIR), $(OBJ))
	$(CC) $(LDFLAGS) $(LDSYM1) -o "$@" $? $(LDLIBS)

$(OBJDIR)%.o: $(SRCDIR)%.c .FORCE
	@echo 'Compiling file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -fmessage-length=0  -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo ' '

$(ADLER32):
	@echo ' '
	$(MAKE) -w -C utils/adler32/
	@echo ' '

copy: $(ELF)
	@echo 'Copying file: $< to $(BUILD_ELF_DIR)'
	$(shell cp $(ELFDIR)/$(ELF) $(BUILD_ELF_DIR))

clean-all: clean
	$(RM) $(ELFDIR)*.elf

clean:
	$(MAKE) -C utils/adler32/ clean
	$(RM) $(OBJDIR)*.o $(OBJDIR)*.d $(ELFDIR)$(ELF)

.PHONY: all clean copy check clean-all

.FORCE:
