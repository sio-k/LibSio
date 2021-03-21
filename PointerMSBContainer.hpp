/*
  Container (mis)using the fact that canonical form of x86 addresses has all bits after 47 be a copy of bit 47
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

#include "utils.hpp"

namespace LibSio
{

namespace detail
{

template< bool is_signed >
struct SignednessDeciderForShort
{
    typedef u16 value;
};

template<>
struct SignednessDeciderForShort< true >
{
    typedef i16 value;
};

template< typename inttype, size_t significant_bytecount >
struct BitmaskConstructor
{
    static const inttype value = ( 1 << ( significant_bytecount - 1 ) )
                               | BitmaskConstructor< inttype, significant_bytecount - 1 >::value;
};

template< typename inttype >
struct BitmaskConstructor< inttype, 0 >
{
    static const inttype value = 0;
};

};

// TODO: autodetection of ARM32/ARM64 and IA-32 for proper support of those
// TODO: use something that isn't a short maybe?
template< typename T, bool is_signed = false, typename intptr_type = u64, size_t significant_bits = 48 >
struct PointerMSBContainer
{
    // bitmask to get significant bits
    static const intptr_type bitmask = detail::BitmaskConstructor< intptr_type, significant_bits >::value;
    static const size_t insignificant_bits = ( sizeof( intptr_type ) * 8 ) - significant_bits;

    intptr_type underlying;

    typedef detail::SignednessDeciderForShort< is_signed >::value inttype;

    intptr_type pad_pointer_according_to_last_significant_bit()
    {
        // pad out pointer according to it's bit 47
        static const intptr_type lsbit_mask = ( 1 << ( significant_bits - 1 ) );
        // whether we're in high or low memory
        const intptr_type high_or_low = ( lsbit_mask & underlying >> ( significant_bits - 1 ) );
        const intptr_type padding = ( ~bitmask ) * high_or_low;
        const intptr_type pointer = underlying & bitmask;
        return pointer | padding;
    }

    PointerMSBContainer()
        : PointerMSBContainer( nullptr, 0 )
    {}

    PointerMSBContainer( T* x, inttype integer )
    {
        setPtr( x );
        setNum( integer );
    }

    // destructor omitted

    T* ptr()
    {
        return ( T* ) pad_pointer_according_to_last_significant_bit();
    }

    void setPtr( T* const x )
    {
        const intptr_type number = ( ( intptr_type ) num() ) << significant_bits;
        const intptr_type top_zeroed_off = ( ( intptr_type ) x ) & bitmask;
        underlying =  top_zeroed_off | number;
    }

    inttype num()
    {
        return ( inttype ) ( ( underlying & ( ~bitmask ) ) >> significant_bits );
    }

    void setNum( const inttype x )
    {
        const intptr_type pointer = underlying & bitmask;
        const intptr_type number = ( ( intptr_type ) x ) << significant_bits;
        underlying = pointer | number;
    }

    operator T*()
    {
        return ptr();
    }
};

}
