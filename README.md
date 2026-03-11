
# ECS 34 Project 3 — CSV Bus System & OpenStreetMap XML Parser

## Author

Haoqing Chen (shanoaice) 923493503  
Iris Zhong (zzzsqiris) 923292733  
*Hongrui Zhao (zeturn) 923210265 (Project 2 Code)*

This project implements a set of CSV & OpenStreetMap XML Parser:

- CSV `CCSVBusSystem`
- OSM XML：`COpenStreetMap`

## Project Status

Preliminary implementations of:

- `CDijkstraPathRouter`
- `CDijkstraTransportationPlanner`
- `CCSVBusSystemIndexer`
- `TransportationPlannerCommandLine`

are provided. Simple functions (accessors, etc.) are mostly complete. Complex / Core functions remains to be implemented.

`CMakeLists.txt` config exists only for profiling with CLion. Its profiler only supports CMake projects. `Makefile` is still the main compilation method and is fully functional.

## Folder Structure

- `include/`: public headers
- `src/`: implementation
- `testsrc/`: GoogleTest unit tests
- `docs/`: API / usage docs

## Code References

- various cppreference algorithms page (`std::find_if`, `std::any_of`, etc.)
- [GoogleTest Fixtures](https://google.github.io/googletest/primer.html#:~:text=Test%20Fixtures%3A%20Using%20the%20Same%20Data%20Configuration%20for%20Multiple%20Tests)

## Generative AI Use

### Haoqing Chen

Some prompts and dialogues are overly long, and I am not going to paste full content.

- https://www.perplexity.ai/search/i-am-asked-to-implement-dijkst-9WKgKp5RQjqVzyiHNZjltw
