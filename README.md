<img src="assets/jd-logo.png" width="96" alt="JD logo">

# c-data-structures

[![ci](https://github.com/AyerPrabin/c-data-structures/actions/workflows/ci.yml/badge.svg)](https://github.com/AyerPrabin/c-data-structures/actions/workflows/ci.yml)

A handful of basic containers written from scratch in C11, with no libraries
other than the C standard library:

- `src/vector.c` - dynamic array (growable, fixed-size elements)
- `src/list.c` - singly and doubly linked lists
- `src/hashmap.c` - hash map with string keys, open addressing, linear probing
  and tombstones
- `src/heap.c` - binary min-heap used as a priority queue
- `src/bst.c` - binary search tree (not balanced) with an in-order iterator

## Why

I use lists, dictionaries and sorted maps every day in Python and
JavaScript without thinking about what they are made of. I wanted to know
how they actually work: what happens when a dynamic array runs out of room,
why deleting from a hash table is harder than inserting, and why everyone
says a plain binary search tree is a bad idea. Writing them in C makes you
deal with every allocation yourself, which is where most of the interesting
mistakes are.

This is a learning project. The real libraries (glibc, the C++ STL, CPython)
are much more careful and much faster.

## Building

You need `make` and gcc or clang on Linux (or any POSIX system).

```sh
make            # builds test_ds and bench_ds
make test       # unit tests
make sanitize   # unit tests built with -fsanitize=address,undefined
make valgrind   # unit tests under Valgrind (needs valgrind installed)
make bench      # lookup benchmark
make CC=clang test
```

Everything is compiled with `-std=c11 -Wall -Wextra -Werror -pedantic`.

## API style

All five containers follow the same pattern so they are easy to remember:

- The struct lives on the caller's side (`Vec v; vec_init(&v, sizeof(int));`)
  and `*_free` releases whatever the container allocated.
- Functions that can allocate return `0` (or `1` for "added a new key") on
  success and `-1` if `malloc` failed, and leave the container unchanged.
- Lookups return `1` / `0` for found / not found and write the value through
  an optional out pointer, so a stored `NULL` value is not confused with
  "missing".

Ownership rules, which are also written at the top of each header:

| Container | Owns | Does not own |
|-----------|------|--------------|
| `Vec` | the element bytes (copied in with `memcpy`) | anything an element points to |
| `SList` / `DList` | the nodes | the `void *` data (use `*_free_with(l, free)` if it was malloc'd) |
| `HashMap` | a copy of every key string | the values |
| `Heap` | the item array | the `void *` data |
| `Bst` | the nodes | keys and values (they must outlive the tree) |

## Complexity

| Operation | Vec | SList | DList | HashMap | Heap | Bst |
|-----------|-----|-------|-------|---------|------|-----|
| push / insert | O(1) amortised at the end, O(n) in the middle | O(1) front and back | O(1) anywhere, given a node | O(1) average | O(log n) | O(h) |
| lookup | O(1) by index | O(n) | O(n) | O(1) average | O(1) peek at min | O(h) |
| remove | O(1) at the end, O(n) in the middle | O(1) front, O(n) by value | O(1) given a node | O(1) average | O(log n) pop min | O(h) |
| ordered walk | - | - | - | no (slot order) | no | O(n) |

`h` is the height of the tree: about log2(n) when keys arrive in random
order, but n in the worst case (keys inserted already sorted).

## Design choices

### Vector growth

The capacity doubles when the array is full (starting at 4). Each element is
copied at most a constant number of times on average, so `vec_push` is O(1)
amortised. Pushing 1000 ints causes 9 reallocations (4, 8, ..., 1024), and
there is a test for exactly that.

### Why open addressing for the hash map

I could have used separate chaining (an array of linked lists), which is
simpler. I went with open addressing because:

- all the entries sit in one array, so a lookup usually touches one or two
  cache lines instead of following list pointers around the heap;
- there is one allocation for the table instead of one per entry;
- deletion is the tricky part, and I wanted to understand it.

Linear probing means that if slot `i` is taken, try `i+1`, `i+2`, and so on
(wrapping around). The capacity is always a power of two, so `hash & (cap-1)`
replaces the slower `%`. Keys are hashed with 64-bit FNV-1a, and the full hash
is stored in each slot so most non-matching slots are skipped without calling
`strcmp`.

### How tombstones work

With linear probing you cannot just empty a slot on delete. Say `a`, `b` and
`c` all hash to slot 3, so they end up in slots 3, 4 and 5. If deleting `b`
made slot 4 empty again, a lookup for `c` would start at 3, hit the empty
slot 4 and stop, wrongly reporting that `c` is missing.

So a deleted slot is marked as a *tombstone* instead:

- lookups treat a tombstone as "keep going";
- inserting a new key reuses the first tombstone on its probe run, but only
  after checking the whole run, so a key that already exists further along is
  replaced instead of duplicated (the tests build this exact case with keys
  that collide on purpose);
- tombstones count towards the load factor, because they make probe runs
  just as long as live entries do.

The table is rebuilt when live entries plus tombstones would go over 70% of
the capacity. The rebuild moves the live entries into a new array sized so it
is at most half full, which also throws all the tombstones away. If the load
was mostly tombstones, the capacity stays the same (or even shrinks), so a
loop that keeps adding and removing different keys does not make the table
grow forever. There is a test that does 5000 add/remove pairs and checks the
capacity never goes above 8.

Keeping the load at or under 0.7 also guarantees there is always at least one
empty slot, which is what makes every probe loop terminate.

### Heap

A binary heap stored in a plain array: the children of index `i` are at
`2i+1` and `2i+2`. Push adds at the end and sifts up, pop moves the last item
to the root and sifts down. `heap_from_array` builds a heap from an unordered
array in O(n) by sifting down every parent from the bottom up (Floyd's
method), which is what a heapsort would use. The heap is not stable: equal
priorities can come out in any order.

### Binary search tree, and why it is not balanced

The BST is a plain one. Insert, find and remove are loops rather than
recursion, removing a node with two children copies in its in-order successor,
and `bst_free` frees the tree by rotating it into a list as it goes, so none
of them need a stack. The in-order iterator keeps an explicit stack (a `Vec`
of node pointers) instead of recursing, so it can stop half way and be
resumed.

I did not balance it. A red-black or AVL tree is a project of its own, and I
wanted to see the problem a balanced tree solves before solving it. The
benchmark below shows it clearly: with shuffled keys the tree height for
100,000 keys is 43, but if the keys arrive sorted the tree is just a linked
list with height n, and lookups become slower than a plain linear scan.
Because everything is iterative, the 20,000-deep tree in the tests is slow
but does not crash.

## Benchmark

`bench/bench.c` builds an array, a BST and a hash map from the same string
keys (`key00000042` style), then times 20,000 lookups in each, half for
keys that exist and half for keys that do not. The times are wall-clock
nanoseconds per lookup from a single run, so only the order of magnitude
means anything.

One run of `make bench` in CI (GitHub's ubuntu-24.04 runner, gcc, `-O2`):

```
       n  insert         linear        bst   height    hashmap
     100  shuffled          315         56       11         20
    1000  shuffled         3844        130       24         23
   10000  shuffled        41121        252       29         41
  100000  shuffled       420894        522       43        102
    1000  sorted           2996       7919     1000         23
   10000  sorted          29042      78298    10000         38
```

What I take from it:

- Linear search grows with n, as expected, and is already about 15x slower
  than the hash map at 100 keys.
- The BST grows slowly (log n) when the keys are shuffled, but the hash map
  stays roughly flat until the table gets too big for the CPU cache.
- Inserted in sorted order, the BST is worse than linear search. Each step is
  a pointer chase to a node somewhere else in memory, while the linear scan
  walks one array.
- Linear search on the sorted rows is faster than on the shuffled rows with
  the same n. The strings were allocated in order, so walking them in order
  is kinder to the cache. I had not expected that before I saw it.

## Tests

`make test` runs `tests/`, which is plain C with three small checking macros
in `tests/check.h` (no framework). The runner prints a count per container:

```
vector     37 checks, 0 failed
list       41 checks, 0 failed
hashmap    59 checks, 0 failed
heap       25 checks, 0 failed
bst        41 checks, 0 failed
total     203 checks, 0 failed
```

Some of the cases covered: every operation on an empty container, growth and
the exact reallocation count of the vector, removing the head/tail of a
linked list (and checking `tail` is still right after), duplicate keys,
resize at the 0.7 load factor, deletes that leave tombstones in the middle of
a collision chain, re-inserting past a tombstone, tombstone churn, 20,000
keys with half deleted, a heap checked against sorted order with random
input, heapify, all three BST removal cases, stopping an iterator early, and
a 20,000-node degenerate tree.

CI runs the tests with gcc and clang, again with AddressSanitizer and
UBSan, and once more under Valgrind with full leak checking.

## Limitations

- The hash map only takes C string keys. A generic version would need a
  hash and an equality function passed in, like the BST's comparator.
- Nothing is thread-safe.
- The containers store `void *`, so there is no type checking on values.
- The BST is not balanced (see above).
- Iterators are invalidated by any change to the container, and nothing
  detects that.
- I have not tested the out-of-memory paths by making `malloc` fail on
  purpose; they are there, but only reviewed by reading.

## What I would do next

- Make the BST a red-black tree (or start with an AVL tree, which I find
  easier to reason about) and rerun the sorted benchmark.
- Try Robin Hood hashing or backward-shift deletion, which avoid tombstones
  altogether, and compare probe lengths.
- A `malloc` wrapper for the tests that fails on the Nth call, to exercise
  the error paths.
- A proper benchmark with several runs and the median, instead of one run.

## Licence

MIT, see [LICENSE](LICENSE).
