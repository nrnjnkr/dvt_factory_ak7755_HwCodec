TARGETS := wdog
SYSTEM_BITS:=$(shell getconf LONG_BIT)
ifeq ($(SYSTEM_BITS),64)
DEPS_PATH := ./deps/linux_x86_64
else
DEPS_PATH := ./deps/linux_x86
endif
LIB_PATH := $(DEPS_PATH)/lib
LIBS := ev m rt
INCS := . \
		$(DEPS_PATH)/include

RM := rm -f
PS = c
CC = gcc
CPPFLAGS = -g -march=native -fno-strict-aliasing
CPPFLAGS += $(addprefix -I,$(INCS))
#CPPFLAGS += -MMD
SOURCE := $(wildcard *.$(PS))
SOURCE +=

OBJS := $(patsubst %.$(PS),%.o,$(SOURCE))
DEPS := $(patsubst %.o,%.d,$(OBJS))
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS))
MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.$(PS),$(MISSING_DEPS)))

.PHONY : all deps objs clean rebuild

all : $(TARGETS)

deps : $(DEPS)
	#$(CC) -MM -MMD $(SOURCE)
	$(CC) -MM $(SOURCE)

objs : $(OBJS)

clean :
	@$(RM) *.o
	@$(RM) *.d
	@$(RM) $(TARGETS)

rebuild: clean all

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) :
	@$(RM) $(patsubst %.d,%.o,$@)
endif

-include $(DEPS)

$(TARGETS) : $(OBJS)
	$(CC) -o $(TARGETS) $(OBJS) $(addprefix -l,$(LIBS)) $(addprefix -L,$(LIB_PATH))
