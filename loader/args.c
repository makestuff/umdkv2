/*
 * Copyright (C) 2009-2012 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include "args.h"

void suggest(const char *prog) {
	fprintf(stderr, "Try '%s -h' for more information.\n", prog);
}
void requires(const char *prog, const char *arg) {
	fprintf(stderr, "%s: option \"-%s\" requires an argument\n", prog, arg);
	suggest(prog);
}
void missing(const char *prog, const char *arg) {
	fprintf(stderr, "%s: missing option \"-%s\"\n", prog, arg);
	suggest(prog);
}
void invalid(const char *prog, char arg) {
	fprintf(stderr, "%s: invalid option \"-%c\"\n", prog, arg);
	suggest(prog);
}
void unexpected(const char *prog, const char *arg) {
	fprintf(stderr, "%s: unexpected option \"%s\"\n", prog, arg);
	suggest(prog);
}
