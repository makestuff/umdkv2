PLATFORM=nexys2
MODULE=businterface

all: synthesis simulation

synthesis: ${MODULE}.vhdl
	mkdir -p synthesis
	echo vhdl work "${MODULE}.vhdl" > synthesis/${MODULE}.prj
	echo "set -tmpdir \"xst/projnav.tmp\"" > synthesis/${PLATFORM}.xst
	echo "set -xsthdpdir \"xst\"" >> synthesis/${PLATFORM}.xst
	echo "run" >> synthesis/${PLATFORM}.xst
	echo "-ifn ${MODULE}.prj" >> synthesis/${PLATFORM}.xst
	echo "-ifmt mixed" >> synthesis/${PLATFORM}.xst
	echo "-ofn ${MODULE}" >> synthesis/${PLATFORM}.xst
	echo "-ofmt NGC" >> synthesis/${PLATFORM}.xst
	echo "-p xc3s1200e-4-fg320" >> synthesis/${PLATFORM}.xst
	echo "-top ${MODULE}" >> synthesis/${PLATFORM}.xst
	echo "-opt_mode Speed" >> synthesis/${PLATFORM}.xst
	echo "-opt_level 1" >> synthesis/${PLATFORM}.xst
	cp ${MODULE}.vhdl synthesis/
	mkdir -p synthesis/xst/projnav.tmp
	cd synthesis && xst -intstyle ise -ifn ${PLATFORM}.xst -ofn ${MODULE}.syr
	grep WARNINGS synthesis/* | grep -Eq "XST_NUMBER_OF_WARNINGS\" value=\"0\""
	touch $@

simulation: ${MODULE}.vhdl ${MODULE}_tb.vhdl
	mkdir -p simulation
	ghdl -i --ieee=synopsys --std=93c --vital-checks --warn-binding --warn-reserved --warn-library --warn-vital-generic --warn-delayed-checks --warn-body --warn-specs --warn-unused --warn-error --workdir=simulation --work=work *.vhdl
	ghdl -m --ieee=synopsys --std=93c --vital-checks --warn-binding --warn-reserved --warn-library --warn-vital-generic --warn-delayed-checks --warn-body --warn-specs --warn-unused --warn-error --workdir=simulation --work=work ${MODULE}_tb
	mv ${MODULE}_tb simulation
	./simulation/${MODULE}_tb --stop-time=500ns --vcd=simulation/${MODULE}_tb.vcd
	touch $@

waves: simulation
	echo "set signalList [list]" > simulation/startup.tcl
	echo "lappend signalList \"uut.reset_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.clk_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdAddr_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdAccessMem_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdAccessIO_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdOE_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdSyncOE_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdBeginRead_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdLDSW_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdSyncWE_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdBeginWrite_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdData_in\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdData_io\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdBufOE_out\"" >> simulation/startup.tcl
	echo "lappend signalList \"uut.mdBufDir_out\"" >> simulation/startup.tcl
	echo "set num_added [ gtkwave::addSignalsFromList \$$signalList ]" >> simulation/startup.tcl
	echo "gtkwave::setZoomFactor -26" >> simulation/startup.tcl
	echo "gtkwave::setMarker 55ns" >> simulation/startup.tcl
	gtkwave -T simulation/startup.tcl simulation/${MODULE}_tb.vcd

clean: FORCE
	rm -rf synthesis simulation

FORCE:
