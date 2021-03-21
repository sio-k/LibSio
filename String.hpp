/*
    Pointer-sized string class for strings you don't do much with.
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

#include "utils.hpp"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <locale>
#include <codecvt>

namespace LibSio
{

template< typename C >
struct StringT
{
  private:
    static size_t strlen( const C* const x )
    {
        char null[sizeof( C )];
        memset( null, 0, sizeof( C ) );
        size_t n = 0;
        for ( ; 0 != memcmp( x + n, null, sizeof( C ) ); n++ );
        return n;
    }

    constexpr static const C emptystr {};
  public:
    C* str; // NULL-terminated underlying string

    StringT()
        : StringT( ( const C* const ) &emptystr )
    {}

    StringT( const C* const x )
        : str( nullptr )
    {
        size_t len = strlen( x );
        str = ( C* ) calloc( len + 1, sizeof( C ) );
        for ( size_t i = 0; i < len; i++ ) {
            new( str + i ) C( x[i] );
        }
        //memcpy( str, x, len * sizeof( C ) );
        new( str + len ) C( emptystr );
        //memset( str + len, 0, sizeof( C ) );
    }

    StringT( C* const x )
        : StringT( ( const C* const ) x )
    {}

    StringT( const StringT< C >& x )
        : StringT( ( const C* const ) x.str )
    {}

    ~StringT()
    {
        free( ( void* ) str );
        str = nullptr;
    }

    size_t length() const
    {
        return strlen( str );
    }

    // returns the underlying array
    C* c_str() const
    {
        return str;
    }

    operator C* () const
    {
        return c_str();
    }

    StringT< C >& operator=( const StringT< C >& x )
    {
        this -> ~StringT< C >();
        new( this ) StringT< C >( x );
        return *this;
    }

    StringT< C >& operator=( const C* const x )
    {
        this -> ~StringT< C >();
        new( this ) StringT< C >( x );
        return *this;
    }

    StringT< C >& operator=( C* const x )
    {
        return ( *this = ( const C* const ) x );
    }

    StringT< C >& operator+=( const StringT< C >& x )
    {
        *this = *this + x;
        return *this;
    }

    StringT< C > operator+( const StringT< C >& x ) const
    {
        const size_t len = length();
        const size_t xlen = x.length();
        C* nstr = ( C* ) calloc( alignof( C )
                               , ( len + xlen + 1 ) * sizeof( C )
                               );
        memcpy( nstr, str, len * sizeof( C ) );
        memcpy( nstr + len, x.str, xlen );
        memset( nstr + len + xlen, 0, sizeof( C ) );
        StringT< C > y( nstr );
        free( nstr );
        return y;
    }

    bool operator==( const StringT< C >& x ) const
    {
        size_t xlen = x.length();
        size_t ownlength = length();
        if ( xlen != ownlength ) {
            return false;
        }

        return 0 == memcmp( c_str(), x.c_str(), ownlength );
    }

    bool operator!=( const StringT< C >& x ) const
    {
        return ! ( *this ==  x );
    }

    // will return less if asking for something past end and nothing if asking for start > end and start > length
    StringT< C > take( size_t start, size_t end ) const
    {
        if ( end <= length() ) {
            // alright
        } else {
            end = length();
        }

        if ( start > end ) {
            return "";
        }

        C tmp[end - start + 1];
        memset( &tmp, 0, ( end - start + 1 ) * sizeof( C ) );
        memcpy( &tmp, str + start, end - start );

        StringT< C > toreturn( tmp );
        return toreturn;
    }
};

typedef StringT< char > String;
typedef StringT< char32_t > UTF32LEString;

}

template< typename C >
struct std::hash< LibSio::StringT< C > >
{
    size_t operator()( const LibSio::StringT< C >& x ) const
    {
        std::hash< basic_string< C > > h;
        return h( basic_string< C >( x.c_str() ) );
    }
};
