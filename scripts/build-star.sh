#!/bin/sh

# sudo apt-get install nasm
if [ ! -e star026c.zip ]; then
    wget http://www.neillcorlett.com/downloads/star026c.zip
fi
rm -rf build starcpu.h
mkdir build
cd build
unzip ../star026c.zip
patch -l starcpu.h <<EOF
--- old/starcpu.h	1999-06-16 18:54:34.000000000 +0100
+++ new/starcpu.h	2011-06-10 19:58:51.061109000 +0100
@@ -71,2 +71,3 @@
 	void         (*resethandler)(void);                   \
+	void         (*illegalhandler)(void);                 \
 	unsigned       dreg[8];                               \
EOF
patch -l star.c <<EOF
--- old/star.c	2001-07-16 22:51:26.000000000 +0100
+++ new/star.c	2011-06-10 19:52:23.753109002 +0100
@@ -244 +244 @@
-	emit("global %s%s_\n", sourcename, fname);
+	emit("global %s%s\n", sourcename, fname);
@@ -246 +246 @@
-	emit("%s%s_:\n", sourcename, fname);
+	emit("%s%s:\n", sourcename, fname);
@@ -253 +253 @@
-	emit("global _%scontext\n", sourcename);
+	emit("global %scontext\n", sourcename);
@@ -255 +255 @@
-	emit("_%scontext:\n", sourcename);
+	emit("%scontext:\n", sourcename);
@@ -312,0 +313 @@
+		emit("__illegalhandler       dd 0\n");
@@ -4343,3 +4344,3 @@
-	emit("mov edx,10h\n");
-	emit("call group_1_exception\n");
-	perform_cached_rebase();
+	airlock_exit();
+	emit("call [__illegalhandler]\n");
+	airlock_enter();
EOF
gcc -o star star.c
./star m68k.s -hog
nasm -f elf m68k.s
#nm m68k.o | grep s68000init
#nm m68k.o | grep s68000context
cp m68k.o ..
cp starcpu.h ..
cd ..
rm -rf build
