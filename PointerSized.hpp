/*
  Pointer-sized container holding two things of a prespecified size.
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
// pointer-sized container for two things of specified size, or one thing of specified size, with the second filling the rest. Essentially a pointer-sized tuple.
#pragma once

#include <cstdlib>
#include <cstring>

#include "utils.hpp"

namespace LibSio
{

namespace detail
{

template< size_t n >
struct ProduceMask
{
    static const size_t value = ( 1ull << n ) | ProduceMask< n - 1ull >::value;
};

template<>
struct ProduceMask< 0 >
{
    static const size_t value = 1ull;
};

}

// TODO: may not work correctly, write tests
// sizes are in bits, A and B must be POD types, size_a must be smaller than sizeof( size_t ) * 8
// NOTE: assuming 1 Byte == 8 bits
template< typename A, size_t size_a, typename B, size_t size_b = ( sizeof( size_t ) * 8ull ) - size_a >
struct PointerSized
{
    static const size_t larger_size = sizeof( A ) > sizeof( B )
                                    ? sizeof( A )
                                    : sizeof( B )
                                    ;
    static const size_t fst_mask = detail::ProduceMask< size_a - 1ull >::value;
    static const size_t snd_mask = detail::ProduceMask< ( sizeof( size_t ) * 8ull ) - 1ull >::value & ~fst_mask;

    size_t underlying;

    PointerSized()
        : underlying( 0ull )
    {}

    PointerSized( A x, B y )
        : underlying( 0ull )
    {
        setFst( x );
        setSnd( y );
    }

    // destructor omitted

    void setFst( A x )
    {
        underlying = ( 0ull | ( underlying & snd_mask ) )
                   | reinterpret< size_t >( x );
    }

    void setSnd( B x )
    {
        underlying = ( 0ull | ( underlying & fst_mask ) )
                   | ( reinterpret< size_t >( x ) << size_a );
    }

    A fst()
    {
        A x = reinterpret< A >( underlying & fst_mask );
        return x;
    }

    B snd()
    {
        B x = reinterpret< B >( ( underlying & snd_mask ) >> size_a );
        return x;
    }
};

}
