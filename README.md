A bunch of templatized containers in C++. Intended to be reasonably cache-friendly and of some use to... well, somebody at least. Mostly hashmaps. Generally "construct once at great expense" type stuff that hopefully has very quick accesses afterwards.

Typically intended to be filled at one point in time and then only accessed (see also: StaticHashMap).

The String template is intended as a deliberately more unfriendly variant of std::string that occupies only the size of a pointer.

Usage
-----

Headers only - just copy them into your project, include them and you ought to be good to go. Requires C++17 (preferably with GNU extensions; use either clang or gcc).

A few things to keep in mind:
* This library expects everything given it's containers has a copy constructor.
* The `HashMap` class, while still slightly better than `std::unordered_map` for general use, isn't meant for general use (even if it's name might suggest that).
* You may run into *serious* issues on non-x86_64 systems. I would like to fix these, but having written this a few years ago I'm not entirely sure which bits were particularly offensive in the first place.
* Address stability across insertions/deletions is not guaranteed by any of the hashmaps.
* None of this is intended to be thread safe out the gate.
* The interfaces presented have nothing to do with the C++ STL and everything to do with what I'd consider a useful interface. As such, a lot of elements you may usually be familiar with do not exist.
* This library should not throw any exceptions, ever. Failures are either signaled via return codes, or silently tolerated (as in the case where one removes an element that doesn't exist).
* `Optional` somewhat replicates the interface of Haskell's `Maybe` type.

Notable further work relating to `HashMap`
------------------------------------------

The [Odin programming language](https://odin-lang.org) has a built-in `map` type that appears to be heavily based on if not this `HashMap` implementation, then at least the same ideas for data layout, map access, cacheline alignment, struct size etc. Said implementation is much easier to understand, comprehensive, has considerably fewer restrictions and is of generally higher quality. It lacks "fibonacci hashing" (i.e. the LCG used here to deal with `std::hash` on integers often being implemented as the identity function), and uses a proper hash function instead to begin with. Should you wish to implement such a hashmap type, I heavily recommend referring to the Odin implementation instead of my implementation.

Copyright & License
-------------------

Copyright (C) 2018 - 2021 Sio Kreuzer.
Licensed under the terms of the GNU GPLv2+. See COPYING.txt for full license text.
