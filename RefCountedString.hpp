/*
  Simple reference counting string implementation for C++.
  Copyright (C) 2021 Sio Kreuzer

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

#include <string.h>

namespace LibSio
{

struct RefCountedString
{
    char const* const str;

    uint64_t& refcount()
    {
        return *( reinterpret_cast< uint64_t* >( const_cast< char* >( str ) - sizeof( uint64_t ) ) );
    }

    RefCountedString() = delete;

    RefCountedString( char const* s )
        : str { nullptr }
    {
        // this is all kinda hacky because we want to compute the length only once if possible (that is, we walk the input two times and not more than that)
        size_t const length = strlen( s );
        char* const _str =
    reinterpret_cast< char* >( calloc( length + 1 + sizeof( uint64_t ), sizeof( char ) ) ) + sizeof( uint64_t );

        // copy to str
        strncpy( _str, s, length );

        // set str (hacky)
        memcpy( const_cast< char** >( &str ), &_str, sizeof( char* ) );

        ref();
    }

    RefCountedString( RefCountedString const& rcs )
        : str { rcs.str }
    {
        ref();
    }

    ~RefCountedString()
    {
        unref();
        if ( refcount() == 0 ) {
            fprintf( stdout, "refcount == 0, deallocating string %s\n", str );
            free( reinterpret_cast< void* >( const_cast< char* >( str ) - sizeof( uint64_t ) ) );
        }
    }

    void ref()
    {
        refcount()++;
    }

    void unref()
    {
        if ( refcount() > 0 ) {
            refcount()--;
        }
    }

    char const* c_str()
    {
        return str;
    }

    RefCountedString& operator=( RefCountedString const& x )
    {
        this -> ~RefCountedString();
        new( this ) RefCountedString( x );
        return *this;
    }
};

}
