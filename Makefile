TARGET = cndl.elf

# compiler
CROSS_COMPILE_PREFIX = 
CC      ?= $(CROSS_COMPILE_PREFIX)gcc
CXX     ?= $(CROSS_COMPILE_PREFIX)g++
SIZE    ?= $(CROSS_COMPILE_PREFIX)size

SRC_FOLDERS = src/
LIBS = c pthread yaml-cpp jsoncpp crypto
PACKETS = 
LIB_PATHS =
INCLUDES = src/

MAP_FILE = $(TARGET).map


INCLUDE_CMD = $(addprefix -I, $(INCLUDES))
LIB_CMD = $(addprefix -l, $(LIBS))
LIB_PATH_CMD = $(addprefix -L, $(LIB_PATHS))

# Flags

DEFINES		+= -D_FILE_OFFSET_BITS=64 -DDUK_USE_CPP_EXCEPTIONS

FP_FLAGS       ?=
COMMON_FLAGS	+= $(DEFINES) $(FP_FLAGS)
COMMON_FLAGS	+= -O0 -ggdb
COMMON_FLAGS	+= $(INCLUDE_CMD)
#COMMON_FLAGS    += -fsanitize=address

COMMON_COMPILE_FLAGS += $(foreach PACKET, $(PACKETS), $(shell pkg-config $(PACKET) --cflags))

# Warnings
W_FLAGS      += -Wextra -Wredundant-decls
W_FLAGS      += -Wall -Wundef

###############################################################################
# C flags

CFLAGS		+= $(COMMON_FLAGS)
CFLAGS		+= $(W_FLAGS)
CFLAGS      += -Wimplicit-function-declaration -Wmissing-prototypes -Wstrict-prototypes

###############################################################################
# C++ flags

CPPFLAGS	+= $(COMMON_FLAGS)
CPPFLAGS	+= $(W_FLAGS)
CPPFLAGS	+= -std=c++2a
CPPFLAGS	+= -I$(INCLUDE_DIR)
CPPFLAGS    += -gsplit-dwarf


###############################################################################
# Linker flags

LINKERFLAGS +=  $(COMMON_FLAGS)
LINKERFLAGS += $(foreach PACKET, $(PACKETS), $(shell pkg-config $(PACKET) --libs))
LINKERFLAGS += -fuse-ld=gold
LINKERFLAGS += -Wl,--gdb-index
#LINKERFLAGS += -s

CPP_SUFFIX = .cpp
C_SUFFIX   = .c
OBJ_SUFFIX = .o
DEP_SUFFIX = .d
OBJ_DIR    = obj/

IGNORE_STRINGS = /archive/
#IGNORE_STRINGS = /test/

CPP_FILES            += $(sort $(filter-out $(IGNORE_STRINGS), $(foreach SRC_FOLDER, $(SRC_FOLDERS), $(shell find $(SRC_FOLDER) -name "*$(CPP_SUFFIX)" | grep -v $(addprefix -e, $(IGNORE_STRINGS))))))
C_FILES              += $(sort $(filter-out $(IGNORE_STRINGS), $(foreach SRC_FOLDER, $(SRC_FOLDERS), $(shell find $(SRC_FOLDER) -name "*$(C_SUFFIX)" | grep -v $(addprefix -e, $(IGNORE_STRINGS))))))

CPP_OBJ_FILES        += $(addsuffix $(OBJ_SUFFIX), $(addprefix $(OBJ_DIR), $(CPP_FILES)))
C_OBJ_FILES          += $(addsuffix $(OBJ_SUFFIX), $(addprefix $(OBJ_DIR), $(C_FILES)))

OBJ_FILES = $(CPP_OBJ_FILES) $(C_OBJ_FILES)

DEP_FILES            += $(addsuffix $(DEP_SUFFIX), $(OBJ_FILES))


#### makefile magic

HASH_SUFFIX = _hash
LINKER_HASH = $(OBJ_DIR)$(shell echo $(OBJ_FILES) $(LINKERFLAGS) $(LIB_PATH_CMD) $(LIB_CMD) | md5sum | cut -d ' ' -f 1).linker$(HASH_SUFFIX)
CPP_FLAGS_HASH = $(OBJ_DIR)$(shell echo $(CPPFLAGS) | md5sum | cut -d ' ' -f 1).cpp_flags$(HASH_SUFFIX)
C_FLAGS_HASH = $(OBJ_DIR)$(shell echo $(CFLAGS) | md5sum | cut -d ' ' -f 1).c_flags$(HASH_SUFFIX)

ifndef VERBOSE
SILENT = @
endif


.phony: all clean dbg

all: $(TARGET)

.PRECIOUS: $(CPP_FLAGS_HASH) $(C_FLAGS_HASH)

dbg:
	@echo $(C_FLAGS_HASH)
	@echo $(CPP_FLAGS_HASH) 

clean:
	$(SILENT) rm -rf $(OBJ_DIR) $(TARGET) $(TARGET).map $(TARGET).bin

$(TARGET): $(OBJ_FILES) $(LINKER_HASH)
	@echo linking $@
	$(SILENT) $(CXX) -o $@ $(OBJ_FILES) $(LINKERFLAGS) $(LIB_PATH_CMD) $(LIB_CMD)
	$(SILENT) $(SIZE) $@
	@ echo done

$(OBJ_DIR)%$(HASH_SUFFIX): | $(OBJ_DIR)
	$(SILENT) rm -f $(OBJ_DIR)*$(suffix $@)
	$(SILENT) touch $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)%$(C_SUFFIX)$(OBJ_SUFFIX): %$(C_SUFFIX) $(C_FLAGS_HASH) | $(OBJ_DIR)
	@echo building $<
	@ mkdir -p $(dir $@)
	$(SILENT) $(CC) $(CFLAGS) -o $@ -c $<
	
	@ $(CC) $(CFLAGS) -MM -MF $(OBJ_DIR)$<$(DEP_SUFFIX) -c $<
	@ sed -i -e 's|.*:|$@:|' $(OBJ_DIR)$<$(DEP_SUFFIX)
	
$(OBJ_DIR)%$(CPP_SUFFIX)$(OBJ_SUFFIX): %$(CPP_SUFFIX) $(CPP_FLAGS_HASH) | $(OBJ_DIR)
	@echo building $<
	@ mkdir -p $(dir $@)
	$(SILENT) $(CXX) $(CPPFLAGS) -o $@ -c $<
	
	@ $(CXX) $(CPPFLAGS) -MM -MF $@$(DEP_SUFFIX) -c $<
	@ sed -i -e 's|.*:|$@:|' $@$(DEP_SUFFIX)

-include $(DEP_FILES)
