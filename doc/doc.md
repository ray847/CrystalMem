# DOC

## Functionality

This module provides macro memory management features that aims for highly
optimized memory storage for improved memory footprints, cache locality,
memory fragmentation and speed.

### Allocators

TODO: Implement these features:
All memory (de)allocators can be tuned with these characteristics:
- longevity
- size
- expansion(Mostly for containers)
- source (direct source / pool / resource)

Predefined allocation strategies are also defined:
- StaticStorage (sts):
No / extremely conservative management strategy.
- LongTermStorage (lts):
Stability-focused management stratety.
- TemporaryStorage (tps):
Aggressive management strategy.

Automatic allocators are also provided.
**Currently runtime allocation optimization is NOT supported.**

### Resources & Pools

