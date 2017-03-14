targets	:= sim.x
objs	:= main.o gel.o queue.o

CC	:= gcc
CFLAGS	:= -Werror
CFLAGS	+= -g

ifneq ($(V),1)
Q = @
endif

deps := $(patsubst %.o,%.d,$objs)
-include $(deps)

all: $(targets)

%.x: $(objs)
	@echo "LD $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF $(@:.o=.d)

clean:
	@echo "clean"
	$(Q)rm -f $(targets) $(objs) $(deps)