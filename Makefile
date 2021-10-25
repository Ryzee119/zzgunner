SOURCE_DIR = $(CURDIR)
BUILD_DIR = build
include $(N64_INST)/include/n64.mk

N64_ROM_TITLE = zzgunner
N64_ROM_SAVETYPE = sram256k
N64_ROM_REGIONFREE = true

ASSETS_MUSIC = $(wildcard assets/music/*.xm)
ASSETS_MUSIC_CONV = $(addprefix filesystem/music/,$(notdir $(ASSETS_MUSIC:%.xm=%.xm64)))

ASSETS_SFX = $(wildcard assets/sfx/*.wav)
ASSETS_SFX_CONV = $(addprefix filesystem/sfx/,$(notdir $(ASSETS_SFX:%.wav=%.wav64)))

N64_CFLAGS += -Wno-error #Disable -Werror from n64.mk

CFLAGS += \
	-I$(SOURCE_DIR) \
	-Ilib/n64/ugfx \
	-Ilib/cute_headers \
	-Ilib

SRCS = \
	lib/n64/ugfx/ugfx.c \
	main.c \
	sprite.c \
	video_n64.c \
	sound_n64.c \
	player.c \
	weapon.c \
	map.c \
	camera.c \
	gui.c \
	enemy.c

all: $(N64_ROM_TITLE).z64

filesystem/music/%.xm64: assets/music/%.xm
	@mkdir -p $(dir $@)
	@echo "    [AUDIO - MUSIC] $@"
	@$(N64_AUDIOCONV) -o filesystem/music $<

filesystem/sfx/%.wav64: assets/sfx/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO - SFX] $@"
	@$(N64_AUDIOCONV) -o filesystem/sfx $<

$(BUILD_DIR)/$(N64_ROM_TITLE).dfs: $(wildcard filesystem/*) $(ASSETS_MUSIC_CONV) $(ASSETS_SFX_CONV)
$(BUILD_DIR)/$(N64_ROM_TITLE).elf: $(SRCS:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/lib/n64/ugfx/rsp_ugfx.o

$(N64_ROM_TITLE).z64: N64_ROM_TITLE="Gunner64"
$(N64_ROM_TITLE).z64: $(BUILD_DIR)/$(N64_ROM_TITLE).dfs

clean:
	rm -f $(BUILD_DIR)/* $(N64_ROM_TITLE).z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
