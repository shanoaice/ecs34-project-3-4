# ECS 34 Project 4 — Dijkstra-based Transportation Planning

This project implements a set of classes that is crucial for navigation and transportation planning:

- Core Dijkstra Algorithm: `CDijkstraPathRouter`
- Transportation Planner powered by Dijkstra: `CDjikstraTransportationPlanner`
- Bus system indexer that helps to build graph and verify transportation plan: `CCSVBusSystemIndexer`
- Transportation Planner cmdline Interface: `CTransportationPlannerCommandLine`

## Project Status

All working, code quality might be spaghetti due to time difficulty. GetPathDescription does not work but that's bonus anyway.

Precompute method in DijkstraPathRouter takes a multithreaded brute-force approach where it spawns up to CPU logical thread count of threads, each working on one source vertex that performs a resumable Dijkstra Search. If they cannot finish before the deadline, subsequent FindShortestPath calls can still build on previous work to save computation. On CSIF Computer 25, it is way faster than the baseline implementation and achieves a similar performance in `speedtest` compared to professor's `speedtest_optimized` (orders of magnitude in 10^7). On another computer that has a Ryzen 7 9700X, it scales up better than professor's optimized version, and can be up to one order of magnitude faster than professors. Similarly, if you clamp down on how much cores/threads available, it scales down worse than professor's single-threaded optimized version.

`CMakeLists.txt` config exists only for profiling with CLion. Its profiler only supports CMake projects. `Makefile` is still the main compilation method and is fully functional.

## Folder Structure

- `include/`: public headers
- `src/`: implementation
- `testsrc/`: GoogleTest unit tests
- `docs/`: API / usage docs

## Code References

- various cppreference algorithms page (`std::find_if`, `std::any_of`, etc.)
- [GoogleTest Fixtures](https://google.github.io/googletest/primer.html#:~:text=Test%20Fixtures%3A%20Using%20the%20Same%20Data%20Configuration%20for%20Multiple%20Tests)
