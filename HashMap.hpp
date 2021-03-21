/*
  Hashmap with guaranteed O(1) access for amd64 systems.
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
// size optimized
// uses:
// -> fibonacci secondary hash function (LCG to affect a better distribution in case of a bad primary hash function, multiplies the result of the primary hash function to distribute it within the 2^64 number space)
// -> robin hood bucket stealing
// -> cacheline sized and aligned buckets
// -> linear probing
// -> a probe limit of two cachelines (as adjacent cachelines usually get prefetched)
//    -> if probe limit is reached when inserting, table size is doubled
//    -> guarantees an element will be reachable with no more than two cache misses (most likely one, however)
// CONSTRAINTS:
// Key:
//   -> 64 % sizeof( Key ) == 0 (currently not enforced by compiler)
//   -> must be copy constructible
//   -> must set aside an "empty"/"null" value
// Value:
//   -> must be copy constructible
//
// Invariants:
//   -> for a given position i \in \{ 0 ... length() \} :
//       -> if the key located at keys()[i] is the empty key, then values()[i] does not contain anything (the value at that position has been destroyed/never constructed)
//       -> if the key located at keys()[i] is not the empty key, then values()[i] must contain a valid instance of V
//   -> if a key hashes to position i and is present in the map, it must be either within the cacheline of keys()[i] or the next cacheline (at insert, if this would not be the case, the size of the map is doubled)
//
// Performance characteristics:
//   -> in big-O notation:
//       -> insert: best O(1) average O(1) worst O(n)
//       -> enlarge: O(n) (doubling size, filling rest of keys in map with empty key)
//       -> remove: best O(1) average O(1) worst O(n)
//       -> get: O(1) (result is guaranteed to be within two cachelines of where it should be)
//       -> clear: O(n)
//       -> foreach: O(n)
//       -> construct: O(1)
//       -> destroy: O(n)
//   -> when using integer keys \in \{ 0 ... length() \}, a maximum load factor of >90% can be achieved
//       -> this is the general use case for most hashmaps in this engine, which map from id to a large object.
//   -> worst case load factor is <10% if inserting exclusively fibonacci numbers as keys and using id as the provided hash function

#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cassert>

#include "Optional.hpp"
#include "StaticHashMap.hpp"
#include "AlignedPointerContainer.hpp"

#include "utils.hpp"

#include <functional>

namespace LibSio
{

template< typename K, typename V
        , size_t ( *_hash )( const K* const x ) = standard_hash< K >
        , bool ( *eq )( const K* const a, const K* const b ) = standard_eq< K > >
struct HashMap
{
    typedef HashMap< K, V, _hash, eq > own_type;

    static const size_t initial_length = 7; // 2^7 = 128
    static const size_t no_index = __SIZE_MAX__;
    static const size_t probe_limit = ( 64 * 2 ) / sizeof( K ); // try to make sure a key is within two cachelines of where it should be

    // pointer-sized container; do not modify except using set_kvs(), set_length_power() and increase_length_power()
    // LSB to 5: size as a power of two, rest: pointer to kvs array
    //byte* underlying;
    AlignedPointerContainer< byte, size_t, 64 > underlying;
    K empty_key;

    // maximum length: 2^(2^6 - 1) = 2^63 (would fill more than the address space even if K = V = char)
    static const size_t length_power_mask = 0b111111;

    inline byte* kvs()
    {
        return underlying.ptr();
        //return ( byte* ) ( ( ( size_t ) underlying ) & ( ~length_power_mask ) );
    }

    inline void set_kvs( byte* x )
    {
        underlying.setPtr( x );
        //size_t lp = length_power();
        //underlying = ( byte* ) ( ( ( size_t ) x ) | lp );
    }

    inline size_t length_power()
    {
        return underlying.num();
        //return ( ( size_t ) underlying ) & length_power_mask;
    }

    inline void set_length_power( size_t x )
    {
        underlying.setNum( x );
        //underlying = ( byte* ) ( ( ( size_t ) kvs() ) | x );
    }

    inline void increase_length_power()
    {
        underlying.setNum( underlying.num() * 2 );
        //set_length_power( length_power() + 1 );
    }

    inline size_t length()
    {
        return 1 << length_power();
    }

    inline K* keys()
    {
        return ( K* ) kvs();
    }

    inline V* values()
    {
        return ( V* ) ( kvs() + key_arr_len_in_bytes< K, V >( length() ) );
    }

    // facility for probing backwards to the start of a cacheline
    static inline K* align_backwards_to_cacheline( K* x )
    {
        return ( K* ) ( ( ( intptr_t ) x ) & ( ~( ( intptr_t ) 0b111111 ) ) );
    }

    inline size_t hash( const K* const k )
    {
        return hash_secondary< K, _hash >( k ) % length();//& power_mask( length_power );
    }

  //private:
    // NOTE: due to only keeping the size as a power of two, size may only be doubled or halved
    // trigger for doubling size: insert/emplace goes over probe limit
    inline void double_size()
    {
        byte hm[sizeof( own_type )];
        ( ( own_type* ) hm ) -> underlying = underlying;
        new( &( ( ( own_type* ) hm ) -> empty_key ) ) K( empty_key );

        increase_length_power();//length_power++;

        set_kvs( ( byte* ) aligned_alloc( 64, overall_arr_len_in_bytes< K, V >( length() ) ) );
        for ( size_t i = 0; i < length(); i++ ) {
            new( keys() + i ) K( empty_key );
        }

        own_type* self = this;
        ( ( own_type* ) hm ) -> foreach_lambda(
            [=]
            ( const K* k, V* v )
            -> void
            {
                self -> insert( *k, *v );
            }
        );

        ( ( own_type* ) hm ) -> ~HashMap();
    }

    inline void kill_cell( size_t i )
    {
        kill_cell_unsafe( i );
        new( keys() + i ) K( empty_key );
    }

    inline void kill_cell_unsafe( size_t i )
    {
        callDestructorIfExistent< K >( keys() + i );
        callDestructorIfExistent< V >( values() + i );
    }

    size_t get_index_for_key( const K* const k )
    {
        const size_t hashed = hash( k );
        K* const probe_start = align_backwards_to_cacheline( keys() + hashed );
        K* const arr_end =  keys() + length();
        K* const probe_end = probe_start + probe_limit > arr_end
                           ? arr_end
                           : probe_start + probe_limit;
        for ( K* i = probe_start
            ;    i < probe_end
              && !eq( i, &empty_key )
            ; i++
            ) {
            if ( eq( i, k ) ) {
                return ( size_t ) ( ( ( ( intptr_t ) i ) - ( ( intptr_t ) keys() ) ) / sizeof( K ) );
            }
        }
        return no_index;
    }

    static inline ptrdiff_t distance( const size_t x, const size_t y )
    {
        return std::abs( ( ( ptrdiff_t ) x ) - ( ( ptrdiff_t ) y ) );
    }

    void reshuffle( const size_t start )
    {
        // TODO: may not work entirely correctly, write more tests
        if ( !eq( keys() + start, &empty_key ) ) {
            return;
        }

        // is there anything within the probe limit that wants to be in this cacheline? If so, start shuffling (might reshuffle literally every other element in the map, but that should be extremely rare as we are unlikely to reach the load factor required for that)
        const size_t probe_start = start;
        const size_t probe_end = start + probe_limit < length() ? start + probe_limit : length();
        for ( size_t i = probe_start + 1; i < probe_end && !eq( keys() + i, &empty_key ); i++ ) {
            const size_t hash_curr = hash( keys() + i );
            const ptrdiff_t dist_i = distance( i, hash_curr );
            const ptrdiff_t dist_start = distance( start, hash_curr );
            // test whether dist_start might still have element at i end up on the correct cacheline and whether start is at a lower distance than i
            if ( keys() + start >= align_backwards_to_cacheline( keys() + hash_curr )
                 && dist_i > dist_start && eq( keys() + i, &empty_key ) ) {
                callDestructorIfExistent< K >( keys() + i );
                new( keys() + i ) K( keys()[start] );
                new( values() + i ) V( values()[start] );
                callDestructorIfExistent< K >( keys() + start );
                callDestructorIfExistent< V >( values() + start );
                //memcpy( keys() + start, keys() + i, sizeof( K ) );
                //memcpy( values() + start, values() + i, sizeof( V ) );
                new( keys() + i ) K( empty_key );
                return reshuffle( i );
            }
        }
    }

    HashMap() = delete;

    HashMap( K _empty_key )
        : underlying()
        , empty_key( _empty_key )
    {
        set_length_power( initial_length );
        set_kvs( ( byte* ) aligned_alloc( 64, overall_arr_len_in_bytes< K, V >( length() ) ) );
        assert( kvs() );
        for ( size_t i = 0; i < length(); i++ ) {
            new( keys() + i ) K( empty_key );
        }
    }

    HashMap( HashMap& x )
        : HashMap( x.empty_key )
    {
        HashMap< K, V, _hash, eq >* self = this;
        x.foreach_lambda(
            [=]
            ( const K* k, V* v )
            -> void
            {
                self -> insert( *k, *v );
            }
        );
    }

    ~HashMap()
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !( eq( keys() + i, &empty_key ) ) ) {
                kill_cell_unsafe( i );
            } else {
                callDestructorIfExistent< K >( keys() + i );
            }
        }
        free( kvs() );
    }

    HashMap& operator=( HashMap& x )
    {
        this -> ~HashMap< K, V, _hash, eq >();
        new( this ) HashMap< K, V, _hash, eq >( x );
        return *this;
    }

    V* get_ref( const K& k )
    {
        auto index = get_index_for_key( &k );
        if ( index != no_index ) {
            return values() + index;
        } else {
            return nullptr;
        }
    }

    Optional< V > get( const K& k )
    {
        V* x = get_ref( k );
        if ( x ) {
            return Just< V >( *x );
        } else {
            return Nothing< V >();
        }
    }

    bool insert( const K& k, V& v )
    {
        return emplace( k, v );
    }

    // returns true on success, false if key already in map
    template< typename... Args >
    bool emplace( const K& k, Args... args )
    {
        if ( eq( &empty_key, &k ) ) {
            return false; // attempting to insert empty key
        }
        const size_t hashed = hash( &k );
        const K* probe_start = align_backwards_to_cacheline( keys() + hashed );
        const K* arr_end = keys() + length();
        const K* probe_end = probe_start + probe_limit > arr_end
                           ? arr_end
                           : probe_start + probe_limit;
        K* i = ( K* ) probe_start;
        for ( // nothing
            ;    i < probe_end
              && !eq( i, &empty_key )
            ; i++
            ) {
            if ( eq( i, &k ) ) {
                return false; // key already in map
            }
        }
        if ( i == probe_end ) {
            double_size();
            return emplace< Args... >( k, args... );
        } else {
            // found space at i
            size_t n = ( size_t ) ( ( ( intptr_t ) i ) - ( ( intptr_t ) keys() ) ) / sizeof( K );
            V* j = values() + n;

            callDestructorIfExistent< K >( i );
            new( i ) K( k );
            new( j ) V( args... );
            return true;
        }
    }

    void rm( const K& k )
    {
        auto index = get_index_for_key( &k );
        if ( index != no_index ) {
            kill_cell( index );
            reshuffle( index );
        }
    }

    void clear()
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !( eq( keys() + i, &empty_key ) ) ) {
                kill_cell( i );
            }
        }
    }

    struct foreach_frame
    {
        static const size_t frame_count = 1 << 4; // 16, 2 ^ 4 -> this must be a power of 2
        static const size_t area = probe_limit / 2;
        static const size_t stride = frame_count * area; // each frame deals with a cacheline's worth of keys
        size_t index = 0;
    };

    void foreach_value( void ( *fn )( V* value ) )
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( values() + i );
            }
        }
    }

    void foreach_value_lambda( std::function< void( V* ) > fn )
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( values() + i );
            }
        }
    }

    void foreach( void ( *fn )( const K* key, V* value ) )
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( keys() + i, values() + i );
            }
        }
    }

    void foreach_lambda( std::function< void( const K*, V* ) > fn )
    {
        for ( size_t i = 0; i < length(); i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( keys() + i, values() + i );
            }
        }
    }
};

}
