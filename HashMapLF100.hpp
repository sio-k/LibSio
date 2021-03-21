/*
  Hashmap with a guaranteed load factor of 100% for fast access; consider not using insert/emplace/rm at all.
  Copyright (C) 2018-2021 Sio Kreuzer

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
// hashmap with a constant 100 percent load factor. Access O(n) max., all other operations O(n)

#include "StaticHashMap.hpp"

namespace LibSio
{

template< typename K
        , typename V
        , size_t ( *_hash )( const K* const x ) = standard_hash< K >
        , bool ( *eq )( const K* const a, const K* const b ) = standard_eq< K >
        >
struct HashMapLF100
{
    StaticHashMap< K, V, _hash, eq > underlying;

    HashMapLF100() = delete;

    HashMapLF100( K empty_key )
        : underlying( 1, empty_key )
    {}

    HashMapLF100( HashMapLF100& x )
        : underlying( x.underlying )
    {}

    HashMapLF100( K empty_key, K* keys, V* values, size_t count )
        : underlying( count, empty_key )
    {
        for ( size_t i = 0; i < count; i++ ) {
            underlying.emplace( keys[i], values[i] );
        }
    }

    HashMapLF100( K empty_key, std::vector< std::pair< K, V > > kvs )
        : underlying( kvs.size(), empty_key )
    {
        for ( std::pair< K, V >& kv : kvs ) {
            underlying.emplace( kv.first, kv.second );
        }
    }

    ~HashMapLF100()
    {}

    HashMapLF100& operator=( HashMapLF100& x )
    {
        underlying = x.underlying;
        return *this;
    }

    void increase_size_by_1()
    {
        StaticHashMap< K, V, _hash, eq > x( underlying );
        underlying = StaticHashMap< K, V, _hash, eq >( x.length + 1, x.empty_key );
        x.foreach_lambda(
            [&]
          ( const K* k, V* v )
          -> void
          {
              insert( *k, *v );
          }
        );
    }

    void decrease_size_by_1()
    {
        StaticHashMap< K, V, _hash, eq > x( underlying );
        underlying = StaticHashMap< K, V, _hash, eq >( x.length - 1, x.empty_key );
        x.foreach_lambda(
            [&]
            ( const K* k, V* v )
            -> void
            {
                insert( *k, *v );
            }
        );
    }

    V* get_ref( const K& k )
    {
        return underlying.get_ref( k );
    }

    Optional< V > get( const K& k )
    {
        return underlying.get( k );
    }

    bool insert( const K& k, const V& v )
    {
        if ( !empty() ) {
            increase_size_by_1();
        }
        return underlying.insert( k, v );
    }

    template< typename... Args >
    bool emplace( const K& k, Args... args )
    {
        if ( !empty() ) {
            increase_size_by_1();
        }
        return underlying.emplace( k, args... );
    }

    void rm( const K& k )
    {
        size_t n = count();
        underlying.rm( k );
        if ( count() != n ) {
            decrease_size_by_1();
        }
    }

    void clear()
    {
        ( *this ) = HashMapLF100( underlying.empty_key );
    }

    void foreach_value( void ( *fn )( V* ) )
    {
        underlying.foreach_value( fn );
    }

    void foreach_value_lambda( std::function< void( V* ) > fn )
    {
        underlying.foreach_value_lambda( fn );
    }

    void foreach( void ( *fn )( const K*, V* ) )
    {
        underlying.foreach( fn );
    }

    void foreach_lambda( std::function< void( const K*, V* ) > fn )
    {
        underlying.foreach_lambda( fn );
    }

    bool empty()
    {
        return underlying.empty();
    }

    size_t count()
    {
        return underlying.count();
    }
};

}
