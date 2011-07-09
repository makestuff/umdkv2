#!/bin/bash
#
# Copyright (C) 2011 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
if [ ! -e gdb-7.2.tar.bz2 ]; then
    wget http://ftp.gnu.org/gnu/gdb/gdb-7.2.tar.bz2
fi
rm -rf gdb-7.2 gdb-7.2-ms1
bunzip2 -c gdb-7.2.tar.bz2 | tar xf -
mv gdb-7.2 gdb-7.2-ms1

patch gdb-7.2-ms1/gdb/remote.c <<EOF
--- remote.c.old	2011-06-26 20:06:36.704386001 +0100
+++ remote.c.new	2011-06-26 20:29:14.872386002 +0100
@@ -6840 +6840 @@
-	      if (tcount > 3)
+	      //if (tcount > 3)
EOF

patch gdb-7.2-ms1/gdb/m68k-tdep.c <<EOF
--- gdb-7.2-old/gdb/m68k-tdep.c	2010-01-01 07:31:37.000000000 +0000
+++ gdb-7.2-new/gdb/m68k-tdep.c	2011-06-12 19:23:10.917109001 +0100
@@ -168 +168 @@
-    "ps", "pc",
+    "sr", "pc",
EOF

patch gdb-7.2-ms1/gdb/regformats/reg-m68k.dat <<EOF
--- gdb-7.2-old/gdb/regformats/reg-m68k.dat	2003-01-04 21:55:53.000000000 +0000
+++ gdb-7.2-new/gdb/regformats/reg-m68k.dat	2011-06-12 19:23:29.597109001 +0100
@@ -19 +19 @@
-32:ps
+32:sr
EOF

