prjroot = .
#have prjname variable  all upper case to let keil + make work together
prjname =AS399XDEMO
builddir = $(prjroot)
libdir = $(builddir)/lib
objdir = $(builddir)/objects
sourcedir = .
includedir = .
ifndef keildir
keildir = $(wildcard /cygdrive/c/Keil)
endif

ALL_OBJECTS=\
STARTUP.obj                            \
uart.obj                               \
main.obj                               \
global.obj                             \
timer.obj                              \
F3xx_Blink_Control_F340.obj            \
F3xx_USB0_Standard_Requests.obj        \
F3xx_USB0_Descriptor.obj               \
F3xx_USB0_InterruptServiceRoutine.obj  \
F3xx_USB0_ReportHandler.obj            \
usb_commands.obj                       \
usb_commands_table.obj                 \
iso6b.obj                              \
bitbang.obj                            \
crc16.obj                              \
gen2.obj                               \
as399x.obj                             \
as399x_com.obj                         \
tuner.obj                              \
F340_FlashPrimitives.obj               \

CC = "$(keildir)"/C51/BIN/c51.exe
AS = "$(keildir)"/C51/BIN/a51.exe
LD = "$(keildir)"/C51/BIN/lx51.exe
HEXER = "$(keildir)"/C51/BIN/Ohx51.exe
HEX2BIN = objcopy -I ihex -O binary
MV = mv
AR = ar
GCC = gcc
SHELL = /bin/sh
CTAGS = ctags

# change ENTRY_POINT_ADDR to 0x1001 (or any other value whereas value%512 == 1) 
# in order to build a bootloader compatible image
ENTRY_POINT_ADDR = 0x1001
#ENTRY_POINT_ADDR = 0x0

CFLAGS = INTVECTOR\($(ENTRY_POINT_ADDR)\) LARGE OMF2 ROM\(COMPACT\) BROWSE VARBANKING DEBUG SYMBOLS CODE DEFINE\(ENTRY_POINT_ADDR=$(ENTRY_POINT_ADDR)\)
ASFLAGS = SET\(LARGE\) DEBUG EP DEFINE\(ENTRY_POINT_ADDR=$(ENTRY_POINT_ADDR)\)
LDFLAGS = CLASSES\(CODE\(C:$(ENTRY_POINT_ADDR)\), CONST\(C:$(ENTRY_POINT_ADDR)\), XDATA\(X:000000h-X:000fffh\)\) CODE PRINT\($(objdir)/$(prjname).map\) CASE DISABLEWARNING \(15, 16\) RESERVE \(I:0x002f.7-I:0x002f.7\) SEGMENTS\(\?STACK\(I:0x0080\)\)
GCFLAGS = -I$(keildir)/C51/INC -I$(includedir) -I$(sourcedir)

vpath %.c $(sourcedir)
vpath %.asm $(sourcedir)
vpath %.A51

all: app

ALL_OBJECTS := $(addprefix $(objdir)/,$(ALL_OBJECTS))
-include $(ALL_OBJECTS:.obj=.obj.d)


app: $(objdir)/$(prjname).bin

.PRECIOUS : $(objdir)/$(prjname)
$(objdir)/$(prjname) : $(ALL_OBJECTS)
	$(LD) `echo $(ALL_OBJECTS) | sed -e's/ /,/g'` TO $@ $(LDFLAGS)
	$(HEXER) $(objdir)/$(prjname)  HEXFILE \($(objdir)/$(prjname).hex\) 

.PRECIOUS : %.hex
%.hex: %
	$(HEXER) $<

%.bin: %.hex
	$(HEX2BIN) $< $@
 

	
.PHONY: ctags
ctags:
	$(CTAGS) -R

.PHONY: modules
modules: $(ALL_OBJECTS)

$(objdir)/%.obj: %.c $(objdir)/.dir
	$(CC) $< $(CFLAGS) PRINT\($(objdir)/$*.lst\) OBJECT\($@\)
	@$(GCC) -MM $(GCFLAGS) $< > $@.d
	@$(MV) $@.d $@.d.tmp
	@sed -e 's/.*:/$(subst /,\/,$(objdir))\/$*.obj:/' < $@.d.tmp > $@.d
	@rm -f $@.d.tmp

$(objdir)/%.obj: %.A51 $(objdir)/.dir
	$(AS) $< $(ASFLAGS) PRINT\($(objdir)/$*.lst\) OBJECT\($@\)

clean:
	rm -rf $(objdir) $(libdir)

# Mark .dir files as not being intermediate, otherwise it will be deleted and 
# subsequently recreated forcing rebuild of all oobjects
.PRECIOUS: %/.dir
%/.dir: 
	mkdir -p $*
	touch $@
