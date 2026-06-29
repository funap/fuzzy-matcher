# FuzzyMatcher

A lightweight C++ fuzzy string matching library that provides fast and flexible fuzzy search functionality.

## Features

- Fuzzy string matching with scoring system
- Case-sensitive and case-insensitive matching support
- Special scoring for:
  - Consecutive character matches
  - First letter matches
  - Camel case boundaries
  - Extension starts
  - Directory separators
  - Word separators (space, underscore)
- Position tracking of matched characters (returns byte/character offsets depending on string type)
- Full Unicode support (UTF-8, UTF-16/Wide, and UTF-32)

## Installation

Just include the FuzzyMatcher.h and FuzzyMatcher.cpp files in your project.

## Usage

```cpp
#include "FuzzyMatcher.h"
#include <vector>

// 1. UTF-8 matching (standard string/string_view)
{
    FuzzyMatcher matcher("てすと");
    std::vector<size_t> positions;
    int score = matcher.ScoreMatch("あていすうと", &positions);
    // positions will contain the byte offsets of matched characters: {3, 9, 15}
}

// 2. Wide/UTF-16 matching (wstring/wstring_view)
{
    FuzzyMatcher matcher(L"ptr");
    std::vector<size_t> positions;
    int score = matcher.ScoreMatch(L"MyPointer", &positions);
    // positions will contain character offsets: {2, 5, 7}
}

// 3. UTF-32 matching (u32string/u32string_view)
{
    FuzzyMatcher matcher(U"abc");
    std::vector<size_t> positions;
    int score = matcher.ScoreMatch(U"xaxbxc", &positions);
    // positions will contain character offsets: {1, 3, 5}
}
```

## Scoring System

The matching algorithm takes into account the following matching patterns:

- Basic character matches
- Case-sensitive matches
- First letter matches
- Consecutive character matches
- File extension boundaries (after '.')
- Directory separator matches (after '\' or '/')
- Word separator matches (after space or underscore)
- Camel case word boundaries

A higher score indicates a better match quality.

## Requirements

- C++17 or later
- Compiler with C++17 filesystem/library support

## Test

To run the tests, follow these steps:

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Run CMake to build the project:
```bash
cmake ..
cmake --build . --config Debug
```

3. Run the tests:
```bash
# For single-configuration generators (e.g. Makefiles on Linux)
ctest

# For multi-configuration generators (e.g. Visual Studio on Windows)
ctest -C Debug
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
