
# ECS 34 Project 3 — CSV Bus System & OpenStreetMap XML Parser

This project implements a set of classes that is crucial for navigation and transportation planning:

- CSV `CCSVBusSystem`
- OSM XML：`COpenStreetMap`

## Project Status

Passes all test with reasonable coverage. We assume input OSM files will be valid XML.

## Folder Structure

- `include/`: public headers
- `src/`: implementation
- `testsrc/`: GoogleTest unit tests
- `docs/`: API / usage docs

## Code References

- various cppreference algorithms page (`std::find_if`, `std::any_of`, etc.)
- [GoogleTest Fixtures](https://google.github.io/googletest/primer.html#:~:text=Test%20Fixtures%3A%20Using%20the%20Same%20Data%20Configuration%20for%20Multiple%20Tests)

## Generative AI Use

I used Gemini for all prompts

### Prompt

c++ how to convert string to number

### Response

1. Modern C++: std::stoi, std::stol, std::stod

The most common and readable method is the "String To [Type]" family of functions found in the `<string>` header. These handle various numeric types and throw an exception if the conversion fails.

Function Targeted Type Example
std::stoi() int int i = std::stoi("42");
std::stol() long long l = std::stol("123456789");
std::stoll() long long long long ll = std::stoll("999999999");
std::stod() double double d = std::stod("3.1415");
std::stof() float float f = std::stof("2.5f");
