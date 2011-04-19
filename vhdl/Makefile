TOP_LEVEL = TopLevel
PLATFORM = nexys2
VHDLS = \
	$(TOP_LEVEL).vhdl \
	ClockGenerator.vhdl \
	MemoryController.vhdl \
	businterface/businterface.vhdl \
	serialio/serialio.vhdl \
	sevenseg/sevenseg.vhdl

ifeq ($(PLATFORM),s3board)
	FPGA = xc3s200-ft256-4
endif
ifeq ($(PLATFORM),nexys2)
	FPGA = xc3s1200e-fg320-4
endif

all: $(TOP_LEVEL).bit

load: $(TOP_LEVEL).bit
	sudo xilprg -c nero:04B4:8613 -b TopLevel.bit

xsvf: $(TOP_LEVEL).xsvf

csvf: $(TOP_LEVEL).csvf

report: $(TOP_LEVEL).twr

$(TOP_LEVEL).csvf: $(TOP_LEVEL).xsvf
	xsvf2csvf $< $@

# This assumes that the "XILINX" environment variable is set 
$(TOP_LEVEL).xsvf: platforms/$(PLATFORM)/$(PLATFORM).batch $(TOP_LEVEL).bit
	cat $< | sed s#\$${XILINX}#$(subst \,/,$(XILINX))#g > temp.batch
	impact -batch temp.batch
	rm -f temp.batch

$(TOP_LEVEL).bit: platforms/$(PLATFORM)/$(PLATFORM).ut $(TOP_LEVEL).ncd
	bitgen -intstyle ise -f $+

$(TOP_LEVEL).twr: $(TOP_LEVEL).ncd platforms/$(PLATFORM)/$(PLATFORM).ucf
	trce -intstyle ise -v 3 -s 4 -n 3 -fastpaths -xml ${TOP_LEVEL}.twx $< -o $@ ${TOP_LEVEL}.pcf -ucf platforms/$(PLATFORM)/$(PLATFORM).ucf

$(TOP_LEVEL).ncd: $(TOP_LEVEL)_map.ncd
	par -w -intstyle ise -ol high -t 1 $< $@ ${TOP_LEVEL}.pcf

$(TOP_LEVEL)_map.ncd: $(TOP_LEVEL).ngd
	map -intstyle ise -p $(FPGA) -cm area -ir off -pr off -c 100 -o $@ $< ${TOP_LEVEL}.pcf

$(TOP_LEVEL).ngd: $(TOP_LEVEL).ngc platforms/$(PLATFORM)/$(PLATFORM).ucf
	ngdbuild -intstyle ise -dd _ngo -nt timestamp -uc platforms/$(PLATFORM)/$(PLATFORM).ucf -p $(FPGA) $< $@

$(TOP_LEVEL).ngc: platforms/$(PLATFORM)/$(PLATFORM).xst $(TOP_LEVEL).prj
	mkdir -p xst/projnav.tmp
	xst -intstyle ise -ifn $< -ofn $(TOP_LEVEL).syr

$(TOP_LEVEL).prj: $(VHDLS)
	for i in $+; do echo "vhdl work \"$$i\""; done > $@

clean: FORCE
	rm -rf *.xsvf *.csvf _ngo *.bgn *.drc *.ncd *.ntrc_log *.prj *.twr *.csv *.html fx2fpga_xdb _xmsgs *.bit *.gise *.ngc *.pad *.ptwx *.twx *.ngm *.txt *.xml *.xrpt *.bld *.ise *.ngd *.par *.stx *.map *.twr auto_project_xdb *.cmd_log *.lso *.ngr *.pcf *.syr *.unroutes *.xpi *.mrp xst *.log *.cmd *.xwbt iseconfig xlnx_auto_0_xdb

FORCE:
