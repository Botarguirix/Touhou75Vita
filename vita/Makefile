VITASDK ?= $(shell printf '%s' "$$VITASDK")
PREFIX  ?= arm-vita-eabi

ifneq ($(wildcard $(VITASDK)/bin/arm-vita-eabi-gcc),)
include $(VITASDK)/share/vita_rules
else
$(error VitaSDK not found. Set VITASDK and install arm-vita-eabi-gcc/vita_rules)
endif

PROJECT = touhou75_vita
PROJECT_TITLE = Touhou 7.5 Vita - Diagnostic
PROJECT_TITLEID = T075VITA1
OBJS = src/main.o
CFLAGS += -O2 -Wall -Wextra
LIBS += -lSceIofilemgr_stub

all: $(PROJECT).vpk

$(PROJECT).velf: $(OBJS)
	$(LINK.o) $^ $(LIBS) -o $@

$(PROJECT).self: $(PROJECT).velf
	vita-make-fself $< $@

PARAM.SFO:
	vita-mksfoex -s TITLE_ID=$(PROJECT_TITLEID) "$(PROJECT_TITLE)" $@

$(PROJECT).vpk: $(PROJECT).self PARAM.SFO
	vita-pack-vpk -s PARAM.SFO -b $(PROJECT).self $@

clean:
	$(RM) $(OBJS) $(PROJECT).velf $(PROJECT).self $(PROJECT).vpk PARAM.SFO

.PHONY: all clean
