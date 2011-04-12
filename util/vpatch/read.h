/* 
 * Copyright (C) 2010 Chris McClelland
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
#include <types.h>

/*
 * Allocate a buffer big enough to fit file into, then read the file into it,
 * then write the file length to the location pointed to by 'length'. Naturally,
 * responsibility for the allocated buffer passes to the caller.
 */
uint8 *readFile(const char *name, uint32 *length);
