/*
    Optional type: may contain something, or not. Does not allocate.
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

#include <cstring>
#include <functional>

#include "utils.hpp"

namespace LibSio
{

// NOTE: owns T if it has been given one.
template< typename T >
struct Optional
{
    alignas( T ) char just[sizeof( T )];
    bool is_just;
    Optional()
        : is_just( false )
    {}

    Optional( T& x )
        : is_just( true )
    {
        new( &just ) T( x );
    }

    template< typename... Args >
    Optional( Args... args )
        : is_just( true )
    {
        ( ( T* ) just ) -> T( args... );
    }

    ~Optional()
    {
        if ( is_just ) {
            LibSio::callDestructorIfExistent< T >( ( T* ) just );
        }
    }

    // NOTE: potentially unsafe, could lead to memory leaks
    Optional< T >& operator=( Optional< T > x )
    {
        this -> ~Optional();
        is_just = x.is_just;
        new( just ) T( *( ( T* ) x.just ) );
        return *this;
    }

    operator bool() const
    {
        return is_just;
    }

    // monadic bind operator
    template< typename U >
    Optional< U > operator>>=( std::function< Optional< U >( T ) > fn )
    {
        if ( is_just ) {
            return fn( *( ( T* ) just ) );
        } else {
            return Optional< U >();
        }
    }

    // map function
    template< typename U >
    Optional< U > fmap( std::function< U( T ) > fn )
    {
        if ( is_just ) {
            return Optional< U >( fn( *( ( T* ) just ) ) );
        } else {
            return Optional< U >();
        }
    }
};

template< typename T >
Optional< T > Nothing()
{
    return Optional< T >();
}

template< typename T >
Optional< T > Just( T& x )
{
    return Optional< T >( x );
}

template< typename T >
bool isJust( Optional< T > const& x )
{
    return x;
}

template< typename T >
bool isNothing( Optional< T > const& x )
{
    return !x;
}

template< typename T >
T fromJust( Optional< T > x )
{
    return *( ( T* ) &( x.just ) );
}

}
