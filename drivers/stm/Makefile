#
# Makefile for the STMicroelectronics device drivers.
#

obj-y					+= soc.o
obj-y					+= emi.o
obj-y					+= gpio.o
obj-y					+= pad.o
obj-y					+= sysconf.o
obj-y					+= device.o
obj-y					+= clk.o
obj-y					+= clocks/

obj-$(CONFIG_STM_MIPHY)			+= miphy.o
obj-$(CONFIG_STM_MIPHY365X)		+= miphy365x.o
obj-$(CONFIG_STM_MIPHYA40X)		+= miphya40x.o
obj-$(CONFIG_STM_MIPHY_TAP)		+= miphy_tap.o tap.o
obj-$(CONFIG_STM_MIPHY_PCIE_MP)		+= miphy_pcie_mp.o
obj-$(CONFIG_STM_MIPHY_DUMMY)		+= miphy_dummy.o
obj-$(CONFIG_STM_COPROCESSOR_CLASS)	+= coprocessor.o
obj-$(CONFIG_STM_COPROCESSOR_ST40)	+= coproc-st40.o
obj-$(CONFIG_STM_COPROCESSOR_ST200)	+= coproc-st200.o
obj-$(CONFIG_STM_RNG)			+= rng.o
###obj-$(CONFIG_STM_MALI)			+= ump/ mali/
obj-$(CONFIG_STM_AMBA_BRIDGE)		+= amba_bridge.o

obj-$(CONFIG_PM)			+= wakeup_devices.o
obj-$(CONFIG_PM)			+= pm_notify.o

obj-$(CONFIG_STM_LPM)			+= lpm_com.o lpm_mb.o

obj-$(CONFIG_STM_PCI_EMISS)		+= pci-emiss.o
obj-$(CONFIG_STM_PCIE)			+= pcie.o

obj-$(CONFIG_STM_IRQ_MUX)		+= irqmux.o

obj-$(CONFIG_CPU_SUBTYPE_FLI7610)	+= fli7610.o fli7610_clock.o fli7610_audio.o pio-control.o \
					   fli7610_usb.o fli7610_comms.o mpe41_devices.o \
					   fli7610_hispeed.o fli7610_pci.o
obj-$(CONFIG_CPU_SUBTYPE_STX7108)	+= stx7108.o stx7108_clock.o stx7108_audio.o stx7108_comms.o stx7108_hispeed.o stx7108_pci.o pio-control.o
obj-$(CONFIG_CPU_SUBTYPE_STXH205)	+= stxh205.o stxh205_clock.o stxh205_comms.o stxh205_hispeed.o pio-control.o
obj-$(CONFIG_MACH_STM_STIG125)		+= pio-control.o stig125/
obj-$(CONFIG_CPU_SUBTYPE_STIH415)	+= stih415.o stih415_clock.o stih415_audio.o stih415_hispeed.o stih415_usb.o \
					   stih415_comms.o stih415_pci.o pio-control.o mpe41_devices.o stih415_lpm.o
