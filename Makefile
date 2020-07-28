#building and linking compilation targets below were taken and adapted from Xilinx SDK generated Makefiles

#################################################################
# First run the ./configure script to configure the environment #
# This will generate the Makefile.inc file                      #
#################################################################

-include Makefile.inc Makefile.config

# get version info for elf naming later

GIT_BRANCH=$(shell git rev-parse --verify -q --abbrev-ref HEAD 2>/dev/null || echo unknown)

# Note: we want to temporarily ignore any changes to Makefile.config when getting the version info
#	otherwise the elf will be tagged with "uncommitted-changes" when configuring a build
GIT_DESCRIBE=$(shell git update-index --assume-unchanged Makefile.config; \
	git describe --dirty=-uncommitted-changes --always --tags --long 2> /dev/null || echo unknown; \
	git update-index --no-assume-unchanged Makefile.config)

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

OBJCOPY := $(CCPATH)$(CROSS_COMPILE)objcopy

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
ifndef MBVERSION
MBVERSION = v11.0
endif
MBFLAGS := -mlittle-endian -mcpu=$(MBVERSION) -mxl-soft-mul -mno-xl-soft-div
#common flags
CFLAGS += -Wall -Wl,--no-relax -Wuninitialized -Wpedantic
#some extra flags to check switch statements (not automatically enabled by -Wall)
CFLAGS += -Wswitch-enum -Wswitch-default
CFLAGS += -Warray-bounds
#optimization flags

#CFLAGS += -Os
#  OR
CFLAGS += -O2

#optimiztions: use with \-Wl,--gc-sections\ linker option below
#CFLAGS += -ffunction-sections -fdata-sections

#extra
CFLAGS += $(MBFLAGS) -MMD -MP

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

lib:
	 export PATH=${PATH}:${CCPATH}; ${MAKE} -C bsp/skarab_microblaze_bsp

check: .FORCE
ifeq ("$(EXISTS)","NO")
	$(error "First run ./configure to set up environment")
else
	@cat Makefile.inc
endif

.build: $(ELF)
	@echo "\nAdding compile-time flags to .cflags section (no-load) in elf"
	@$(shell echo $(CFLAGS) $(CPPFLAGS) > .cflags.temp)
	#run 'objdump -sj .cflags <elf>' to see the compiler flags the elf was built with
	$(OBJCOPY) --add-section .cflags=.cflags.temp --set-section-flags .cflags=noload,readonly $(ELFDIR)$(ELF)
	@$(RM) .cflags.temp
	@echo $(RED_COLOUR)
	@echo 'Finished building: $< in $(ELFDIR) directory'
	@echo $(END_COLOUR)

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

#link first time to compile elf in order to calculate adler checksum
.temp.elf: $(addprefix $(OBJDIR), $(OBJ))
	$(CC) $(LDFLAGS) $(LDSYM1) -o "$@" $? $(LDLIBS)

$(OBJDIR)%.o: $(SRCDIR)%.c .FORCE
	@echo 'Compiling file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INC) -c -fmessage-length=0  -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo ' '

$(ADLER32):
	@echo ' '
	$(MAKE) -w -C utils/adler32/
	@echo ' '

clean-all: clean
	$(RM) $(ELFDIR)*.elf

clean:
	$(MAKE) -C utils/adler32/ clean
	$(RM) $(OBJDIR)*.o $(OBJDIR)*.d $(ELFDIR)$(ELF)

.PHONY: all clean check clean-all

.FORCE:
