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
#ifndef ARGS_H
#define ARGS_H

#define GET_ARG(argName, var, failCode, label) \
	argv++; \
	argc--; \
	if ( !argc ) { requires(prog, argName); FAIL(failCode, label); } \
	var = *argv

void suggest(const char *prog);
void requires(const char *prog, const char *arg);
void missing(const char *prog, const char *arg);
void invalid(const char *prog, char arg);
void unexpected(const char *prog, const char *arg);
void usage(const char *prog);

#endif
