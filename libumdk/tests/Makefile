#
# Copyright (C) 2009-2010 Chris McClelland
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

# To run the tests you will need UnitTest++ from http://unittest-cpp.sourceforge.net
#
TARGET = runTests
UTPP_HOME=../../../../3rd/UnitTest++

INCLUDES = \
	-I../../../../include

LIBS = \
	../libumdk.a \
	../../../neopurgo/libsync/libsync.a \
	../../../../libs/usbwrap/libusbwrap.a \
	$(UTPP_HOME)/libUnitTest++.a \
	-lusb

CPP_SRCS = $(shell ls *.cpp)
CPP_OBJS = $(CPP_SRCS:%.cpp=$(OBJDIR)/%.o)
CPP = g++
CPPFLAGS = -O3 -Wall -Wextra -Wundef -std=c++98 -pedantic-errors -I$(UTPP_HOME)/src $(INCLUDES) -D_ISOC99_SOURCE -DBRK_ADDR=$(shell ../../util/scripts/getsym.sh ../../m68k/testrom/testrom.elf brk)UL
LDFLAGS =
OBJDIR = .build
DEPDIR = .deps

all: $(TARGET)
	sudo ./$(TARGET)

$(TARGET): $(CPP_OBJS) $(TEST_OBJS)
	$(CPP) $(LDFLAGS) -o $@ $(CPP_OBJS) $(LIBS)
	strip $(TARGET)

$(OBJDIR)/%.o : %.cpp
	$(CPP) -c $(CPPFLAGS) -MMD -MP -MF $(DEPDIR)/$(@F).d $< -o $@

clean: FORCE
	rm -rf $(OBJDIR) $(TARGET) $(DEPDIR) Debug Release *.ncb *.user tmpFile.*

-include $(shell mkdir -p $(OBJDIR) $(DEPDIR) 2>/dev/null) $(wildcard $(DEPDIR)/*)

FORCE:
