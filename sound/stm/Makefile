ccflags-$(CONFIG_SND_DEBUG)  := -DDEBUG
ccflags-$(CONFIG_SND_DEBUG_VERBOSE) += -DVERBOSE_DEBUG

# Core

obj-$(CONFIG_SND_STM) += snd-stm-core.o
snd-stm-core-y := core.o clock.o conv.o

obj-$(CONFIG_SND_STM_PCM_PLAYER) += snd-stm-pcm-player.o
snd-stm-pcm-player-y := pcm_player.o

obj-$(CONFIG_SND_STM_PCM_READER) += snd-stm-pcm-reader.o
snd-stm-pcm-reader-y := pcm_reader.o

obj-$(CONFIG_SND_STM_SPDIF_PLAYER) += snd-stm-spdif-player.o
snd-stm-spdif-player-y := spdif_player.o

obj-$(CONFIG_SND_STM_UNIPERIF_PLAYER) += snd-stm-uniperif-player.o
snd-stm-uniperif-player-y := uniperif_player.o

obj-$(CONFIG_SND_STM_UNIPERIF_READER) += snd-stm-uniperif-reader.o
snd-stm-uniperif-reader-y := uniperif_reader.o

obj-$(CONFIG_SND_STM_CONV_I2S_TO_SPDIF) += snd-stm-conv-i2sspdif.o
snd-stm-conv-i2sspdif-y := conv_i2sspdif.o

obj-$(CONFIG_SND_STM_CONV_DAC_MEMORY_MAPPED) += snd-stm-conv-dac-mem.o
snd-stm-conv-dac-mem-y := conv_dac_mem.o

obj-$(CONFIG_SND_STM_CONV_DAC_SYSCONF) += snd-stm-conv-dac-sc.o
snd-stm-conv-dac-sc-y := conv_dac_sc.o



# SOCs

obj-$(CONFIG_SND_STM_FLI7610) += snd-stm-fli7610.o
snd-stm-fli7610-y := fli7610.o

obj-$(CONFIG_SND_STM_STX7108) += snd-stm-stx7108.o
snd-stm-stx7108-y := stx7108.o

obj-$(CONFIG_SND_STM_STIH415) += snd-stm-stih415.o
snd-stm-stih415-y := stih415.o



# External converters

obj-$(CONFIG_SND_STM_CONV_BIPHASE) += snd-stm-conv-biphase.o
snd-stm-conv-biphase-y := conv_biphase.o

obj-$(CONFIG_SND_STM_CONV_DUMMY) += snd-stm-conv-dummy.o
snd-stm-conv-dummy-y := conv_dummy.o

obj-$(CONFIG_SND_STM_CONV_EPLD) += snd-stm-conv-epld.o
snd-stm-conv-epld-y := conv_epld.o

obj-$(CONFIG_SND_STM_CONV_GPIO) += snd-stm-conv-gpio.o
snd-stm-conv-gpio-y := conv_gpio.o

obj-$(CONFIG_SND_STM_CONV_I2C) += snd-stm-conv-i2c.o
snd-stm-conv-i2c-y := conv_i2c.o

obj-$(CONFIG_SND_STM_CONV_SPDIF_RX) += snd-stm-conv-spdif-rx.o
snd-stm-conv-spdif-rx-y := conv_spdif_rx.o
