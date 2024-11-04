# Beehive OS

A dive into the world of OS programming as a micro-kernel

## Components 

- Memory management via a cpu-bound Slub allocator
- A wacky thread scheduler (no NUMA or task grouping, yet)
- Can run go programs when extending the Go runtime to support the architecture

## Limitations

- A lot
- Only support Aarch64 currently
