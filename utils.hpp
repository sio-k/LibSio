/*
  Basic types and utilities for this container library.
  Copyright (C) 2018 Sio Kreuzer

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <stdint.h>

namespace LibSio
{

typedef uint8_t byte;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

template< typename T >
void callDestructorIfExistent( T* x )
{
    x -> ~T();
}

template< typename T >
void callDestructorIfExistent( ... )
{
    // do nothing
}

// forcibly reinterpret some bits
template< typename dst, typename src >
dst reinterpret( src x )
{
    char tmp[sizeof( dst )];
    memcpy( tmp, &x, sizeof( dst ) );
    return *( ( dst* ) tmp );
}

}
