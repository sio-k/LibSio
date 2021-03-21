/*
  Container that is guaranteed to be the size of a single 64-bit pointer.
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
// container containing an aligned pointer and an unsigned integer
#pragma once

#include <cstdlib>
#include <cstring>
#include <cassert>

#include "PointerSized.hpp"

namespace LibSio
{

namespace detail
{

template< size_t align >
struct AlignmentBits
{
    constexpr static const size_t value = 1ull + AlignmentBits< align / 2ull >::value;
};

template<>
struct AlignmentBits< 1 >
{
    constexpr static const size_t value = 0ull;
};

}
// TODO: may not work correctly, write tests
// alignment must be a power of 2
template< typename T, typename inttype, size_t alignment = 64 >
struct AlignedPointerContainer
{
    static const size_t align_bits = detail::AlignmentBits< alignment >::value;
    typedef PointerSized< inttype, align_bits, intptr_t > underlying_t;

    underlying_t underlying;

    AlignedPointerContainer()
        : underlying { 0, 0 }
    {}

    AlignedPointerContainer( T* x, inttype y )
        : underlying { y, ( intptr_t ) x }
    {}

    // destructor omitted

    T* ptr()
    {
        return ( T* ) ( underlying.snd() << align_bits );
    }

    operator T*()
    {
        return ptr();
    }

    inttype num()
    {
        return underlying.fst();
    }

    void setPtr( T* x )
    {
        // verify alignment
        assert( ( ( ( intptr_t ) x ) & underlying_t::fst_mask ) == 0 );
        underlying.setSnd( ( ( intptr_t ) x ) >> align_bits );
    }

    void setNum( inttype x )
    {
        underlying.setFst( x );
    }
};

}
