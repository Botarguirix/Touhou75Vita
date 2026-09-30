VITASDK ?= $(shell printf '%s' "$$VITASDK")
PREFIX  ?= arm-vita-eabi

ifneq ($(wildcard $(VITASDK)/bin/arm-vita-eabi-gcc),)
include $(VITASDK)/share/vita_rules
else
$(error VitaSDK not found. Set VITASDK and install arm-vita-eabi-gcc/vita_rules)
endif

TARGET = iamp_vita
OBJS = src/main.o ../runtime/iamp_semantic_runtime.o ../generated/guest_semantic.o
CFLAGS = -O2 -Wall -Wextra -I../runtime
LIBS = -lm

all: $(TARGET).vpk

$(TARGET).velf: $(OBJS)
	$(LINK.o) $^ $(LIBS) -o $@

$(TARGET).self: $(TARGET).velf
	vita-make-fself $< $@

PARAM.SFO:
	vita-mksfoex -s TITLE_ID=IAMP075 -s TITLE="Immaterial and Missing Power" "Immaterial and Missing Power" $@

$(TARGET).vpk: $(TARGET).self PARAM.SFO
	vita-pack-vpk -s PARAM.SFO -b $(TARGET).self $@

clean:
	$(RM) $(OBJS) $(TARGET).velf $(TARGET).self $(TARGET).vpk PARAM.SFO

.PHONY: all clean