patch gdb-7.2-ms1/opcodes/m68k-dis.c <<EOF
--- gdb-7.2-old/opcodes/m68k-dis.c	2010-06-16 16:12:49.000000000 +0100
+++ gdb-7.2-new/opcodes/m68k-dis.c	2011-06-13 21:14:07.389109000 +0100
@@ -41,3 +41,3 @@
-  "%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
-  "%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%fp", "%sp",
-  "%ps", "%pc"
+  "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
+  "a0", "a1", "a2", "a3", "a4", "a5", "fp", "sp",
+  "sr", "pc"
@@ -500 +499,0 @@
-      (*info->fprintf_func) (info->stream, "%%pc@(");
@@ -501,0 +501 @@
+      (*info->fprintf_func) (info->stream, "(pc");
@@ -507 +507,2 @@
-      if (regno == -2)
+      if (regno == -2) {
+	// No base register
@@ -509 +510,4 @@
-      else if (regno == -3)
+	sprintf_vma (buf, disp);
+	(*info->fprintf_func) (info->stream, "%s", buf);
+      } else if (regno == -3) {
+	// ZPC base reg
@@ -511,5 +515,6 @@
-      else
-	(*info->fprintf_func) (info->stream, "%s@(", reg_names[regno]);
-
-      sprintf_vma (buf, disp);
-      (*info->fprintf_func) (info->stream, "%s", buf);
+	sprintf_vma (buf, disp);
+	(*info->fprintf_func) (info->stream, "%s", buf);
+      } else {
+	// Some other base reg
+	(*info->fprintf_func) (info->stream, "%d(%s", disp, reg_names[regno]);
+      }
@@ -531 +536 @@
-  static char *const scales[] = { "", ":2", ":4", ":8" };
+  static char *const scales[] = { "", "*2", "*4", "*8" };
@@ -541 +546 @@
-  sprintf (buf, "%s:%c%s",
+  sprintf (buf, "%s.%c%s",
@@ -556 +561 @@
-      (*info->fprintf_func) (info->stream, ",%s)", buf);
+      (*info->fprintf_func) (info->stream, ", %s)", buf);
@@ -588 +593 @@
-	(*info->fprintf_func) (info->stream, ",%s", buf);
+	(*info->fprintf_func) (info->stream, ", %s", buf);
@@ -607 +612 @@
-      (*info->fprintf_func) (info->stream, ",%s", buf);
+      (*info->fprintf_func) (info->stream, ", %s", buf);
@@ -613 +618 @@
-    (*info->fprintf_func) (info->stream, ",%s", buf);
+    (*info->fprintf_func) (info->stream, ", %s", buf);
@@ -676 +681 @@
-      (*info->fprintf_func) (info->stream, "%%ccr");
+      (*info->fprintf_func) (info->stream, "ccr");
@@ -680 +685 @@
-      (*info->fprintf_func) (info->stream, "%%sr");
+      (*info->fprintf_func) (info->stream, "sr");
@@ -684 +689 @@
-      (*info->fprintf_func) (info->stream, "%%usp");
+      (*info->fprintf_func) (info->stream, "usp");
@@ -688 +693 @@
-      (*info->fprintf_func) (info->stream, "%%acc");
+      (*info->fprintf_func) (info->stream, "acc");
@@ -692 +697 @@
-      (*info->fprintf_func) (info->stream, "%%macsr");
+      (*info->fprintf_func) (info->stream, "macsr");
@@ -696 +701 @@
-      (*info->fprintf_func) (info->stream, "%%mask");
+      (*info->fprintf_func) (info->stream, "mask");
@@ -707,8 +712,8 @@
-	    {"%sfc", 0x000}, {"%dfc", 0x001}, {"%cacr", 0x002},
-	    {"%tc",  0x003}, {"%itt0",0x004}, {"%itt1", 0x005},
-	    {"%dtt0",0x006}, {"%dtt1",0x007}, {"%buscr",0x008},
-	    {"%rgpiobar", 0x009}, {"%acr4",0x00c},
-	    {"%acr5",0x00d}, {"%acr6",0x00e}, {"%acr7", 0x00f},
-	    {"%usp", 0x800}, {"%vbr", 0x801}, {"%caar", 0x802},
-	    {"%msp", 0x803}, {"%isp", 0x804},
-	    {"%pc", 0x80f},
+	    {"sfc", 0x000}, {"dfc", 0x001}, {"cacr", 0x002},
+	    {"tc",  0x003}, {"itt0",0x004}, {"itt1", 0x005},
+	    {"dtt0",0x006}, {"dtt1",0x007}, {"buscr",0x008},
+	    {"rgpiobar", 0x009}, {"acr4",0x00c},
+	    {"acr5",0x00d}, {"acr6",0x00e}, {"acr7", 0x00f},
+	    {"usp", 0x800}, {"vbr", 0x801}, {"caar", 0x802},
+	    {"msp", 0x803}, {"ssp", 0x804},
+	    {"pc", 0x80f},
@@ -717 +722 @@
-	    {"%rambar0", 0xc04}, {"%rambar1", 0xc05},
+	    {"rambar0", 0xc04}, {"rambar1", 0xc05},
@@ -721 +726 @@
-	    {"%mbar0", 0xc0e}, {"%mbar1", 0xc0f},
+	    {"mbar0", 0xc0e}, {"mbar1", 0xc0f},
@@ -724 +729 @@
-	    {"%mmusr",0x805},
+	    {"mmusr",0x805},
@@ -726 +731 @@
-	    {"%urp", 0x806}, {"%srp", 0x807}, {"%pcr", 0x808},
+	    {"urp", 0x806}, {"srp", 0x807}, {"pcr", 0x808},
@@ -729 +734 @@
-	    {"%cac", 0xffe}, {"%mbo", 0xfff}
+	    {"cac", 0xffe}, {"mbo", 0xfff}
@@ -734,2 +739,2 @@
-	    {"%asid",0x003}, {"%acr0",0x004}, {"%acr1",0x005},
-	    {"%acr2",0x006}, {"%acr3",0x007}, {"%mmubar",0x008},
+	    {"asid",0x003}, {"acr0",0x004}, {"acr1",0x005},
+	    {"acr2",0x006}, {"acr3",0x007}, {"mmubar",0x008},
@@ -828,4 +833 @@
-      if (regno > 7)
-	(*info->fprintf_func) (info->stream, "%s@", reg_names[regno]);
-      else
-	(*info->fprintf_func) (info->stream, "@(%s)", reg_names[regno]);
+      (*info->fprintf_func) (info->stream, "(%s)", reg_names[regno]);
@@ -836 +838 @@
-      (*info->fprintf_func) (info->stream, "%%fp%d", val);
+      (*info->fprintf_func) (info->stream, "fp%d", val);
@@ -849 +851 @@
-      (*info->fprintf_func) (info->stream, "%s@+", reg_names[val + 8]);
+      (*info->fprintf_func) (info->stream, "(%s)+", reg_names[val + 8]);
@@ -854 +856 @@
-      (*info->fprintf_func) (info->stream, "%s@-", reg_names[val + 8]);
+      (*info->fprintf_func) (info->stream, "-(%s)", reg_names[val + 8]);
@@ -933 +935 @@
-	(*info->fprintf_func) (info->stream, "%s@(%d)", reg_names[val1 + 8], val);
+	(*info->fprintf_func) (info->stream, "%d(%s)", val, reg_names[val1 + 8]);
@@ -944 +946 @@
-      (*info->fprintf_func) (info->stream, "%%acc%d", val);
+      (*info->fprintf_func) (info->stream, "acc%d", val);
@@ -949 +951 @@
-      (*info->fprintf_func) (info->stream, "%%accext%s", val == 0 ? "01" : "23");
+      (*info->fprintf_func) (info->stream, "accext%s", val == 0 ? "01" : "23");
@@ -1027 +1029 @@
-	  (*info->fprintf_func) (info->stream, "%s@", regname);
+	  (*info->fprintf_func) (info->stream, "(%s)", regname);
@@ -1031 +1033 @@
-	  (*info->fprintf_func) (info->stream, "%s@+", regname);
+	  (*info->fprintf_func) (info->stream, "(%s)+", regname);
@@ -1035 +1037 @@
-	  (*info->fprintf_func) (info->stream, "%s@-", regname);
+	  (*info->fprintf_func) (info->stream, "-(%s)", regname);
@@ -1040 +1042 @@
-	  (*info->fprintf_func) (info->stream, "%s@(%d)", regname, val);
+	  (*info->fprintf_func) (info->stream, "%d(%s)", val, regname);
@@ -1064 +1065,0 @@
-	      (*info->fprintf_func) (info->stream, "%%pc@(");
@@ -1066 +1067 @@
-	      (*info->fprintf_func) (info->stream, ")");
+	      (*info->fprintf_func) (info->stream, "(pc)");
@@ -1207 +1208 @@
-		  (*info->fprintf_func) (info->stream, "%%fp%d", regno);
+		  (*info->fprintf_func) (info->stream, "fp%d", regno);
@@ -1212 +1213 @@
-		    (*info->fprintf_func) (info->stream, "-%%fp%d", regno);
+		    (*info->fprintf_func) (info->stream, "-fp%d", regno);
@@ -1258 +1259 @@
-		(info->stream, val == 0x1c ? "%%bad%d" : "%%bac%d",
+		(info->stream, val == 0x1c ? "bad%d" : "bac%d",
@@ -1276 +1277 @@
-	  (*info->fprintf_func) (info->stream, "%%dfc");
+	  (*info->fprintf_func) (info->stream, "dfc");
@@ -1278 +1279 @@
-	  (*info->fprintf_func) (info->stream, "%%sfc");
+	  (*info->fprintf_func) (info->stream, "sfc");
@@ -1286 +1287 @@
-      (*info->fprintf_func) (info->stream, "%%val");
+      (*info->fprintf_func) (info->stream, "val");
@@ -1463 +1464 @@
-	info->fprintf_func (info->stream, ",");
+	info->fprintf_func (info->stream, ", ");
EOF

patch gdb-7.2-ms1/opcodes/m68k-opc.c <<EOF
--- gdb-7.2-old/opcodes/m68k-opc.c	2010-06-16 17:27:36.000000000 +0100
+++ gdb-7.2-new/opcodes/m68k-opc.c	2011-06-12 15:23:27.169109001 +0100
@@ -42,2 +42,2 @@
-{"addaw", 2,	one(0150300),	one(0170700), "*wAd", m68000up },
-{"addal", 2,	one(0150700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"adda.w", 2,	one(0150300),	one(0170700), "*wAd", m68000up },
+{"adda.l", 2,	one(0150700),	one(0170700), "*lAd", m68000up | mcfisa_a },
@@ -45,8 +45,8 @@
-{"addib", 4,	one(0003000),	one(0177700), "#b\$s", m68000up },
-{"addiw", 4,	one(0003100),	one(0177700), "#w\$s", m68000up },
-{"addil", 6,	one(0003200),	one(0177700), "#l\$s", m68000up },
-{"addil", 6,	one(0003200),	one(0177700), "#lDs", mcfisa_a },
-
-{"addqb", 2,	one(0050000),	one(0170700), "Qd\$b", m68000up },
-{"addqw", 2,	one(0050100),	one(0170700), "Qd%w", m68000up },
-{"addql", 2,	one(0050200),	one(0170700), "Qd%l", m68000up | mcfisa_a },
+{"addi.b", 4,	one(0003000),	one(0177700), "#b\$s", m68000up },
+{"addi.w", 4,	one(0003100),	one(0177700), "#w\$s", m68000up },
+{"addi.l", 6,	one(0003200),	one(0177700), "#l\$s", m68000up },
+{"addi.l", 6,	one(0003200),	one(0177700), "#lDs", mcfisa_a },
+
+{"addq.b", 2,	one(0050000),	one(0170700), "Qd\$b", m68000up },
+{"addq.w", 2,	one(0050100),	one(0170700), "Qd%w", m68000up },
+{"addq.l", 2,	one(0050200),	one(0170700), "Qd%l", m68000up | mcfisa_a },
@@ -55,29 +55,29 @@
-{"addb", 2,	one(0050000),	one(0170700), "Qd\$b", m68000up },
-{"addb", 4,	one(0003000),	one(0177700), "#b\$s", m68000up },
-{"addb", 2,	one(0150000),	one(0170700), ";bDd", m68000up },
-{"addb", 2,	one(0150400),	one(0170700), "Dd~b", m68000up },
-{"addw", 2,	one(0050100),	one(0170700), "Qd%w", m68000up },
-{"addw", 2,	one(0150300),	one(0170700), "*wAd", m68000up },
-{"addw", 4,	one(0003100),	one(0177700), "#w\$s", m68000up },
-{"addw", 2,	one(0150100),	one(0170700), "*wDd", m68000up },
-{"addw", 2,	one(0150500),	one(0170700), "Dd~w", m68000up },
-{"addl", 2,	one(0050200),	one(0170700), "Qd%l", m68000up | mcfisa_a },
-{"addl", 6,	one(0003200),	one(0177700), "#l\$s", m68000up },
-{"addl", 6,	one(0003200),	one(0177700), "#lDs", mcfisa_a },
-{"addl", 2,	one(0150700),	one(0170700), "*lAd", m68000up | mcfisa_a },
-{"addl", 2,	one(0150200),	one(0170700), "*lDd", m68000up | mcfisa_a },
-{"addl", 2,	one(0150600),	one(0170700), "Dd~l", m68000up | mcfisa_a },
-
-{"addxb", 2,	one(0150400),	one(0170770), "DsDd", m68000up },
-{"addxb", 2,	one(0150410),	one(0170770), "-s-d", m68000up },
-{"addxw", 2,	one(0150500),	one(0170770), "DsDd", m68000up },
-{"addxw", 2,	one(0150510),	one(0170770), "-s-d", m68000up },
-{"addxl", 2,	one(0150600),	one(0170770), "DsDd", m68000up | mcfisa_a },
-{"addxl", 2,	one(0150610),	one(0170770), "-s-d", m68000up },
-
-{"andib", 4,	one(0001000),	one(0177700), "#b\$s", m68000up },
-{"andib", 4,	one(0001074),	one(0177777), "#bCs", m68000up },
-{"andiw", 4,	one(0001100),	one(0177700), "#w\$s", m68000up },
-{"andiw", 4,	one(0001174),	one(0177777), "#wSs", m68000up },
-{"andil", 6,	one(0001200),	one(0177700), "#l\$s", m68000up },
-{"andil", 6,	one(0001200),	one(0177700), "#lDs", mcfisa_a },
+{"add.b", 2,	one(0050000),	one(0170700), "Qd\$b", m68000up },
+{"add.b", 4,	one(0003000),	one(0177700), "#b\$s", m68000up },
+{"add.b", 2,	one(0150000),	one(0170700), ";bDd", m68000up },
+{"add.b", 2,	one(0150400),	one(0170700), "Dd~b", m68000up },
+{"add.w", 2,	one(0050100),	one(0170700), "Qd%w", m68000up },
+{"add.w", 2,	one(0150300),	one(0170700), "*wAd", m68000up },
+{"add.w", 4,	one(0003100),	one(0177700), "#w\$s", m68000up },
+{"add.w", 2,	one(0150100),	one(0170700), "*wDd", m68000up },
+{"add.w", 2,	one(0150500),	one(0170700), "Dd~w", m68000up },
+{"add.l", 2,	one(0050200),	one(0170700), "Qd%l", m68000up | mcfisa_a },
+{"add.l", 6,	one(0003200),	one(0177700), "#l\$s", m68000up },
+{"add.l", 6,	one(0003200),	one(0177700), "#lDs", mcfisa_a },
+{"add.l", 2,	one(0150700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"add.l", 2,	one(0150200),	one(0170700), "*lDd", m68000up | mcfisa_a },
+{"add.l", 2,	one(0150600),	one(0170700), "Dd~l", m68000up | mcfisa_a },
+
+{"addx.b", 2,	one(0150400),	one(0170770), "DsDd", m68000up },
+{"addx.b", 2,	one(0150410),	one(0170770), "-s-d", m68000up },
+{"addx.w", 2,	one(0150500),	one(0170770), "DsDd", m68000up },
+{"addx.w", 2,	one(0150510),	one(0170770), "-s-d", m68000up },
+{"addx.l", 2,	one(0150600),	one(0170770), "DsDd", m68000up | mcfisa_a },
+{"addx.l", 2,	one(0150610),	one(0170770), "-s-d", m68000up },
+
+{"andi.b", 4,	one(0001000),	one(0177700), "#b\$s", m68000up },
+{"andi.b", 4,	one(0001074),	one(0177777), "#bCs", m68000up },
+{"andi.w", 4,	one(0001100),	one(0177700), "#w\$s", m68000up },
+{"andi.w", 4,	one(0001174),	one(0177777), "#wSs", m68000up },
+{"andi.l", 6,	one(0001200),	one(0177700), "#l\$s", m68000up },
+{"andi.l", 6,	one(0001200),	one(0177700), "#lDs", mcfisa_a },
@@ -89,12 +89,12 @@
-{"andb", 4,	one(0001000),	one(0177700), "#b\$s", m68000up },
-{"andb", 4,	one(0001074),	one(0177777), "#bCs", m68000up },
-{"andb", 2,	one(0140000),	one(0170700), ";bDd", m68000up },
-{"andb", 2,	one(0140400),	one(0170700), "Dd~b", m68000up },
-{"andw", 4,	one(0001100),	one(0177700), "#w\$s", m68000up },
-{"andw", 4,	one(0001174),	one(0177777), "#wSs", m68000up },
-{"andw", 2,	one(0140100),	one(0170700), ";wDd", m68000up },
-{"andw", 2,	one(0140500),	one(0170700), "Dd~w", m68000up },
-{"andl", 6,	one(0001200),	one(0177700), "#l\$s", m68000up },
-{"andl", 6,	one(0001200),	one(0177700), "#lDs", mcfisa_a },
-{"andl", 2,	one(0140200),	one(0170700), ";lDd", m68000up | mcfisa_a },
-{"andl", 2,	one(0140600),	one(0170700), "Dd~l", m68000up | mcfisa_a },
+{"and.b", 4,	one(0001000),	one(0177700), "#b\$s", m68000up },
+{"and.b", 4,	one(0001074),	one(0177777), "#bCs", m68000up },
+{"and.b", 2,	one(0140000),	one(0170700), ";bDd", m68000up },
+{"and.b", 2,	one(0140400),	one(0170700), "Dd~b", m68000up },
+{"and.w", 4,	one(0001100),	one(0177700), "#w\$s", m68000up },
+{"and.w", 4,	one(0001174),	one(0177777), "#wSs", m68000up },
+{"and.w", 2,	one(0140100),	one(0170700), ";wDd", m68000up },
+{"and.w", 2,	one(0140500),	one(0170700), "Dd~w", m68000up },
+{"and.l", 6,	one(0001200),	one(0177700), "#l\$s", m68000up },
+{"and.l", 6,	one(0001200),	one(0177700), "#lDs", mcfisa_a },
+{"and.l", 2,	one(0140200),	one(0170700), ";lDd", m68000up | mcfisa_a },
+{"and.l", 2,	one(0140600),	one(0170700), "Dd~l", m68000up | mcfisa_a },
@@ -107,60 +107,60 @@
-{"aslb", 2,	one(0160400),	one(0170770), "QdDs", m68000up },
-{"aslb", 2,	one(0160440),	one(0170770), "DdDs", m68000up },
-{"aslw", 2,	one(0160500),	one(0170770), "QdDs", m68000up },
-{"aslw", 2,	one(0160540),	one(0170770), "DdDs", m68000up },
-{"aslw", 2,	one(0160700),	one(0177700), "~s",   m68000up },
-{"asll", 2,	one(0160600),	one(0170770), "QdDs", m68000up | mcfisa_a },
-{"asll", 2,	one(0160640),	one(0170770), "DdDs", m68000up | mcfisa_a },
-
-{"asrb", 2,	one(0160000),	one(0170770), "QdDs", m68000up },
-{"asrb", 2,	one(0160040),	one(0170770), "DdDs", m68000up },
-{"asrw", 2,	one(0160100),	one(0170770), "QdDs", m68000up },
-{"asrw", 2,	one(0160140),	one(0170770), "DdDs", m68000up },
-{"asrw", 2,	one(0160300),	one(0177700), "~s",   m68000up },
-{"asrl", 2,	one(0160200),	one(0170770), "QdDs", m68000up | mcfisa_a },
-{"asrl", 2,	one(0160240),	one(0170770), "DdDs", m68000up | mcfisa_a },
-
-{"bhiw", 2,	one(0061000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"blsw", 2,	one(0061400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bccw", 2,	one(0062000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bcsw", 2,	one(0062400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bnew", 2,	one(0063000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"beqw", 2,	one(0063400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bvcw", 2,	one(0064000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bvsw", 2,	one(0064400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bplw", 2,	one(0065000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bmiw", 2,	one(0065400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bgew", 2,	one(0066000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bltw", 2,	one(0066400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bgtw", 2,	one(0067000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"blew", 2,	one(0067400),	one(0177777), "BW", m68000up | mcfisa_a },
-
-{"bhil", 2,	one(0061377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"blsl", 2,	one(0061777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bccl", 2,	one(0062377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bcsl", 2,	one(0062777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bnel", 2,	one(0063377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"beql", 2,	one(0063777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bvcl", 2,	one(0064377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bvsl", 2,	one(0064777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bpll", 2,	one(0065377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bmil", 2,	one(0065777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bgel", 2,	one(0066377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bltl", 2,	one(0066777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bgtl", 2,	one(0067377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"blel", 2,	one(0067777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-
-{"bhis", 2,	one(0061000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"blss", 2,	one(0061400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bccs", 2,	one(0062000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bcss", 2,	one(0062400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bnes", 2,	one(0063000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"beqs", 2,	one(0063400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bvcs", 2,	one(0064000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bvss", 2,	one(0064400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bpls", 2,	one(0065000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bmis", 2,	one(0065400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bges", 2,	one(0066000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"blts", 2,	one(0066400),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bgts", 2,	one(0067000),	one(0177400), "BB", m68000up | mcfisa_a },
-{"bles", 2,	one(0067400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"asl.b", 2,	one(0160400),	one(0170770), "QdDs", m68000up },
+{"asl.b", 2,	one(0160440),	one(0170770), "DdDs", m68000up },
+{"asl.w", 2,	one(0160500),	one(0170770), "QdDs", m68000up },
+{"asl.w", 2,	one(0160540),	one(0170770), "DdDs", m68000up },
+{"asl.w", 2,	one(0160700),	one(0177700), "~s",   m68000up },
+{"asl.l", 2,	one(0160600),	one(0170770), "QdDs", m68000up | mcfisa_a },
+{"asl.l", 2,	one(0160640),	one(0170770), "DdDs", m68000up | mcfisa_a },
+
+{"asr.b", 2,	one(0160000),	one(0170770), "QdDs", m68000up },
+{"asr.b", 2,	one(0160040),	one(0170770), "DdDs", m68000up },
+{"asr.w", 2,	one(0160100),	one(0170770), "QdDs", m68000up },
+{"asr.w", 2,	one(0160140),	one(0170770), "DdDs", m68000up },
+{"asr.w", 2,	one(0160300),	one(0177700), "~s",   m68000up },
+{"asr.l", 2,	one(0160200),	one(0170770), "QdDs", m68000up | mcfisa_a },
+{"asr.l", 2,	one(0160240),	one(0170770), "DdDs", m68000up | mcfisa_a },
+
+{"bhi.w", 2,	one(0061000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bls.w", 2,	one(0061400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bcc.w", 2,	one(0062000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bcs.w", 2,	one(0062400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bne.w", 2,	one(0063000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"beq.w", 2,	one(0063400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bvc.w", 2,	one(0064000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bvs.w", 2,	one(0064400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bpl.w", 2,	one(0065000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bmi.w", 2,	one(0065400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bge.w", 2,	one(0066000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"blt.w", 2,	one(0066400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bgt.w", 2,	one(0067000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"ble.w", 2,	one(0067400),	one(0177777), "BW", m68000up | mcfisa_a },
+
+{"bhi.l", 2,	one(0061377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bls.l", 2,	one(0061777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bcc.l", 2,	one(0062377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bcs.l", 2,	one(0062777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bne.l", 2,	one(0063377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"beq.l", 2,	one(0063777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bvc.l", 2,	one(0064377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bvs.l", 2,	one(0064777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bpl.l", 2,	one(0065377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bmi.l", 2,	one(0065777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bge.l", 2,	one(0066377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"blt.l", 2,	one(0066777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bgt.l", 2,	one(0067377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"ble.l", 2,	one(0067777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+
+{"bhi.s", 2,	one(0061000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bls.s", 2,	one(0061400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bcc.s", 2,	one(0062000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bcs.s", 2,	one(0062400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bne.s", 2,	one(0063000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"beq.s", 2,	one(0063400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bvc.s", 2,	one(0064000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bvs.s", 2,	one(0064400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bpl.s", 2,	one(0065000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bmi.s", 2,	one(0065400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bge.s", 2,	one(0066000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"blt.s", 2,	one(0066400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bgt.s", 2,	one(0067000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"ble.s", 2,	one(0067400),	one(0177400), "BB", m68000up | mcfisa_a },
@@ -206,3 +206,3 @@
-{"braw", 2,	one(0060000),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bral", 2,	one(0060377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b},
-{"bras", 2,	one(0060000),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bra.w", 2,	one(0060000),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bra.l", 2,	one(0060377),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b},
+{"bra.s", 2,	one(0060000),	one(0177400), "BB", m68000up | mcfisa_a },
@@ -215,3 +215,3 @@
-{"bsrw", 2,	one(0060400),	one(0177777), "BW", m68000up | mcfisa_a },
-{"bsrl", 2,	one(0060777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
-{"bsrs", 2,	one(0060400),	one(0177400), "BB", m68000up | mcfisa_a },
+{"bsr.w", 2,	one(0060400),	one(0177777), "BW", m68000up | mcfisa_a },
+{"bsr.l", 2,	one(0060777),	one(0177777), "BL", m68020up | cpu32 | fido_a | mcfisa_b | mcfisa_c},
+{"bsr.s", 2,	one(0060400),	one(0177400), "BB", m68000up | mcfisa_a },
@@ -227,12 +227,12 @@
-{"cas2w", 6,    two(0006374,0), two(0177777,0007070), "D3D6D2D5r1r4", m68020up },
-{"cas2w", 6,    two(0006374,0), two(0177777,0007070), "D3D6D2D5R1R4", m68020up },
-{"cas2l", 6,    two(0007374,0), two(0177777,0007070), "D3D6D2D5r1r4", m68020up },
-{"cas2l", 6,    two(0007374,0), two(0177777,0007070), "D3D6D2D5R1R4", m68020up },
-
-{"casb", 4,	two(0005300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
-{"casw", 4,	two(0006300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
-{"casl", 4,	two(0007300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
-
-{"chk2b", 4, 	two(0000300,0004000), two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
-{"chk2w", 4, 	two(0001300,0004000),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
-{"chk2l", 4, 	two(0002300,0004000),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+{"cas2.w", 6,    two(0006374,0), two(0177777,0007070), "D3D6D2D5r1r4", m68020up },
+{"cas2.w", 6,    two(0006374,0), two(0177777,0007070), "D3D6D2D5R1R4", m68020up },
+{"cas2.l", 6,    two(0007374,0), two(0177777,0007070), "D3D6D2D5r1r4", m68020up },
+{"cas2.l", 6,    two(0007374,0), two(0177777,0007070), "D3D6D2D5R1R4", m68020up },
+
+{"cas.b", 4,	two(0005300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
+{"cas.w", 4,	two(0006300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
+{"cas.l", 4,	two(0007300, 0), two(0177700, 0177070),	"D3D2~s", m68020up },
+
+{"chk2.b", 4, 	two(0000300,0004000), two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+{"chk2.w", 4, 	two(0001300,0004000),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+{"chk2.l", 4, 	two(0002300,0004000),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
@@ -240,2 +240,2 @@
-{"chkl", 2,	one(0040400),		one(0170700), ";lDd", m68000up },
-{"chkw", 2,	one(0040600),		one(0170700), ";wDd", m68000up },
+{"chk.l", 2,	one(0040400),		one(0170700), ";lDd", m68000up },
+{"chk.w", 2,	one(0040600),		one(0170700), ";wDd", m68000up },
@@ -259,21 +259,21 @@
-{"clrb", 2,	one(0041000),	one(0177700), "\$s", m68000up | mcfisa_a },
-{"clrw", 2,	one(0041100),	one(0177700), "\$s", m68000up | mcfisa_a },
-{"clrl", 2,	one(0041200),	one(0177700), "\$s", m68000up | mcfisa_a },
-
-{"cmp2b", 4,	two(0000300,0), two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
-{"cmp2w", 4,	two(0001300,0),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
-{"cmp2l", 4,	two(0002300,0),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
-
-{"cmpaw", 2,	one(0130300),	one(0170700), "*wAd", m68000up },
-{"cmpal", 2,	one(0130700),	one(0170700), "*lAd", m68000up | mcfisa_a },
-
-{"cmpib", 4,	one(0006000),	one(0177700), "#b@s", m68000up },
-{"cmpib", 4,	one(0006000),	one(0177700), "#bDs", mcfisa_b | mcfisa_c },
-{"cmpiw", 4,	one(0006100),	one(0177700), "#w@s", m68000up },
-{"cmpiw", 4,	one(0006100),	one(0177700), "#wDs", mcfisa_b | mcfisa_c },
-{"cmpil", 6,	one(0006200),	one(0177700), "#l@s", m68000up },
-{"cmpil", 6,	one(0006200),	one(0177700), "#lDs", mcfisa_a },
-
-{"cmpmb", 2,	one(0130410),	one(0170770), "+s+d", m68000up },
-{"cmpmw", 2,	one(0130510),	one(0170770), "+s+d", m68000up },
-{"cmpml", 2,	one(0130610),	one(0170770), "+s+d", m68000up },
+{"clr.b", 2,	one(0041000),	one(0177700), "\$s", m68000up | mcfisa_a },
+{"clr.w", 2,	one(0041100),	one(0177700), "\$s", m68000up | mcfisa_a },
+{"clr.l", 2,	one(0041200),	one(0177700), "\$s", m68000up | mcfisa_a },
+
+{"cmp2.b", 4,	two(0000300,0), two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+{"cmp2.w", 4,	two(0001300,0),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+{"cmp2.l", 4,	two(0002300,0),	two(0177700,07777), "!sR1", m68020up | cpu32 | fido_a },
+
+{"cmpa.w", 2,	one(0130300),	one(0170700), "*wAd", m68000up },
+{"cmpa.l", 2,	one(0130700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+
+{"cmpi.b", 4,	one(0006000),	one(0177700), "#b@s", m68000up },
+{"cmpi.b", 4,	one(0006000),	one(0177700), "#bDs", mcfisa_b | mcfisa_c },
+{"cmpi.w", 4,	one(0006100),	one(0177700), "#w@s", m68000up },
+{"cmpi.w", 4,	one(0006100),	one(0177700), "#wDs", mcfisa_b | mcfisa_c },
+{"cmpi.l", 6,	one(0006200),	one(0177700), "#l@s", m68000up },
+{"cmpi.l", 6,	one(0006200),	one(0177700), "#lDs", mcfisa_a },
+
+{"cmpm.b", 2,	one(0130410),	one(0170770), "+s+d", m68000up },
+{"cmpm.w", 2,	one(0130510),	one(0170770), "+s+d", m68000up },
+{"cmpm.l", 2,	one(0130610),	one(0170770), "+s+d", m68000up },
@@ -282,15 +282,15 @@
-{"cmpb", 4,	one(0006000),	one(0177700), "#b@s", m68000up },
-{"cmpb", 4,	one(0006000),	one(0177700), "#bDs", mcfisa_b | mcfisa_c },
-{"cmpb", 2,	one(0130410),	one(0170770), "+s+d", m68000up },
-{"cmpb", 2,	one(0130000),	one(0170700), ";bDd", m68000up },
-{"cmpb", 2,	one(0130000),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
-{"cmpw", 2,	one(0130300),	one(0170700), "*wAd", m68000up },
-{"cmpw", 4,	one(0006100),	one(0177700), "#w@s", m68000up },
-{"cmpw", 4,	one(0006100),	one(0177700), "#wDs", mcfisa_b | mcfisa_c },
-{"cmpw", 2,	one(0130510),	one(0170770), "+s+d", m68000up },
-{"cmpw", 2,	one(0130100),	one(0170700), "*wDd", m68000up | mcfisa_b | mcfisa_c },
-{"cmpl", 2,	one(0130700),	one(0170700), "*lAd", m68000up | mcfisa_a },
-{"cmpl", 6,	one(0006200),	one(0177700), "#l@s", m68000up },
-{"cmpl", 6,	one(0006200),	one(0177700), "#lDs", mcfisa_a },
-{"cmpl", 2,	one(0130610),	one(0170770), "+s+d", m68000up },
-{"cmpl", 2,	one(0130200),	one(0170700), "*lDd", m68000up | mcfisa_a },
+{"cmp.b", 4,	one(0006000),	one(0177700), "#b@s", m68000up },
+{"cmp.b", 4,	one(0006000),	one(0177700), "#bDs", mcfisa_b | mcfisa_c },
+{"cmp.b", 2,	one(0130410),	one(0170770), "+s+d", m68000up },
+{"cmp.b", 2,	one(0130000),	one(0170700), ";bDd", m68000up },
+{"cmp.b", 2,	one(0130000),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
+{"cmp.w", 2,	one(0130300),	one(0170700), "*wAd", m68000up },
+{"cmp.w", 4,	one(0006100),	one(0177700), "#w@s", m68000up },
+{"cmp.w", 4,	one(0006100),	one(0177700), "#wDs", mcfisa_b | mcfisa_c },
+{"cmp.w", 2,	one(0130510),	one(0170770), "+s+d", m68000up },
+{"cmp.w", 2,	one(0130100),	one(0170700), "*wDd", m68000up | mcfisa_b | mcfisa_c },
+{"cmp.l", 2,	one(0130700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"cmp.l", 6,	one(0006200),	one(0177700), "#l@s", m68000up },
+{"cmp.l", 6,	one(0006200),	one(0177700), "#lDs", mcfisa_a },
+{"cmp.l", 2,	one(0130610),	one(0170770), "+s+d", m68000up },
+{"cmp.l", 2,	one(0130200),	one(0170700), "*lDd", m68000up | mcfisa_a },
@@ -305,6 +305,6 @@
-{"cp0ldb",   6, one (0176000), one (01777700), ".pwR1jEK3", mcfisa_a},
-{"cp1ldb",   6, one (0177000), one (01777700), ".pwR1jEK3", mcfisa_a},
-{"cp0ldw",   6, one (0176100), one (01777700), ".pwR1jEK3", mcfisa_a},
-{"cp1ldw",   6, one (0177100), one (01777700), ".pwR1jEK3", mcfisa_a},
-{"cp0ldl",   6, one (0176200), one (01777700), ".pwR1jEK3", mcfisa_a},
-{"cp1ldl",   6, one (0177200), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp0ld.b",   6, one (0176000), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp1ld.b",   6, one (0177000), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp0ld.w",   6, one (0176100), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp1ld.w",   6, one (0177100), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp0ld.l",   6, one (0176200), one (01777700), ".pwR1jEK3", mcfisa_a},
+{"cp1ld.l",   6, one (0177200), one (01777700), ".pwR1jEK3", mcfisa_a},
@@ -313,6 +313,6 @@
-{"cp0stb",   6, one (0176400), one (01777700), ".R1pwjEK3", mcfisa_a},
-{"cp1stb",   6, one (0177400), one (01777700), ".R1pwjEK3", mcfisa_a},
-{"cp0stw",   6, one (0176500), one (01777700), ".R1pwjEK3", mcfisa_a},
-{"cp1stw",   6, one (0177500), one (01777700), ".R1pwjEK3", mcfisa_a},
-{"cp0stl",   6, one (0176600), one (01777700), ".R1pwjEK3", mcfisa_a},
-{"cp1stl",   6, one (0177600), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp0st.b",   6, one (0176400), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp1st.b",   6, one (0177400), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp0st.w",   6, one (0176500), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp1st.w",   6, one (0177500), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp0st.l",   6, one (0176600), one (01777700), ".R1pwjEK3", mcfisa_a},
+{"cp1st.l",   6, one (0177600), one (01777700), ".R1pwjEK3", mcfisa_a},
@@ -339 +339 @@
-{"divsw", 2,	one(0100700),	one(0170700), ";wDd", m68000up | mcfhwdiv },
+{"divs.w", 2,	one(0100700),	one(0170700), ";wDd", m68000up | mcfhwdiv },
@@ -341,22 +341,22 @@
-{"divsl", 4, 	two(0046100,0006000),two(0177700,0107770),";lD3D1", m68020up | cpu32 | fido_a },
-{"divsl", 4, 	two(0046100,0004000),two(0177700,0107770),";lDD",   m68020up | cpu32 | fido_a },
-{"divsl", 4, 	two(0046100,0004000),two(0177700,0107770),"qsDD",   mcfhwdiv },
-
-{"divsll", 4, 	two(0046100,0004000),two(0177700,0107770),";lD3D1",m68020up | cpu32 | fido_a },
-{"divsll", 4, 	two(0046100,0004000),two(0177700,0107770),";lDD",  m68020up | cpu32 | fido_a },
-
-{"divuw", 2,	one(0100300),		one(0170700), ";wDd", m68000up | mcfhwdiv },
-
-{"divul", 4,	two(0046100,0002000),two(0177700,0107770),";lD3D1", m68020up | cpu32 | fido_a },
-{"divul", 4,	two(0046100,0000000),two(0177700,0107770),";lDD",   m68020up | cpu32 | fido_a },
-{"divul", 4,	two(0046100,0000000),two(0177700,0107770),"qsDD",   mcfhwdiv },
-
-{"divull", 4,	two(0046100,0000000),two(0177700,0107770),";lD3D1",m68020up | cpu32 | fido_a },
-{"divull", 4,	two(0046100,0000000),two(0177700,0107770),";lDD",  m68020up | cpu32 | fido_a },
-
-{"eorib", 4,	one(0005000),	one(0177700), "#b\$s", m68000up },
-{"eorib", 4,	one(0005074),	one(0177777), "#bCs", m68000up },
-{"eoriw", 4,	one(0005100),	one(0177700), "#w\$s", m68000up },
-{"eoriw", 4,	one(0005174),	one(0177777), "#wSs", m68000up },
-{"eoril", 6,	one(0005200),	one(0177700), "#l\$s", m68000up },
-{"eoril", 6,	one(0005200),	one(0177700), "#lDs", mcfisa_a },
+{"divs.l", 4, 	two(0046100,0006000),two(0177700,0107770),";lD3D1", m68020up | cpu32 | fido_a },
+{"divs.l", 4, 	two(0046100,0004000),two(0177700,0107770),";lDD",   m68020up | cpu32 | fido_a },
+{"divs.l", 4, 	two(0046100,0004000),two(0177700,0107770),"qsDD",   mcfhwdiv },
+
+{"divsl.l", 4, 	two(0046100,0004000),two(0177700,0107770),";lD3D1",m68020up | cpu32 | fido_a },
+{"divsl.l", 4, 	two(0046100,0004000),two(0177700,0107770),";lDD",  m68020up | cpu32 | fido_a },
+
+{"divu.w", 2,	one(0100300),		one(0170700), ";wDd", m68000up | mcfhwdiv },
+
+{"divu.l", 4,	two(0046100,0002000),two(0177700,0107770),";lD3D1", m68020up | cpu32 | fido_a },
+{"divu.l", 4,	two(0046100,0000000),two(0177700,0107770),";lDD",   m68020up | cpu32 | fido_a },
+{"divu.l", 4,	two(0046100,0000000),two(0177700,0107770),"qsDD",   mcfhwdiv },
+
+{"divul.l", 4,	two(0046100,0000000),two(0177700,0107770),";lD3D1",m68020up | cpu32 | fido_a },
+{"divul.l", 4,	two(0046100,0000000),two(0177700,0107770),";lDD",  m68020up | cpu32 | fido_a },
+
+{"eori.b", 4,	one(0005000),	one(0177700), "#b\$s", m68000up },
+{"eori.b", 4,	one(0005074),	one(0177777), "#bCs", m68000up },
+{"eori.w", 4,	one(0005100),	one(0177700), "#w\$s", m68000up },
+{"eori.w", 4,	one(0005174),	one(0177777), "#wSs", m68000up },
+{"eori.l", 6,	one(0005200),	one(0177700), "#l\$s", m68000up },
+{"eori.l", 6,	one(0005200),	one(0177700), "#lDs", mcfisa_a },
@@ -368,9 +368,9 @@
-{"eorb", 4,	one(0005000),	one(0177700), "#b\$s", m68000up },
-{"eorb", 4,	one(0005074),	one(0177777), "#bCs", m68000up },
-{"eorb", 2,	one(0130400),	one(0170700), "Dd\$s", m68000up },
-{"eorw", 4,	one(0005100),	one(0177700), "#w\$s", m68000up },
-{"eorw", 4,	one(0005174),	one(0177777), "#wSs", m68000up },
-{"eorw", 2,	one(0130500),	one(0170700), "Dd\$s", m68000up },
-{"eorl", 6,	one(0005200),	one(0177700), "#l\$s", m68000up },
-{"eorl", 6,	one(0005200),	one(0177700), "#lDs", mcfisa_a },
-{"eorl", 2,	one(0130600),	one(0170700), "Dd\$s", m68000up | mcfisa_a },
+{"eor.b", 4,	one(0005000),	one(0177700), "#b\$s", m68000up },
+{"eor.b", 4,	one(0005074),	one(0177777), "#bCs", m68000up },
+{"eor.b", 2,	one(0130400),	one(0170700), "Dd\$s", m68000up },
+{"eor.w", 4,	one(0005100),	one(0177700), "#w\$s", m68000up },
+{"eor.w", 4,	one(0005174),	one(0177777), "#wSs", m68000up },
+{"eor.w", 2,	one(0130500),	one(0170700), "Dd\$s", m68000up },
+{"eor.l", 6,	one(0005200),	one(0177700), "#l\$s", m68000up },
+{"eor.l", 6,	one(0005200),	one(0177700), "#lDs", mcfisa_a },
+{"eor.l", 2,	one(0130600),	one(0170700), "Dd\$s", m68000up | mcfisa_a },
@@ -387,3 +387,3 @@
-{"extw", 2,	one(0044200),	one(0177770), "Ds", m68000up|mcfisa_a },
-{"extl", 2,	one(0044300),	one(0177770), "Ds", m68000up|mcfisa_a },
-{"extbl", 2,	one(0044700),	one(0177770), "Ds", m68020up | cpu32 | fido_a | mcfisa_a },
+{"ext.w", 2,	one(0044200),	one(0177770), "Ds", m68000up|mcfisa_a },
+{"ext.l", 2,	one(0044300),	one(0177770), "Ds", m68000up|mcfisa_a },
+{"extb.l", 2,	one(0044700),	one(0177770), "Ds", m68020up | cpu32 | fido_a | mcfisa_a },
@@ -1470,2 +1470,2 @@
-{"linkw", 4,	one(0047120),	one(0177770), "As#w", m68000up | mcfisa_a },
-{"linkl", 6,	one(0044010),	one(0177770), "As#l", m68020up | cpu32 | fido_a },
+{"link.w", 4,	one(0047120),	one(0177770), "As#w", m68000up | mcfisa_a },
+{"link.l", 6,	one(0044010),	one(0177770), "As#l", m68020up | cpu32 | fido_a },
@@ -1475,43 +1475,43 @@
-{"lslb", 2,	one(0160410),	one(0170770), "QdDs", m68000up },
-{"lslb", 2,	one(0160450),	one(0170770), "DdDs", m68000up },
-{"lslw", 2,	one(0160510),	one(0170770), "QdDs", m68000up },
-{"lslw", 2,	one(0160550),	one(0170770), "DdDs", m68000up },
-{"lslw", 2,	one(0161700),	one(0177700), "~s",   m68000up },
-{"lsll", 2,	one(0160610),	one(0170770), "QdDs", m68000up | mcfisa_a },
-{"lsll", 2,	one(0160650),	one(0170770), "DdDs", m68000up | mcfisa_a },
-
-{"lsrb", 2,	one(0160010),	one(0170770), "QdDs", m68000up },
-{"lsrb", 2,	one(0160050),	one(0170770), "DdDs", m68000up },
-{"lsrw", 2,	one(0160110),	one(0170770), "QdDs", m68000up },
-{"lsrw", 2,	one(0160150),	one(0170770), "DdDs", m68000up },
-{"lsrw", 2,	one(0161300),	one(0177700), "~s",   m68000up },
-{"lsrl", 2,	one(0160210),	one(0170770), "QdDs", m68000up | mcfisa_a },
-{"lsrl", 2,	one(0160250),	one(0170770), "DdDs", m68000up | mcfisa_a },
-
-{"macw", 4,  	two(0xa080, 0x0000), two(0xf180, 0x0910), "uNuoiI4/Rn", mcfmac },
-{"macw", 4,  	two(0xa080, 0x0200), two(0xf180, 0x0910), "uNuoMh4/Rn", mcfmac },
-{"macw", 4,  	two(0xa080, 0x0000), two(0xf180, 0x0f10), "uNuo4/Rn", mcfmac },
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf1b0, 0x0900), "uMumiI", mcfmac },
-{"macw", 4,  	two(0xa000, 0x0200), two(0xf1b0, 0x0900), "uMumMh", mcfmac },
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf1b0, 0x0f00), "uMum", mcfmac },
-
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf100, 0x0900), "uNuoiI4/RneG", mcfemac },/* Ry,Rx,SF,<ea>,accX.  */
-{"macw", 4,  	two(0xa000, 0x0200), two(0xf100, 0x0900), "uNuoMh4/RneG", mcfemac },/* Ry,Rx,+1/-1,<ea>,accX.  */
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf100, 0x0f00), "uNuo4/RneG", mcfemac },/* Ry,Rx,<ea>,accX.  */
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf130, 0x0900), "uMumiIeH", mcfemac },/* Ry,Rx,SF,accX.  */
-{"macw", 4,  	two(0xa000, 0x0200), two(0xf130, 0x0900), "uMumMheH", mcfemac },/* Ry,Rx,+1/-1,accX.  */
-{"macw", 4,  	two(0xa000, 0x0000), two(0xf130, 0x0f00), "uMumeH", mcfemac }, /* Ry,Rx,accX.  */
-
-{"macl", 4,  	two(0xa080, 0x0800), two(0xf180, 0x0910), "RNRoiI4/Rn", mcfmac },
-{"macl", 4,  	two(0xa080, 0x0a00), two(0xf180, 0x0910), "RNRoMh4/Rn", mcfmac },
-{"macl", 4,  	two(0xa080, 0x0800), two(0xf180, 0x0f10), "RNRo4/Rn", mcfmac },
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf1b0, 0x0b00), "RMRmiI", mcfmac },
-{"macl", 4,  	two(0xa000, 0x0a00), two(0xf1b0, 0x0b00), "RMRmMh", mcfmac },
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf1b0, 0x0900), "RMRm", mcfmac },
-
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf100, 0x0900), "R3R1iI4/RneG", mcfemac },
-{"macl", 4,  	two(0xa000, 0x0a00), two(0xf100, 0x0900), "R3R1Mh4/RneG", mcfemac },
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf100, 0x0f00), "R3R14/RneG", mcfemac },
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf130, 0x0900), "RMRmiIeH", mcfemac },
-{"macl", 4,  	two(0xa000, 0x0a00), two(0xf130, 0x0900), "RMRmMheH", mcfemac },
-{"macl", 4,  	two(0xa000, 0x0800), two(0xf130, 0x0f00), "RMRmeH", mcfemac },
+{"lsl.b", 2,	one(0160410),	one(0170770), "QdDs", m68000up },
+{"lsl.b", 2,	one(0160450),	one(0170770), "DdDs", m68000up },
+{"lsl.w", 2,	one(0160510),	one(0170770), "QdDs", m68000up },
+{"lsl.w", 2,	one(0160550),	one(0170770), "DdDs", m68000up },
+{"lsl.w", 2,	one(0161700),	one(0177700), "~s",   m68000up },
+{"lsl.l", 2,	one(0160610),	one(0170770), "QdDs", m68000up | mcfisa_a },
+{"lsl.l", 2,	one(0160650),	one(0170770), "DdDs", m68000up | mcfisa_a },
+
+{"lsr.b", 2,	one(0160010),	one(0170770), "QdDs", m68000up },
+{"lsr.b", 2,	one(0160050),	one(0170770), "DdDs", m68000up },
+{"lsr.w", 2,	one(0160110),	one(0170770), "QdDs", m68000up },
+{"lsr.w", 2,	one(0160150),	one(0170770), "DdDs", m68000up },
+{"lsr.w", 2,	one(0161300),	one(0177700), "~s",   m68000up },
+{"lsr.l", 2,	one(0160210),	one(0170770), "QdDs", m68000up | mcfisa_a },
+{"lsr.l", 2,	one(0160250),	one(0170770), "DdDs", m68000up | mcfisa_a },
+
+{"mac.w", 4,  	two(0xa080, 0x0000), two(0xf180, 0x0910), "uNuoiI4/Rn", mcfmac },
+{"mac.w", 4,  	two(0xa080, 0x0200), two(0xf180, 0x0910), "uNuoMh4/Rn", mcfmac },
+{"mac.w", 4,  	two(0xa080, 0x0000), two(0xf180, 0x0f10), "uNuo4/Rn", mcfmac },
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf1b0, 0x0900), "uMumiI", mcfmac },
+{"mac.w", 4,  	two(0xa000, 0x0200), two(0xf1b0, 0x0900), "uMumMh", mcfmac },
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf1b0, 0x0f00), "uMum", mcfmac },
+
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf100, 0x0900), "uNuoiI4/RneG", mcfemac },/* Ry,Rx,SF,<ea>,accX.  */
+{"mac.w", 4,  	two(0xa000, 0x0200), two(0xf100, 0x0900), "uNuoMh4/RneG", mcfemac },/* Ry,Rx,+1/-1,<ea>,accX.  */
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf100, 0x0f00), "uNuo4/RneG", mcfemac },/* Ry,Rx,<ea>,accX.  */
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf130, 0x0900), "uMumiIeH", mcfemac },/* Ry,Rx,SF,accX.  */
+{"mac.w", 4,  	two(0xa000, 0x0200), two(0xf130, 0x0900), "uMumMheH", mcfemac },/* Ry,Rx,+1/-1,accX.  */
+{"mac.w", 4,  	two(0xa000, 0x0000), two(0xf130, 0x0f00), "uMumeH", mcfemac }, /* Ry,Rx,accX.  */
+
+{"mac.l", 4,  	two(0xa080, 0x0800), two(0xf180, 0x0910), "RNRoiI4/Rn", mcfmac },
+{"mac.l", 4,  	two(0xa080, 0x0a00), two(0xf180, 0x0910), "RNRoMh4/Rn", mcfmac },
+{"mac.l", 4,  	two(0xa080, 0x0800), two(0xf180, 0x0f10), "RNRo4/Rn", mcfmac },
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf1b0, 0x0b00), "RMRmiI", mcfmac },
+{"mac.l", 4,  	two(0xa000, 0x0a00), two(0xf1b0, 0x0b00), "RMRmMh", mcfmac },
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf1b0, 0x0900), "RMRm", mcfmac },
+
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf100, 0x0900), "R3R1iI4/RneG", mcfemac },
+{"mac.l", 4,  	two(0xa000, 0x0a00), two(0xf100, 0x0900), "R3R1Mh4/RneG", mcfemac },
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf100, 0x0f00), "R3R14/RneG", mcfemac },
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf130, 0x0900), "RMRmiIeH", mcfemac },
+{"mac.l", 4,  	two(0xa000, 0x0a00), two(0xf130, 0x0900), "RMRmMheH", mcfemac },
+{"mac.l", 4,  	two(0xa000, 0x0800), two(0xf130, 0x0f00), "RMRmeH", mcfemac },
@@ -1535,2 +1535,2 @@
-{"moveal", 2,	one(0020100),	one(0170700), "*lAd", m68000up | mcfisa_a },
-{"moveaw", 2,	one(0030100),	one(0170700), "*wAd", m68000up | mcfisa_a },
+{"movea.l", 2,	one(0020100),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"movea.w", 2,	one(0030100),	one(0170700), "*wAd", m68000up | mcfisa_a },
@@ -1538 +1538 @@
-{"movclrl", 2,	one(0xA1C0),	one(0xf9f0), "eFRs", mcfemac },
+{"movclr.l", 2,	one(0xA1C0),	one(0xf9f0), "eFRs", mcfemac },
@@ -1545,10 +1545,10 @@
-{"movemw", 4,	one(0044200),	one(0177700), "Lw&s", m68000up },
-{"movemw", 4,	one(0044240),	one(0177770), "lw-s", m68000up },
-{"movemw", 4,	one(0044200),	one(0177700), "#w>s", m68000up },
-{"movemw", 4,	one(0046200),	one(0177700), "<sLw", m68000up },
-{"movemw", 4,	one(0046200),	one(0177700), "<s#w", m68000up },
-{"moveml", 4,	one(0044300),	one(0177700), "Lw&s", m68000up },
-{"moveml", 4,	one(0044340),	one(0177770), "lw-s", m68000up },
-{"moveml", 4,	one(0044300),	one(0177700), "#w>s", m68000up },
-{"moveml", 4,	one(0046300),	one(0177700), "<sLw", m68000up },
-{"moveml", 4,	one(0046300),	one(0177700), "<s#w", m68000up },
+{"movem.w", 4,	one(0044200),	one(0177700), "Lw&s", m68000up },
+{"movem.w", 4,	one(0044240),	one(0177770), "lw-s", m68000up },
+{"movem.w", 4,	one(0044200),	one(0177700), "#w>s", m68000up },
+{"movem.w", 4,	one(0046200),	one(0177700), "<sLw", m68000up },
+{"movem.w", 4,	one(0046200),	one(0177700), "<s#w", m68000up },
+{"movem.l", 4,	one(0044300),	one(0177700), "Lw&s", m68000up },
+{"movem.l", 4,	one(0044340),	one(0177770), "lw-s", m68000up },
+{"movem.l", 4,	one(0044300),	one(0177700), "#w>s", m68000up },
+{"movem.l", 4,	one(0046300),	one(0177700), "<sLw", m68000up },
+{"movem.l", 4,	one(0046300),	one(0177700), "<s#w", m68000up },
@@ -1556,13 +1556,13 @@
-{"moveml", 4,	one(0044320),	one(0177770), "Lwas", mcfisa_a },
-{"moveml", 4,	one(0044320),	one(0177770), "#was", mcfisa_a },
-{"moveml", 4,	one(0044350),	one(0177770), "Lwds", mcfisa_a },
-{"moveml", 4,	one(0044350),	one(0177770), "#wds", mcfisa_a },
-{"moveml", 4,	one(0046320),	one(0177770), "asLw", mcfisa_a },
-{"moveml", 4,	one(0046320),	one(0177770), "as#w", mcfisa_a },
-{"moveml", 4,	one(0046350),	one(0177770), "dsLw", mcfisa_a },
-{"moveml", 4,	one(0046350),	one(0177770), "ds#w", mcfisa_a },
-
-{"movepw", 2,	one(0000410),	one(0170770), "dsDd", m68000up },
-{"movepw", 2,	one(0000610),	one(0170770), "Ddds", m68000up },
-{"movepl", 2,	one(0000510),	one(0170770), "dsDd", m68000up },
-{"movepl", 2,	one(0000710),	one(0170770), "Ddds", m68000up },
+{"movem.l", 4,	one(0044320),	one(0177770), "Lwas", mcfisa_a },
+{"movem.l", 4,	one(0044320),	one(0177770), "#was", mcfisa_a },
+{"movem.l", 4,	one(0044350),	one(0177770), "Lwds", mcfisa_a },
+{"movem.l", 4,	one(0044350),	one(0177770), "#wds", mcfisa_a },
+{"movem.l", 4,	one(0046320),	one(0177770), "asLw", mcfisa_a },
+{"movem.l", 4,	one(0046320),	one(0177770), "as#w", mcfisa_a },
+{"movem.l", 4,	one(0046350),	one(0177770), "dsLw", mcfisa_a },
+{"movem.l", 4,	one(0046350),	one(0177770), "ds#w", mcfisa_a },
+
+{"movep.w", 2,	one(0000410),	one(0170770), "dsDd", m68000up },
+{"movep.w", 2,	one(0000610),	one(0170770), "Ddds", m68000up },
+{"movep.l", 2,	one(0000510),	one(0170770), "dsDd", m68000up },
+{"movep.l", 2,	one(0000710),	one(0170770), "Ddds", m68000up },
@@ -1574,60 +1574,60 @@
-{"moveb", 2,	one(0010000),	one(0170000), ";b\$d", m68000up },
-{"moveb", 2,	one(0010000),	one(0170070), "Ds\$d", mcfisa_a },
-{"moveb", 2,	one(0010020),	one(0170070), "as\$d", mcfisa_a },
-{"moveb", 2,	one(0010030),	one(0170070), "+s\$d", mcfisa_a },
-{"moveb", 2,	one(0010040),	one(0170070), "-s\$d", mcfisa_a },
-{"moveb", 2,	one(0010000),	one(0170000), "nsqd", mcfisa_a },
-{"moveb", 2,	one(0010000),	one(0170700), "obDd", mcfisa_a },
-{"moveb", 2,	one(0010200),	one(0170700), "obad", mcfisa_a },
-{"moveb", 2,	one(0010300),	one(0170700), "ob+d", mcfisa_a },
-{"moveb", 2,	one(0010400),	one(0170700), "ob-d", mcfisa_a },
-{"moveb", 2,	one(0010000),	one(0170000), "obnd", mcfisa_b | mcfisa_c },
-
-{"movew", 2,	one(0030000),	one(0170000), "*w%d", m68000up },
-{"movew", 2,	one(0030000),	one(0170000), "ms%d", mcfisa_a },
-{"movew", 2,	one(0030000),	one(0170000), "nspd", mcfisa_a },
-{"movew", 2,	one(0030000),	one(0170000), "owmd", mcfisa_a },
-{"movew", 2,	one(0030000),	one(0170000), "ownd", mcfisa_b | mcfisa_c },
-{"movew", 2,	one(0040300),	one(0177700), "Ss\$s", m68000up },
-{"movew", 2,	one(0040300),	one(0177770), "SsDs", mcfisa_a },
-{"movew", 2,	one(0041300),	one(0177700), "Cs\$s", m68010up },
-{"movew", 2,	one(0041300),	one(0177770), "CsDs", mcfisa_a },
-{"movew", 2,	one(0042300),	one(0177700), ";wCd", m68000up },
-{"movew", 2,	one(0042300),	one(0177770), "DsCd", mcfisa_a },
-{"movew", 4,	one(0042374),	one(0177777), "#wCd", mcfisa_a },
-{"movew", 2,	one(0043300),	one(0177700), ";wSd", m68000up },
-{"movew", 2,	one(0043300),	one(0177770), "DsSd", mcfisa_a },
-{"movew", 4,	one(0043374),	one(0177777), "#wSd", mcfisa_a },
-
-{"movel", 2,	one(0070000),	one(0170400), "MsDd", m68000up | mcfisa_a },
-{"movel", 2,	one(0020000),	one(0170000), "*l%d", m68000up },
-{"movel", 2,	one(0020000),	one(0170000), "ms%d", mcfisa_a },
-{"movel", 2,	one(0020000),	one(0170000), "nspd", mcfisa_a },
-{"movel", 2,	one(0020000),	one(0170000), "olmd", mcfisa_a },
-{"movel", 2,	one(0047140),	one(0177770), "AsUd", m68000up | mcfusp },
-{"movel", 2,	one(0047150),	one(0177770), "UdAs", m68000up | mcfusp },
-{"movel", 2,	one(0120600),	one(0177760), "EsRs", mcfmac },
-{"movel", 2,	one(0120400),	one(0177760), "RsEs", mcfmac },
-{"movel", 6,	one(0120474),	one(0177777), "#lEs", mcfmac },
-{"movel", 2,	one(0124600),	one(0177760), "GsRs", mcfmac },
-{"movel", 2,	one(0124400),	one(0177760), "RsGs", mcfmac },
-{"movel", 6,	one(0124474),	one(0177777), "#lGs", mcfmac },
-{"movel", 2,	one(0126600),	one(0177760), "HsRs", mcfmac },
-{"movel", 2,	one(0126400),	one(0177760), "RsHs", mcfmac },
-{"movel", 6,	one(0126474),	one(0177777), "#lHs", mcfmac },
-{"movel", 2,	one(0124700),	one(0177777), "GsCs", mcfmac },
-
-{"movel", 2,	one(0xa180),	one(0xf9f0), "eFRs", mcfemac }, /* ACCx,Rx.  */
-{"movel", 2,	one(0xab80),	one(0xfbf0), "g]Rs", mcfemac }, /* ACCEXTx,Rx.  */
-{"movel", 2,	one(0xa980),	one(0xfff0), "G-Rs", mcfemac }, /* macsr,Rx.  */
-{"movel", 2,	one(0xad80),	one(0xfff0), "H-Rs", mcfemac }, /* mask,Rx.  */
-{"movel", 2,	one(0xa110),	one(0xf9fc), "efeF", mcfemac }, /* ACCy,ACCx.  */
-{"movel", 2,	one(0xa9c0),	one(0xffff), "G-C-", mcfemac }, /* macsr,ccr.  */
-{"movel", 2,	one(0xa100),	one(0xf9f0), "RseF", mcfemac }, /* Rx,ACCx.  */
-{"movel", 6,	one(0xa13c),	one(0xf9ff), "#leF", mcfemac }, /* #,ACCx.  */
-{"movel", 2,	one(0xab00),	one(0xfbc0), "Rsg]", mcfemac }, /* Rx,ACCEXTx.  */
-{"movel", 6,	one(0xab3c),	one(0xfbff), "#lg]", mcfemac }, /* #,ACCEXTx.  */
-{"movel", 2,	one(0xa900),	one(0xffc0), "RsG-", mcfemac }, /* Rx,macsr.  */
-{"movel", 6,	one(0xa93c),	one(0xffff), "#lG-", mcfemac }, /* #,macsr.  */
-{"movel", 2,	one(0xad00),	one(0xffc0), "RsH-", mcfemac }, /* Rx,mask.  */
-{"movel", 6,	one(0xad3c),	one(0xffff), "#lH-", mcfemac }, /* #,mask.  */
+{"move.b", 2,	one(0010000),	one(0170000), ";b\$d", m68000up },
+{"move.b", 2,	one(0010000),	one(0170070), "Ds\$d", mcfisa_a },
+{"move.b", 2,	one(0010020),	one(0170070), "as\$d", mcfisa_a },
+{"move.b", 2,	one(0010030),	one(0170070), "+s\$d", mcfisa_a },
+{"move.b", 2,	one(0010040),	one(0170070), "-s\$d", mcfisa_a },
+{"move.b", 2,	one(0010000),	one(0170000), "nsqd", mcfisa_a },
+{"move.b", 2,	one(0010000),	one(0170700), "obDd", mcfisa_a },
+{"move.b", 2,	one(0010200),	one(0170700), "obad", mcfisa_a },
+{"move.b", 2,	one(0010300),	one(0170700), "ob+d", mcfisa_a },
+{"move.b", 2,	one(0010400),	one(0170700), "ob-d", mcfisa_a },
+{"move.b", 2,	one(0010000),	one(0170000), "obnd", mcfisa_b | mcfisa_c },
+
+{"move.w", 2,	one(0030000),	one(0170000), "*w%d", m68000up },
+{"move.w", 2,	one(0030000),	one(0170000), "ms%d", mcfisa_a },
+{"move.w", 2,	one(0030000),	one(0170000), "nspd", mcfisa_a },
+{"move.w", 2,	one(0030000),	one(0170000), "owmd", mcfisa_a },
+{"move.w", 2,	one(0030000),	one(0170000), "ownd", mcfisa_b | mcfisa_c },
+{"move.w", 2,	one(0040300),	one(0177700), "Ss\$s", m68000up },
+{"move.w", 2,	one(0040300),	one(0177770), "SsDs", mcfisa_a },
+{"move.w", 2,	one(0041300),	one(0177700), "Cs\$s", m68010up },
+{"move.w", 2,	one(0041300),	one(0177770), "CsDs", mcfisa_a },
+{"move.w", 2,	one(0042300),	one(0177700), ";wCd", m68000up },
+{"move.w", 2,	one(0042300),	one(0177770), "DsCd", mcfisa_a },
+{"move.w", 4,	one(0042374),	one(0177777), "#wCd", mcfisa_a },
+{"move.w", 2,	one(0043300),	one(0177700), ";wSd", m68000up },
+{"move.w", 2,	one(0043300),	one(0177770), "DsSd", mcfisa_a },
+{"move.w", 4,	one(0043374),	one(0177777), "#wSd", mcfisa_a },
+
+{"move.l", 2,	one(0070000),	one(0170400), "MsDd", m68000up | mcfisa_a },
+{"move.l", 2,	one(0020000),	one(0170000), "*l%d", m68000up },
+{"move.l", 2,	one(0020000),	one(0170000), "ms%d", mcfisa_a },
+{"move.l", 2,	one(0020000),	one(0170000), "nspd", mcfisa_a },
+{"move.l", 2,	one(0020000),	one(0170000), "olmd", mcfisa_a },
+{"move.l", 2,	one(0047140),	one(0177770), "AsUd", m68000up | mcfusp },
+{"move.l", 2,	one(0047150),	one(0177770), "UdAs", m68000up | mcfusp },
+{"move.l", 2,	one(0120600),	one(0177760), "EsRs", mcfmac },
+{"move.l", 2,	one(0120400),	one(0177760), "RsEs", mcfmac },
+{"move.l", 6,	one(0120474),	one(0177777), "#lEs", mcfmac },
+{"move.l", 2,	one(0124600),	one(0177760), "GsRs", mcfmac },
+{"move.l", 2,	one(0124400),	one(0177760), "RsGs", mcfmac },
+{"move.l", 6,	one(0124474),	one(0177777), "#lGs", mcfmac },
+{"move.l", 2,	one(0126600),	one(0177760), "HsRs", mcfmac },
+{"move.l", 2,	one(0126400),	one(0177760), "RsHs", mcfmac },
+{"move.l", 6,	one(0126474),	one(0177777), "#lHs", mcfmac },
+{"move.l", 2,	one(0124700),	one(0177777), "GsCs", mcfmac },
+
+{"move.l", 2,	one(0xa180),	one(0xf9f0), "eFRs", mcfemac }, /* ACCx,Rx.  */
+{"move.l", 2,	one(0xab80),	one(0xfbf0), "g]Rs", mcfemac }, /* ACCEXTx,Rx.  */
+{"move.l", 2,	one(0xa980),	one(0xfff0), "G-Rs", mcfemac }, /* macsr,Rx.  */
+{"move.l", 2,	one(0xad80),	one(0xfff0), "H-Rs", mcfemac }, /* mask,Rx.  */
+{"move.l", 2,	one(0xa110),	one(0xf9fc), "efeF", mcfemac }, /* ACCy,ACCx.  */
+{"move.l", 2,	one(0xa9c0),	one(0xffff), "G-C-", mcfemac }, /* macsr,ccr.  */
+{"move.l", 2,	one(0xa100),	one(0xf9f0), "RseF", mcfemac }, /* Rx,ACCx.  */
+{"move.l", 6,	one(0xa13c),	one(0xf9ff), "#leF", mcfemac }, /* #,ACCx.  */
+{"move.l", 2,	one(0xab00),	one(0xfbc0), "Rsg]", mcfemac }, /* Rx,ACCEXTx.  */
+{"move.l", 6,	one(0xab3c),	one(0xfbff), "#lg]", mcfemac }, /* #,ACCEXTx.  */
+{"move.l", 2,	one(0xa900),	one(0xffc0), "RsG-", mcfemac }, /* Rx,macsr.  */
+{"move.l", 6,	one(0xa93c),	one(0xffff), "#lG-", mcfemac }, /* #,macsr.  */
+{"move.l", 2,	one(0xad00),	one(0xffc0), "RsH-", mcfemac }, /* Rx,mask.  */
+{"move.l", 6,	one(0xad3c),	one(0xffff), "#lH-", mcfemac }, /* #,mask.  */
@@ -1654,12 +1654,12 @@
-{"mov3ql", 2,	one(0120500),	one(0170700), "xd%s", mcfisa_b | mcfisa_c },
-{"mvsb", 2,	one(0070400),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
-{"mvsw", 2,	one(0070500),	one(0170700), "*wDd", mcfisa_b | mcfisa_c },
-{"mvzb", 2,	one(0070600),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
-{"mvzw", 2,	one(0070700),	one(0170700), "*wDd", mcfisa_b | mcfisa_c },
-
-{"movesb", 4,	two(0007000, 0),     two(0177700, 07777), "~sR1", m68010up },
-{"movesb", 4,	two(0007000, 04000), two(0177700, 07777), "R1~s", m68010up },
-{"movesw", 4,	two(0007100, 0),     two(0177700, 07777), "~sR1", m68010up },
-{"movesw", 4,	two(0007100, 04000), two(0177700, 07777), "R1~s", m68010up },
-{"movesl", 4,	two(0007200, 0),     two(0177700, 07777), "~sR1", m68010up },
-{"movesl", 4,	two(0007200, 04000), two(0177700, 07777), "R1~s", m68010up },
+{"mov3q.l", 2,	one(0120500),	one(0170700), "xd%s", mcfisa_b | mcfisa_c },
+{"mvs.b", 2,	one(0070400),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
+{"mvs.w", 2,	one(0070500),	one(0170700), "*wDd", mcfisa_b | mcfisa_c },
+{"mvz.b", 2,	one(0070600),	one(0170700), "*bDd", mcfisa_b | mcfisa_c },
+{"mvz.w", 2,	one(0070700),	one(0170700), "*wDd", mcfisa_b | mcfisa_c },
+
+{"moves.b", 4,	two(0007000, 0),     two(0177700, 07777), "~sR1", m68010up },
+{"moves.b", 4,	two(0007000, 04000), two(0177700, 07777), "R1~s", m68010up },
+{"moves.w", 4,	two(0007100, 0),     two(0177700, 07777), "~sR1", m68010up },
+{"moves.w", 4,	two(0007100, 04000), two(0177700, 07777), "R1~s", m68010up },
+{"moves.l", 4,	two(0007200, 0),     two(0177700, 07777), "~sR1", m68010up },
+{"moves.l", 4,	two(0007200, 04000), two(0177700, 07777), "R1~s", m68010up },
@@ -1673,37 +1673,37 @@
-{"msacw", 4,  	two(0xa080, 0x0100), two(0xf180, 0x0910), "uNuoiI4/Rn", mcfmac },
-{"msacw", 4,  	two(0xa080, 0x0300), two(0xf180, 0x0910), "uNuoMh4/Rn", mcfmac },
-{"msacw", 4,  	two(0xa080, 0x0100), two(0xf180, 0x0f10), "uNuo4/Rn", mcfmac },
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf1b0, 0x0900), "uMumiI", mcfmac },
-{"msacw", 4,  	two(0xa000, 0x0300), two(0xf1b0, 0x0900), "uMumMh", mcfmac },
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf1b0, 0x0f00), "uMum", mcfmac },
-
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf100, 0x0900), "uNuoiI4/RneG", mcfemac },/* Ry,Rx,SF,<ea>,accX.  */
-{"msacw", 4,  	two(0xa000, 0x0300), two(0xf100, 0x0900), "uNuoMh4/RneG", mcfemac },/* Ry,Rx,+1/-1,<ea>,accX.  */
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf100, 0x0f00), "uNuo4/RneG", mcfemac },/* Ry,Rx,<ea>,accX.  */
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf130, 0x0900), "uMumiIeH", mcfemac },/* Ry,Rx,SF,accX.  */
-{"msacw", 4,  	two(0xa000, 0x0300), two(0xf130, 0x0900), "uMumMheH", mcfemac },/* Ry,Rx,+1/-1,accX.  */
-{"msacw", 4,  	two(0xa000, 0x0100), two(0xf130, 0x0f00), "uMumeH", mcfemac }, /* Ry,Rx,accX.  */
-
-{"msacl", 4,  	two(0xa080, 0x0900), two(0xf180, 0x0910), "RNRoiI4/Rn", mcfmac },
-{"msacl", 4,  	two(0xa080, 0x0b00), two(0xf180, 0x0910), "RNRoMh4/Rn", mcfmac },
-{"msacl", 4,  	two(0xa080, 0x0900), two(0xf180, 0x0f10), "RNRo4/Rn", mcfmac },
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf1b0, 0x0b00), "RMRmiI", mcfmac },
-{"msacl", 4,  	two(0xa000, 0x0b00), two(0xf1b0, 0x0b00), "RMRmMh", mcfmac },
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf1b0, 0x0900), "RMRm", mcfmac },
-
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf100, 0x0900), "R3R1iI4/RneG", mcfemac },
-{"msacl", 4,  	two(0xa000, 0x0b00), two(0xf100, 0x0900), "R3R1Mh4/RneG", mcfemac },
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf100, 0x0f00), "R3R14/RneG", mcfemac },
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf130, 0x0900), "RMRmiIeH", mcfemac },
-{"msacl", 4,  	two(0xa000, 0x0b00), two(0xf130, 0x0900), "RMRmMheH", mcfemac },
-{"msacl", 4,  	two(0xa000, 0x0900), two(0xf130, 0x0f00), "RMRmeH", mcfemac },
-
-{"mulsw", 2,	one(0140700),		one(0170700), ";wDd", m68000up|mcfisa_a },
-{"mulsl", 4,	two(0046000,004000), two(0177700,0107770), ";lD1", m68020up | cpu32 | fido_a },
-{"mulsl", 4,	two(0046000,004000), two(0177700,0107770), "qsD1", mcfisa_a },
-{"mulsl", 4,	two(0046000,006000), two(0177700,0107770), ";lD3D1",m68020up | cpu32 | fido_a },
-
-{"muluw", 2,	one(0140300),		one(0170700), ";wDd", m68000up|mcfisa_a },
-{"mulul", 4,	two(0046000,000000), two(0177700,0107770), ";lD1", m68020up | cpu32 | fido_a },
-{"mulul", 4,	two(0046000,000000), two(0177700,0107770), "qsD1", mcfisa_a },
-{"mulul", 4,	two(0046000,002000), two(0177700,0107770), ";lD3D1",m68020up | cpu32 | fido_a },
+{"msac.w", 4,  	two(0xa080, 0x0100), two(0xf180, 0x0910), "uNuoiI4/Rn", mcfmac },
+{"msac.w", 4,  	two(0xa080, 0x0300), two(0xf180, 0x0910), "uNuoMh4/Rn", mcfmac },
+{"msac.w", 4,  	two(0xa080, 0x0100), two(0xf180, 0x0f10), "uNuo4/Rn", mcfmac },
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf1b0, 0x0900), "uMumiI", mcfmac },
+{"msac.w", 4,  	two(0xa000, 0x0300), two(0xf1b0, 0x0900), "uMumMh", mcfmac },
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf1b0, 0x0f00), "uMum", mcfmac },
+
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf100, 0x0900), "uNuoiI4/RneG", mcfemac },/* Ry,Rx,SF,<ea>,accX.  */
+{"msac.w", 4,  	two(0xa000, 0x0300), two(0xf100, 0x0900), "uNuoMh4/RneG", mcfemac },/* Ry,Rx,+1/-1,<ea>,accX.  */
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf100, 0x0f00), "uNuo4/RneG", mcfemac },/* Ry,Rx,<ea>,accX.  */
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf130, 0x0900), "uMumiIeH", mcfemac },/* Ry,Rx,SF,accX.  */
+{"msac.w", 4,  	two(0xa000, 0x0300), two(0xf130, 0x0900), "uMumMheH", mcfemac },/* Ry,Rx,+1/-1,accX.  */
+{"msac.w", 4,  	two(0xa000, 0x0100), two(0xf130, 0x0f00), "uMumeH", mcfemac }, /* Ry,Rx,accX.  */
+
+{"msac.l", 4,  	two(0xa080, 0x0900), two(0xf180, 0x0910), "RNRoiI4/Rn", mcfmac },
+{"msac.l", 4,  	two(0xa080, 0x0b00), two(0xf180, 0x0910), "RNRoMh4/Rn", mcfmac },
+{"msac.l", 4,  	two(0xa080, 0x0900), two(0xf180, 0x0f10), "RNRo4/Rn", mcfmac },
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf1b0, 0x0b00), "RMRmiI", mcfmac },
+{"msac.l", 4,  	two(0xa000, 0x0b00), two(0xf1b0, 0x0b00), "RMRmMh", mcfmac },
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf1b0, 0x0900), "RMRm", mcfmac },
+
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf100, 0x0900), "R3R1iI4/RneG", mcfemac },
+{"msac.l", 4,  	two(0xa000, 0x0b00), two(0xf100, 0x0900), "R3R1Mh4/RneG", mcfemac },
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf100, 0x0f00), "R3R14/RneG", mcfemac },
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf130, 0x0900), "RMRmiIeH", mcfemac },
+{"msac.l", 4,  	two(0xa000, 0x0b00), two(0xf130, 0x0900), "RMRmMheH", mcfemac },
+{"msac.l", 4,  	two(0xa000, 0x0900), two(0xf130, 0x0f00), "RMRmeH", mcfemac },
+
+{"muls.w", 2,	one(0140700),		one(0170700), ";wDd", m68000up|mcfisa_a },
+{"muls.l", 4,	two(0046000,004000), two(0177700,0107770), ";lD1", m68020up | cpu32 | fido_a },
+{"muls.l", 4,	two(0046000,004000), two(0177700,0107770), "qsD1", mcfisa_a },
+{"muls.l", 4,	two(0046000,006000), two(0177700,0107770), ";lD3D1",m68020up | cpu32 | fido_a },
+
+{"mulu.w", 2,	one(0140300),		one(0170700), ";wDd", m68000up|mcfisa_a },
+{"mulu.l", 4,	two(0046000,000000), two(0177700,0107770), ";lD1", m68020up | cpu32 | fido_a },
+{"mulu.l", 4,	two(0046000,000000), two(0177700,0107770), "qsD1", mcfisa_a },
+{"mulu.l", 4,	two(0046000,002000), two(0177700,0107770), ";lD3D1",m68020up | cpu32 | fido_a },
@@ -1713,9 +1713,9 @@
-{"negb", 2,	one(0042000),	one(0177700), "\$s", m68000up },
-{"negw", 2,	one(0042100),	one(0177700), "\$s", m68000up },
-{"negl", 2,	one(0042200),	one(0177700), "\$s", m68000up },
-{"negl", 2,	one(0042200),	one(0177700), "Ds", mcfisa_a},
-
-{"negxb", 2,	one(0040000),	one(0177700), "\$s", m68000up },
-{"negxw", 2,	one(0040100),	one(0177700), "\$s", m68000up },
-{"negxl", 2,	one(0040200),	one(0177700), "\$s", m68000up },
-{"negxl", 2,	one(0040200),	one(0177700), "Ds", mcfisa_a},
+{"neg.b", 2,	one(0042000),	one(0177700), "\$s", m68000up },
+{"neg.w", 2,	one(0042100),	one(0177700), "\$s", m68000up },
+{"neg.l", 2,	one(0042200),	one(0177700), "\$s", m68000up },
+{"neg.l", 2,	one(0042200),	one(0177700), "Ds", mcfisa_a},
+
+{"negx.b", 2,	one(0040000),	one(0177700), "\$s", m68000up },
+{"negx.w", 2,	one(0040100),	one(0177700), "\$s", m68000up },
+{"negx.l", 2,	one(0040200),	one(0177700), "\$s", m68000up },
+{"negx.l", 2,	one(0040200),	one(0177700), "Ds", mcfisa_a},
@@ -1725,11 +1725,11 @@
-{"notb", 2,	one(0043000),	one(0177700), "\$s", m68000up },
-{"notw", 2,	one(0043100),	one(0177700), "\$s", m68000up },
-{"notl", 2,	one(0043200),	one(0177700), "\$s", m68000up },
-{"notl", 2,	one(0043200),	one(0177700), "Ds", mcfisa_a},
-
-{"orib", 4,	one(0000000),	one(0177700), "#b\$s", m68000up },
-{"orib", 4,	one(0000074),	one(0177777), "#bCs", m68000up },
-{"oriw", 4,	one(0000100),	one(0177700), "#w\$s", m68000up },
-{"oriw", 4,	one(0000174),	one(0177777), "#wSs", m68000up },
-{"oril", 6,	one(0000200),	one(0177700), "#l\$s", m68000up },
-{"oril", 6,	one(0000200),	one(0177700), "#lDs", mcfisa_a },
+{"not.b", 2,	one(0043000),	one(0177700), "\$s", m68000up },
+{"not.w", 2,	one(0043100),	one(0177700), "\$s", m68000up },
+{"not.l", 2,	one(0043200),	one(0177700), "\$s", m68000up },
+{"not.l", 2,	one(0043200),	one(0177700), "Ds", mcfisa_a},
+
+{"ori.b", 4,	one(0000000),	one(0177700), "#b\$s", m68000up },
+{"ori.b", 4,	one(0000074),	one(0177777), "#bCs", m68000up },
+{"ori.w", 4,	one(0000100),	one(0177700), "#w\$s", m68000up },
+{"ori.w", 4,	one(0000174),	one(0177777), "#wSs", m68000up },
+{"ori.l", 6,	one(0000200),	one(0177700), "#l\$s", m68000up },
+{"ori.l", 6,	one(0000200),	one(0177700), "#lDs", mcfisa_a },
@@ -1741,12 +1741,12 @@
-{"orb", 4,	one(0000000),	one(0177700), "#b\$s", m68000up },
-{"orb", 4,	one(0000074),	one(0177777), "#bCs", m68000up },
-{"orb", 2,	one(0100000),	one(0170700), ";bDd", m68000up },
-{"orb", 2,	one(0100400),	one(0170700), "Dd~s", m68000up },
-{"orw", 4,	one(0000100),	one(0177700), "#w\$s", m68000up },
-{"orw", 4,	one(0000174),	one(0177777), "#wSs", m68000up },
-{"orw", 2,	one(0100100),	one(0170700), ";wDd", m68000up },
-{"orw", 2,	one(0100500),	one(0170700), "Dd~s", m68000up },
-{"orl", 6,	one(0000200),	one(0177700), "#l\$s", m68000up },
-{"orl", 6,	one(0000200),	one(0177700), "#lDs", mcfisa_a },
-{"orl", 2,	one(0100200),	one(0170700), ";lDd", m68000up | mcfisa_a },
-{"orl", 2,	one(0100600),	one(0170700), "Dd~s", m68000up | mcfisa_a },
+{"or.b", 4,	one(0000000),	one(0177700), "#b\$s", m68000up },
+{"or.b", 4,	one(0000074),	one(0177777), "#bCs", m68000up },
+{"or.b", 2,	one(0100000),	one(0170700), ";bDd", m68000up },
+{"or.b", 2,	one(0100400),	one(0170700), "Dd~s", m68000up },
+{"or.w", 4,	one(0000100),	one(0177700), "#w\$s", m68000up },
+{"or.w", 4,	one(0000174),	one(0177777), "#wSs", m68000up },
+{"or.w", 2,	one(0100100),	one(0170700), ";wDd", m68000up },
+{"or.w", 2,	one(0100500),	one(0170700), "Dd~s", m68000up },
+{"or.l", 6,	one(0000200),	one(0177700), "#l\$s", m68000up },
+{"or.l", 6,	one(0000200),	one(0177700), "#lDs", mcfisa_a },
+{"or.l", 2,	one(0100200),	one(0170700), ";lDd", m68000up | mcfisa_a },
+{"or.l", 2,	one(0100600),	one(0170700), "Dd~s", m68000up | mcfisa_a },
@@ -1981,31 +1981,31 @@
-{"rolb", 2,	one(0160430),		one(0170770), "QdDs", m68000up },
-{"rolb", 2,	one(0160470),		one(0170770), "DdDs", m68000up },
-{"rolw", 2,	one(0160530),		one(0170770), "QdDs", m68000up },
-{"rolw", 2,	one(0160570),		one(0170770), "DdDs", m68000up },
-{"rolw", 2,	one(0163700),		one(0177700), "~s",   m68000up },
-{"roll", 2,	one(0160630),		one(0170770), "QdDs", m68000up },
-{"roll", 2,	one(0160670),		one(0170770), "DdDs", m68000up },
-
-{"rorb", 2,	one(0160030),		one(0170770), "QdDs", m68000up },
-{"rorb", 2,	one(0160070),		one(0170770), "DdDs", m68000up },
-{"rorw", 2,	one(0160130),		one(0170770), "QdDs", m68000up },
-{"rorw", 2,	one(0160170),		one(0170770), "DdDs", m68000up },
-{"rorw", 2,	one(0163300),		one(0177700), "~s",   m68000up },
-{"rorl", 2,	one(0160230),		one(0170770), "QdDs", m68000up },
-{"rorl", 2,	one(0160270),		one(0170770), "DdDs", m68000up },
-
-{"roxlb", 2,	one(0160420),		one(0170770), "QdDs", m68000up },
-{"roxlb", 2,	one(0160460),		one(0170770), "DdDs", m68000up },
-{"roxlw", 2,	one(0160520),		one(0170770), "QdDs", m68000up },
-{"roxlw", 2,	one(0160560),		one(0170770), "DdDs", m68000up },
-{"roxlw", 2,	one(0162700),		one(0177700), "~s",   m68000up },
-{"roxll", 2,	one(0160620),		one(0170770), "QdDs", m68000up },
-{"roxll", 2,	one(0160660),		one(0170770), "DdDs", m68000up },
-
-{"roxrb", 2,	one(0160020),		one(0170770), "QdDs", m68000up },
-{"roxrb", 2,	one(0160060),		one(0170770), "DdDs", m68000up },
-{"roxrw", 2,	one(0160120),		one(0170770), "QdDs", m68000up },
-{"roxrw", 2,	one(0160160),		one(0170770), "DdDs", m68000up },
-{"roxrw", 2,	one(0162300),		one(0177700), "~s",   m68000up },
-{"roxrl", 2,	one(0160220),		one(0170770), "QdDs", m68000up },
-{"roxrl", 2,	one(0160260),		one(0170770), "DdDs", m68000up },
+{"rol.b", 2,	one(0160430),		one(0170770), "QdDs", m68000up },
+{"rol.b", 2,	one(0160470),		one(0170770), "DdDs", m68000up },
+{"rol.w", 2,	one(0160530),		one(0170770), "QdDs", m68000up },
+{"rol.w", 2,	one(0160570),		one(0170770), "DdDs", m68000up },
+{"rol.w", 2,	one(0163700),		one(0177700), "~s",   m68000up },
+{"rol.l", 2,	one(0160630),		one(0170770), "QdDs", m68000up },
+{"rol.l", 2,	one(0160670),		one(0170770), "DdDs", m68000up },
+
+{"ror.b", 2,	one(0160030),		one(0170770), "QdDs", m68000up },
+{"ror.b", 2,	one(0160070),		one(0170770), "DdDs", m68000up },
+{"ror.w", 2,	one(0160130),		one(0170770), "QdDs", m68000up },
+{"ror.w", 2,	one(0160170),		one(0170770), "DdDs", m68000up },
+{"ror.w", 2,	one(0163300),		one(0177700), "~s",   m68000up },
+{"ror.l", 2,	one(0160230),		one(0170770), "QdDs", m68000up },
+{"ror.l", 2,	one(0160270),		one(0170770), "DdDs", m68000up },
+
+{"roxl.b", 2,	one(0160420),		one(0170770), "QdDs", m68000up },
+{"roxl.b", 2,	one(0160460),		one(0170770), "DdDs", m68000up },
+{"roxl.w", 2,	one(0160520),		one(0170770), "QdDs", m68000up },
+{"roxl.w", 2,	one(0160560),		one(0170770), "DdDs", m68000up },
+{"roxl.w", 2,	one(0162700),		one(0177700), "~s",   m68000up },
+{"roxl.l", 2,	one(0160620),		one(0170770), "QdDs", m68000up },
+{"roxl.l", 2,	one(0160660),		one(0170770), "DdDs", m68000up },
+
+{"roxr.b", 2,	one(0160020),		one(0170770), "QdDs", m68000up },
+{"roxr.b", 2,	one(0160060),		one(0170770), "DdDs", m68000up },
+{"roxr.w", 2,	one(0160120),		one(0170770), "QdDs", m68000up },
+{"roxr.w", 2,	one(0160160),		one(0170770), "DdDs", m68000up },
+{"roxr.w", 2,	one(0162300),		one(0177700), "~s",   m68000up },
+{"roxr.l", 2,	one(0160220),		one(0170770), "QdDs", m68000up },
+{"roxr.l", 2,	one(0160260),		one(0170770), "DdDs", m68000up },
@@ -2127,2 +2127,2 @@
-{"subal", 2,	one(0110700),	one(0170700), "*lAd", m68000up | mcfisa_a },
-{"subaw", 2,	one(0110300),	one(0170700), "*wAd", m68000up },
+{"suba.l", 2,	one(0110700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"suba.w", 2,	one(0110300),	one(0170700), "*wAd", m68000up },
@@ -2130,8 +2130,8 @@
-{"subib", 4,	one(0002000),	one(0177700), "#b\$s", m68000up },
-{"subiw", 4,	one(0002100),	one(0177700), "#w\$s", m68000up },
-{"subil", 6,	one(0002200),	one(0177700), "#l\$s", m68000up },
-{"subil", 6,	one(0002200),	one(0177700), "#lDs", mcfisa_a },
-
-{"subqb", 2,	one(0050400),	one(0170700), "Qd%s", m68000up },
-{"subqw", 2,	one(0050500),	one(0170700), "Qd%s", m68000up },
-{"subql", 2,	one(0050600),	one(0170700), "Qd%s", m68000up | mcfisa_a },
+{"subi.b", 4,	one(0002000),	one(0177700), "#b\$s", m68000up },
+{"subi.w", 4,	one(0002100),	one(0177700), "#w\$s", m68000up },
+{"subi.l", 6,	one(0002200),	one(0177700), "#l\$s", m68000up },
+{"subi.l", 6,	one(0002200),	one(0177700), "#lDs", mcfisa_a },
+
+{"subq.b", 2,	one(0050400),	one(0170700), "Qd%s", m68000up },
+{"subq.w", 2,	one(0050500),	one(0170700), "Qd%s", m68000up },
+{"subq.l", 2,	one(0050600),	one(0170700), "Qd%s", m68000up | mcfisa_a },
@@ -2140,22 +2140,22 @@
-{"subb", 2,	one(0050400),	one(0170700), "Qd%s", m68000up },
-{"subb", 4,	one(0002000),	one(0177700), "#b\$s", m68000up },
-{"subb", 2,	one(0110000),	one(0170700), ";bDd", m68000up },
-{"subb", 2,	one(0110400),	one(0170700), "Dd~s", m68000up },
-{"subw", 2,	one(0050500),	one(0170700), "Qd%s", m68000up },
-{"subw", 4,	one(0002100),	one(0177700), "#w\$s", m68000up },
-{"subw", 2,	one(0110300),	one(0170700), "*wAd", m68000up },
-{"subw", 2,	one(0110100),	one(0170700), "*wDd", m68000up },
-{"subw", 2,	one(0110500),	one(0170700), "Dd~s", m68000up },
-{"subl", 2,	one(0050600),	one(0170700), "Qd%s", m68000up | mcfisa_a },
-{"subl", 6,	one(0002200),	one(0177700), "#l\$s", m68000up },
-{"subl", 6,	one(0002200),	one(0177700), "#lDs", mcfisa_a },
-{"subl", 2,	one(0110700),	one(0170700), "*lAd", m68000up | mcfisa_a },
-{"subl", 2,	one(0110200),	one(0170700), "*lDd", m68000up | mcfisa_a },
-{"subl", 2,	one(0110600),	one(0170700), "Dd~s", m68000up | mcfisa_a },
-
-{"subxb", 2,	one(0110400),	one(0170770), "DsDd", m68000up },
-{"subxb", 2,	one(0110410),	one(0170770), "-s-d", m68000up },
-{"subxw", 2,	one(0110500),	one(0170770), "DsDd", m68000up },
-{"subxw", 2,	one(0110510),	one(0170770), "-s-d", m68000up },
-{"subxl", 2,	one(0110600),	one(0170770), "DsDd", m68000up | mcfisa_a },
-{"subxl", 2,	one(0110610),	one(0170770), "-s-d", m68000up },
+{"sub.b", 2,	one(0050400),	one(0170700), "Qd%s", m68000up },
+{"sub.b", 4,	one(0002000),	one(0177700), "#b\$s", m68000up },
+{"sub.b", 2,	one(0110000),	one(0170700), ";bDd", m68000up },
+{"sub.b", 2,	one(0110400),	one(0170700), "Dd~s", m68000up },
+{"sub.w", 2,	one(0050500),	one(0170700), "Qd%s", m68000up },
+{"sub.w", 4,	one(0002100),	one(0177700), "#w\$s", m68000up },
+{"sub.w", 2,	one(0110300),	one(0170700), "*wAd", m68000up },
+{"sub.w", 2,	one(0110100),	one(0170700), "*wDd", m68000up },
+{"sub.w", 2,	one(0110500),	one(0170700), "Dd~s", m68000up },
+{"sub.l", 2,	one(0050600),	one(0170700), "Qd%s", m68000up | mcfisa_a },
+{"sub.l", 6,	one(0002200),	one(0177700), "#l\$s", m68000up },
+{"sub.l", 6,	one(0002200),	one(0177700), "#lDs", mcfisa_a },
+{"sub.l", 2,	one(0110700),	one(0170700), "*lAd", m68000up | mcfisa_a },
+{"sub.l", 2,	one(0110200),	one(0170700), "*lDd", m68000up | mcfisa_a },
+{"sub.l", 2,	one(0110600),	one(0170700), "Dd~s", m68000up | mcfisa_a },
+
+{"subx.b", 2,	one(0110400),	one(0170770), "DsDd", m68000up },
+{"subx.b", 2,	one(0110410),	one(0170770), "-s-d", m68000up },
+{"subx.w", 2,	one(0110500),	one(0170770), "DsDd", m68000up },
+{"subx.w", 2,	one(0110510),	one(0170770), "-s-d", m68000up },
+{"subx.l", 2,	one(0110600),	one(0170770), "DsDd", m68000up | mcfisa_a },
+{"subx.l", 2,	one(0110610),	one(0170770), "-s-d", m68000up },
@@ -2172 +2172 @@
-{"swbegl", 6,	one(0045375),	one(0177777), "#l",   m68000up | mcfisa_a },
+{"swbeg.l", 6,	one(0045375),	one(0177777), "#l",   m68000up | mcfisa_a },
@@ -2192,6 +2192,6 @@
-{"tstb", 2,	one(0045000),	one(0177700), ";b", m68020up | cpu32 | fido_a | mcfisa_a },
-{"tstb", 2,	one(0045000),	one(0177700), "\$b", m68000up },
-{"tstw", 2,	one(0045100),	one(0177700), "*w", m68020up | cpu32 | fido_a | mcfisa_a },
-{"tstw", 2,	one(0045100),	one(0177700), "\$w", m68000up },
-{"tstl", 2,	one(0045200),	one(0177700), "*l", m68020up | cpu32 | fido_a | mcfisa_a },
-{"tstl", 2,	one(0045200),	one(0177700), "\$l", m68000up },
+{"tst.b", 2,	one(0045000),	one(0177700), ";b", m68020up | cpu32 | fido_a | mcfisa_a },
+{"tst.b", 2,	one(0045000),	one(0177700), "\$b", m68000up },
+{"tst.w", 2,	one(0045100),	one(0177700), "*w", m68020up | cpu32 | fido_a | mcfisa_a },
+{"tst.w", 2,	one(0045100),	one(0177700), "\$w", m68000up },
+{"tst.l", 2,	one(0045200),	one(0177700), "*l", m68020up | cpu32 | fido_a | mcfisa_a },
+{"tst.l", 2,	one(0045200),	one(0177700), "\$l", m68000up },
EOF

cd gdb-7.2-ms1/
mkdir gdb-build
cd gdb-build/
../configure --target=m68k-elf
make > build.log 2>&1
sudo make install
cat > dis68.c <<EOF
/*
 * Code to test the 68K disassembler used by GDB. This should be extended
 * to cover a large subset of 68000 machine code and run as part of the
 * patch & build process to make sure the switch to Motorola syntax for
 * the disassembly hasn't broken anything.
 * 
 * Copyright (C) 2011 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dis-asm.h>
#include <stdarg.h>

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned long      uint32;

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

extern const bfd_arch_info_type bfd_m68k_arch;
static char disassembly[1024];
static const uint8 *insBase;

uint8 *readFile(const char *name, uint32 *length);

static void print_address(bfd_vma addr, struct disassemble_info *info) {
	(*info->fprintf_func)(info->stream, "0x%X", addr);
}

static int read_memory(
	bfd_vma memaddr, uint8 *myaddr, unsigned int len, struct disassemble_info *UNUSED(info))
{
	memcpy(myaddr, insBase + memaddr, len);
	return 0;
}

static int fprintf_disasm(void *stream, const char *format, ...) {
	va_list args;
	char chunk[1024];
	va_start(args, format);
	vsprintf(chunk, format, args);
	va_end(args);
	strcat((char*)stream, chunk);
	/* Something non -ve.  */
	return 0;
}


#define VERIFY1(e, o) if ( verify1(&di, e, o) ) { expected = e; goto fail; }
int verify1(struct disassemble_info *di, const char *ex, uint16 opcode) {
	int bytesEaten;
	uint8 insn[16];
	insn[0] = opcode >> 8;
	insn[1] = opcode & 0x00FF;
	*disassembly = '\\0';
	insBase = insn;
	bytesEaten = print_insn_m68k(0x000000, di);
	if ( !strcmp(ex, disassembly) && bytesEaten == 2 ) {
		return 0;
	} else {
		return -1;
	}
}
#define VERIFY2(e, o, p0) if ( verify2(&di, e, o, p0) ) { expected = e; goto fail; }
int verify2(struct disassemble_info *di, const char *ex, uint16 opcode, uint16 param0) {
	int bytesEaten;
	uint8 insn[16];
	insn[0] = opcode >> 8;
	insn[1] = opcode & 0x00FF;
	insn[2] = param0 >> 8;
	insn[3] = param0 & 0x00FF;
	*disassembly = '\\0';
	insBase = insn;
	bytesEaten = print_insn_m68k(0x000000, di);
	if ( !strcmp(ex, disassembly) && bytesEaten == 4 ) {
		return 0;
	} else {
		return -1;
	}
}

void init(struct disassemble_info *di) {
	init_disassemble_info(di, disassembly, (fprintf_ftype)fprintf_disasm);
	di->flavour = bfd_target_unknown_flavour;
	di->memory_error_func = NULL;
	di->print_address_func = print_address;
	di->read_memory_func = read_memory;
	di->arch = bfd_m68k_arch.arch;
	di->mach = bfd_m68k_arch.mach;
	di->endian = BFD_ENDIAN_BIG;
	di->endian_code = BFD_ENDIAN_BIG;
	di->application_data = NULL;
	disassemble_init_for_target(di);
}

uint32 disassemble(struct disassemble_info *di, const uint8 *baseAddress, uint32 offset) {
	*disassembly = '\\0';
	insBase = baseAddress;
	return print_insn_m68k(offset, di);
}

int main(int argc, const char *argv[]) {
	struct disassemble_info di;
	const uint8 *rom;
	uint32 romLength;
	uint32 address;
	uint32 numLines;
	uint32 bytesEaten;
	init(&di);
	if ( argc != 4 ) {
		fprintf(stderr, "Synopsis: %s <file> <address> <numLines>\\n", argv[0]);
		exit(1);
	}
	rom = readFile(argv[1], &romLength);
	if ( !rom ) {
		fprintf(stderr, "File not found\\n");
		exit(1);
	}
	address = strtoul(argv[2], NULL, 0);
	if ( !address ) {
		fprintf(stderr, "Address \\"%s\\" cannot be parsed\\n", argv[2]);
		exit(1);
	}
	numLines = strtoul(argv[3], NULL, 0);
	if ( !numLines ) {
		fprintf(stderr, "NumLines \\"%s\\" cannot be parsed\\n", argv[3]);
		exit(1);
	}
	while ( numLines-- ) {
		bytesEaten = disassemble(&di, rom, address);
		printf("0x%06lX  %s\\n", address, disassembly);
		address += bytesEaten;
	}
	free((void*)rom);
	return 0;
}

int main2(void) {
	struct disassemble_info di;
	const char *expected;
	init(&di);

	VERIFY1("rts", 0x4E75);
	VERIFY1("moveq #-1, d0", 0x70FF);
	VERIFY2("move.w 126(a5, d1.w), d0", 0x3035, 0x107E);
	VERIFY2("move.w 126(a5), d0", 0x302D, 0x007E);
	VERIFY2("move.w 0x00001A(pc, d1.w), d0", 0x303B, 0x1018);
	return 0;
fail:
	fprintf(stderr, "Expected \\"%s\\", got \\"%s\\"\\n", expected, disassembly);
	return -1;
}

/*
 * Allocate a buffer big enough to fit file into, then read the file into it,
 * then write the file length to the location pointed to by 'length'. Naturally,
 * responsibility for the allocated buffer passes to the caller.
 */
uint8 *readFile(const char *name, uint32 *length) {
	FILE *file;
	uint8 *buffer;
	uint32 fileLen;
	uint32 returnCode;

	file = fopen(name, "rb");
	if ( !file ) {
		return NULL;
	}
	
	fseek(file, 0, SEEK_END);
	fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate enough space for an extra byte just in case the file size is odd
	buffer = (uint8 *)malloc(fileLen + 1);
	if ( !buffer ) {
		fclose(file);
		return NULL;
	}
	returnCode = fread(buffer, 1, fileLen, file);
	if ( returnCode == fileLen ) {
		if ( fileLen & 1 ) {
			fileLen++;
		}
		*length = fileLen;
	}
	fclose(file);
	return buffer;
}
EOF
gcc -g -Wall -Wextra -Wundef -pedantic-errors -std=c99 -Wstrict-prototypes -Wno-missing-field-initializers -I../include -Ibfd -o dis68 dis68.c opcodes/libopcodes.a bfd/libbfd.a libiberty/libiberty.a -lz
sudo cp dis68 /usr/local/bin/
