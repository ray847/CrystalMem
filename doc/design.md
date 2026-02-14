# DESIGN

This document contains design choices and rules of this library.

## Implementation Architecture

### Resource

`Resource` classes represent a source where memory can be allocated from.
This group of classes should **NOT** worry about optimizations about memory allocation and usage.
An object of a resource class is only required to provide a way to obtain and release memory.
This can be shown in the concept definition: `AnyResource`.
This abstraction layer is made to avoid the problematic usage of new and delete calls everywhere in other parts of the code base.

### Pool

`Pool` classes stand as an intermediate step between obtaining memory and using memory so that "using" memory doesn't always comes with "requesting" memory.
This is also where most of the performance optimizations should be made.
Essentially, a `Pool` object establishes ownership of large chunks of memory and distributes them for regular usage.

It should be noted that the design of `Pool` classes should try their best to **NOT** work on the memory that it "owns".
This is because the memory provided by a resource can be purely **FICTIONAL** as in it is not mapped to any memory hardware.
In this case, the pool is "hijacked" to be use as a memory distributing strategy.
If the implementation will inevitably rely on accessing the owned memory, it should be well documented in its doc string and correctly set the constant `kInMemOptimization` to `true`.

### Allocator

`Allocator` objects can be created from `Pool` objects.
Allocators **MUST** be compliant with the stl.
Allocators are generally split into 3 catagories:
* DiscreteAllocator: This allocator can **ONLY** allocate singular objects as opposed to arrays of objects.
* ContinuousAllocator: This allocator is spetialized for allocating arrays of objects.
* GeneralAllocator: This allocator balances discrete allocation as well as continuous allocation.
