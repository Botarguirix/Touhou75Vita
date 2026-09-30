BUILD_DIR ?= build
CMAKE ?= cmake
PROJECT ?= touhou75_vita

all:
	$(CMAKE) -S . -B $(BUILD_DIR)
	$(CMAKE) --build $(BUILD_DIR) --parallel
	$(CMAKE) -E copy $(BUILD_DIR)/$(PROJECT).vpk $(PROJECT).vpk

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
	$(CMAKE) -E rm -f $(PROJECT).vpk

.PHONY: all clean
