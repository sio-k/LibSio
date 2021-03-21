/*
  x86_64 fat pointer implementation that fits into the space of a single pointer.
  Copyright (C) 2018  Sio Kreuzer

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

#include "AlignedPointerContainer.hpp"
#include "PointerMSBContainer.hpp"

namespace LibSio
{

/*
 * Pointer-sized fat pointer implementation. Aligned to cacheline boundaries.
 * Maximum array size 2^22.
 *
 * Comes out of allocators. Meant to immediately decay into pointer, as what you're giving back to allocators should only be the original fat pointer.
 */
template< typename T >
struct FatPointer
{
    AlignedPointerContainer< T, size_t, 64 > underlying;
    PointerMSBContainer< T > msbcontainer()
    {
        return ( PointerMSBContainer< T > ) ( underlying.ptr() );
    }

    FatPointer() = delete;

    FatPointer( T* x, size_t n )
        : underlying { x, n }
    {}

    FatPointer( FatPointer& x )
        : underlying( x.underlying )
    {}

    // destructor omitted

    T* ptr()
    {
        return msbcontainer().ptr();
    }

    size_t length()
    {
        return underlying.num() | ( msbcontainer().num() << 6);
    }

    size_t size()
    {
        return length() * sizeof( T );
    }

    // zeroes out memory region fat pointer points to
    void zero()
    {
        memset( ptr(), 0, size() );
    }

    T& operator*()
    {
        return ptr()[0];
    }

    T& operator[]( size_t n )
    {
        return ptr()[n];
    }

    T* operator->()
    {
        return ptr();
    }

    T* operator+( size_t n )
    {
        return ptr() + n;
    }

    // NOTE: operators +=, -=, -, /, new, delete etc. intentionally ommited/not overridden.

    // NOTE: returns void to prevent operator= being used as anything but a statement.
    void operator=( FatPointer& x )
    {
        memcpy( &underlying, &( x.underlying ), sizeof( underlying ) );
    }

    bool operator==( FatPointer& x )
    {
        return ptr() == x.ptr() && length() == x.length();
    }

    bool operator!=( FatPointer& x )
    {
        return !( this -> operator==( x ) );
    }

    operator T*()
    {
        return ptr();
    }
};

}
