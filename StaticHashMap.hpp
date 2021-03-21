/*
    Statically sized hashmap using open addressing
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
// Efficient cache-friendly dense (as dense as possible, anyway) statically sized hashmap implementation using linear probing.
// Does not use robin hood hashing - pointers this table hands out are valid for as long as the table contains the element the pointer points to
#pragma once

#include <cstring>
#include <cassert>

#include "Optional.hpp"
#include "utils.hpp"

#include <functional>

#define inline

namespace LibSio
{

template< typename T >
bool standard_eq( const T* const a, const T* const b )
{
    return ( *a ) == ( *b );
}

template< typename T >
size_t standard_hash( const T* const x )
{
    std::hash< T > h;
    return h( *x );
}

template< typename K, typename V >
inline size_t key_arr_len_in_bytes( size_t length )
{
    return ( length * sizeof( K ) )
         + ( ( length * sizeof( K ) ) % alignof( V ) );
}

template< typename K, typename V >
inline size_t overall_arr_len_in_bytes( size_t length )
{
    return key_arr_len_in_bytes< K, V >( length ) + ( length * sizeof( V ) );
}

template< typename K, size_t ( *_hash )( const K* const x ) >
inline size_t hash_secondary( const K* const x )
{
    const size_t hash_multiplier = 11400714819323198485LU; // derived from golden ratio, not ideal - repeated patterns in form of fibonacci sequence
    return ( _hash( x ) * hash_multiplier );
}

template< typename K
        , typename V
        , size_t ( *_hash )( const K* const x ) = standard_hash< K >
        , bool ( *eq )( const K* const a, const K* const b ) = standard_eq< K >
        >
struct StaticHashMap
{
    typedef StaticHashMap< K, V, _hash, eq > own_type;

    typedef unsigned char byte;
    constexpr static const size_t no_index = __SIZE_MAX__;

    // improves the distribution in case of a bad hash function (which std::hash on integers typically is)
    static inline size_t hash( const K* const x, const size_t map_length )
    {
        return hash_secondary< K, _hash >( x ) % map_length;
    }

    size_t key_arr_len_in_bytes()
    {
        return LibSio::key_arr_len_in_bytes< K, V >( length );
    }

    size_t overall_arr_len_in_bytes()
    {
        return LibSio::overall_arr_len_in_bytes< K, V >( length );
    }

    byte* kvs;
    size_t length;
    K empty_key;

    K* keys()
    {
        return ( K* ) kvs;
    }

    V* values()
    {
        return ( V* ) ( kvs + key_arr_len_in_bytes() );
    }

    StaticHashMap() = delete;

    StaticHashMap( const own_type& x )
        : StaticHashMap( const_cast< own_type* >( &x ) )
    {}

    StaticHashMap( own_type* x )
        : kvs( nullptr )
        , length( x -> length )
        , empty_key( x -> empty_key )
    {
        assert( length );
        kvs = ( byte* ) calloc( 1, overall_arr_len_in_bytes() );
        assert( kvs );

        for ( size_t i = 0; i < length; i++ ) {
            new( keys() + i ) K( empty_key );
        }

        own_type* self = this;
        x -> foreach_lambda(
            [=]
            ( const K* key, V* value )
            -> void
            {
                self -> insert( *key, *value );
            }
        );
    }

    StaticHashMap( size_t a_length
                 , K an_empty_key
                 )
        : kvs( nullptr )
        , length( a_length )
        , empty_key( an_empty_key )
    {
        assert( length );
        kvs = ( byte* ) calloc( 1, overall_arr_len_in_bytes() );
        assert( kvs );

        for ( size_t i = 0; i < length; i++ ) {
            new( keys() + i ) K( empty_key );
        }
    }

    ~StaticHashMap()
    {
        if ( keys() && values() ) {
            for ( size_t i = 0; i < length; i++ ) {
                if ( eq( keys() + i, &empty_key ) ) {
                    callDestructorIfExistent< K >( keys() + i );
                    // no value to destroy
                } else {
                    callDestructorIfExistent< K >( keys() + i );
                    callDestructorIfExistent< V >( values() + i );
                }
            }
        }
        if ( kvs ) {
            free( kvs );
        }
    }

    own_type& operator=( own_type x )
    {
        this -> ~StaticHashMap();
        new( this ) StaticHashMap< K, V, _hash, eq >( x );
        return *this;
    }

    // get index for key - key must already be paired with a value in the map
    // index is in elements (length, not size)
    size_t get_index_for_key( const K* const k )
    {
        size_t hashed = hash( k, length );
        for ( size_t i = 0; i < length; i++ ) {
            size_t index = ( hashed + i ) % length;
            if ( eq( keys() + index, k ) ) {
                return index;
            }
        }
        return no_index;
    }

    // get a reference into the map (returns NULL on failure, does not traverse values)
    V* get_ref( const K& k )
    {
        auto index = get_index_for_key( &k );
        if ( index != no_index ) {
            return values() + index;
        } else {
            return nullptr;
        }
    }

    Optional< V > get( const K& x )
    {
        auto el = get_ref( x );
        if ( el ) {
            return Just< V >( *el );
        } else {
            return Nothing< V >();
        }
    }

    // get an index at which there is nothing for this key (if a key/value pair with a key comparing equal to this key is found, returns no_index)
    // index is in elements (length, not size)
    size_t get_new_index_for_key( const K* const k )
    {
        size_t hashed = hash( k, length );
        for ( size_t i = 0; i < length; i++ ) {
            size_t index = ( hashed + i ) % length;
            if ( eq( keys() + index, &( empty_key ) ) ) {
                return index;
            } else if ( i == length - 1 ) {
                return no_index - 1;
            }
        }
        return no_index;
    }

    // may fail if there is no space left in the hashmap or key is already in hashmap
    // returns false on failure
    bool insert( const K& k, const V& v )
    {
        auto index = get_new_index_for_key( &k );
        if ( index == no_index ) {
            return true; // key already in map
        } else if ( index == no_index - 1 ) {
            return false; // no space left
        } else {
            new( keys() + index ) K( k );
            new( values() + index ) V( v );
            return true;
        }
    }

    // may fail if there is no space left in the hashmap
    // will simply do nothing and return true if key already in map
    // returns false on failure
    template< typename... Args >
    bool emplace( const K& k, Args... args )
    {
        auto index = get_new_index_for_key( &k );
        if ( index == no_index ) {
            return true; // key already in map
        } else if ( index == no_index - 1 ) {
            return false; // no space left
        } else {
            callDestructorIfExistent< K >( keys() + index );
            new( keys() + index ) K( k );
            new( values() + index ) V( args... );
            return true;
        }
    }

    void rm( const K& k )
    {
        auto index = get_index_for_key( &k );
        if ( index != no_index ) {
            callDestructorIfExistent< K >( keys() + index );
            callDestructorIfExistent< V >( values() + index );
            new( keys() + index ) K( empty_key );
        }
    }

    void clear()
    {
        for ( size_t i = 0; i < length; i++ ) {
            if ( !eq( keys() + i, &( empty_key ) ) ) {
                rm( *( keys() + i ) );
            }
        }
    }

    Optional< V > pop( const K& k )
    {
        auto index = get_index_for_key( &k );
        if ( index != no_index ) {
            Optional< V > toreturn = Just< V >( values()[index] );
            rm( k );
            return toreturn;
        } else {
            return Nothing< V >();
        }
    }

    // TODO: const foreach
    // TODO: fold
    // TODO: traverse
    // TODO: map (writing out to a new hashmap)

    // NOTE: *insanely* fast when compared to Iterator,
    // faster than foreach_value_lambda
    void foreach_value( void ( *fn )( V* value ) )
    {
        for ( size_t i = 0; i < length; i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( values() + i );
            }
        }
    }

    // NOTE: *significantly* faster than Iterator
    void foreach_value_lambda( std::function< void( V* ) > fn )
    {
        for ( size_t i = 0; i < length; i++ ) {
            if ( !eq( keys() + i, &empty_key ) ) {
                fn( values() + i );
            }
        }
    }

    void foreach( void ( *fn )( const K* key, V* value ) )
    {
        for ( size_t i = 0; i < length; i++ ) {
            if ( !eq( keys() + i, &empty_key ) ){
                fn( keys() + i, values() + i );
            }
        }
    }

    void foreach_lambda( std::function< void( const K*, V* ) > fn )
    {
        for ( size_t i = 0; i < length; i++ ) {
            if ( !eq( keys() + i, &empty_key ) ){
                fn( keys() + i, values() + i );
            }
        }
    }

    bool empty()
    {
        bool is = true;
        foreach_value_lambda(
          [&]
          ( __attribute__((unused)) V* _ )
          -> void
          {
              is = false;
          }
        );
        return is;
    }

    // count of elements in container
    size_t count()
    {
        size_t n = 0;
        foreach_value_lambda(
            [&]
            ( __attribute__((unused)) V* _ )
            -> void
            {
                n++;
            }
        );
        return n;
    }
};

}
