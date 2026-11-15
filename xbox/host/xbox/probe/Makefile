# the memory map's probe: an XBE that reports, over the serial port, which of
# the console's addresses the port can map on the Xbox. see main.c
XBE_TITLE = probe
GEN_XISO = $(XBE_TITLE).iso
SRCS = $(CURDIR)/main.c
NXDK_DIR ?= $(HOME)/refs/nxdk

include $(NXDK_DIR)/Makefile
