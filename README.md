# FuzzyMatcher

[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[English](README.md) | [日本語](README.ja.md)

A lightweight C++17 fuzzy string matching library that uses dynamic programming to find globally optimal character subsequences, score relevance using structural bonuses, and track match positions for UI highlighting.

---

## Key Characteristics

- **Zero External Dependencies**: Implemented in standard C++17 (`FuzzyMatcher.h` and `FuzzyMatcher.cpp`).
- **Globally Optimal Alignment**: Uses a 2D dynamic programming matrix rather than a greedy search, ensuring that bonuses (boundaries, consecutive runs) are maximized even when characters repeat.
- **Context-Aware Scoring**: Favors acronyms, CamelCase boundaries, path separators, file extensions, and consecutive runs.
- **Native Multi-Encoding Support**: Accepts UTF-8 (`std::string_view`), UTF-16 / Wide string (`std::wstring_view`), and UTF-32 (`std::u32string_view`).
- **Direct Byte-Offset Tracking**: For UTF-8 input, the returned match positions correspond to original byte offsets, allowing direct string slicing or terminal color insertion without character-to-byte remapping.

---

## Quick Start

### Basic Matching & Position Retrieval

```cpp
#include <iostream>
#include <vector>
#include "FuzzyMatcher.h"

int main() {
    // 1. UTF-8 matching (returns byte offsets)
    {
        FuzzyMatcher matcher("fb");
        std::vector<size_t> positions;
        int score = matcher.ScoreMatch("FooBar", &positions);

        std::cout << "Score: " << score << "\n";
        std::cout << "Matched byte offsets: ";
        for (size_t pos : positions) {
            std::cout << pos << " ";
        }
        std::cout << "\n";
        // Output:
        // Score: 14
        // Matched byte offsets: 0 3
    }

    // 2. Wide / UTF-16 matching (returns character offsets)
    {
        FuzzyMatcher matcher(L"ptr");
        std::vector<size_t> positions;
        int score = matcher.ScoreMatch(L"MyPointer", &positions);
        // positions: { 2, 5, 7 }
    }

    // 3. UTF-32 matching (returns character offsets)
    {
        FuzzyMatcher matcher(U"abc");
        std::vector<size_t> positions;
        int score = matcher.ScoreMatch(U"xaxbxc", &positions);
        // positions: { 1, 3, 5 }
    }

    return 0;
}
```

### Visualizing Matches (CLI Highlighting Example)

Because `positions` contains exact byte offsets, highlighting matched characters is straightforward:

```cpp
#include <iostream>
#include <string>
#include <vector>
#include "FuzzyMatcher.h"

std::string HighlightMatch(std::string_view target, const std::vector<size_t>& positions) {
    std::string result;
    size_t posIdx = 0;

    for (size_t i = 0; i < target.size(); ++i) {
        if (posIdx < positions.size() && positions[posIdx] == i) {
            result += "[" + std::string(1, target[i]) + "]";
            ++posIdx;
        } else {
            result += target[i];
        }
    }
    return result;
}

int main() {
    FuzzyMatcher matcher("doc");
    std::string target = "my_document.txt";
    std::vector<size_t> positions;

    if (matcher.ScoreMatch(target, &positions) > 0) {
        std::cout << HighlightMatch(target, positions) << "\n";
        // Output: my_[d][o][c]ument.txt
    }
}
```

---

## How It Works

### 1. Why Dynamic Programming?

A naive greedy matcher matches characters at the earliest possible index. When matching pattern `"git"` against `"digital_git_repo"`:

```
Target:   d i g i t a l _ g i t _ r e p o
Greedy:       ^ ^ ^                     -> di[g][i][t]al_git_repo (Score: 21)
DP:                       ^ ^ ^         -> digital_[g][i][t]_repo (Score: 25)
```

- **Greedy approach**: Eagerly consumes `g`, `i`, `t` inside the prefix `digital` (indices 2, 3, 4). Because these characters sit in the middle of a word, they receive no word-boundary bonus.
- **DP approach**: Evaluates the global scoring matrix. It recognizes that matching the standalone word `_git` (indices 8, 9, 10) earns a word separator bonus (`+4`) followed by consecutive run multipliers, yielding 25 points instead of 21. DP guarantees the globally optimal alignment rather than being trapped in an accidental prefix match.

### 2. DP Recurrence Relation

Given pattern $P$ of length $M$ and target $T$ of length $N$:

Two flat matrices of size $M \times N$ are maintained:
- `scoreMatrix[i, j]`: Maximum cumulative score matching $P[0..i]$ within $T[0..j]$.
- `matchMatrix[i, j]`: Length of consecutive matches ending at $(i, j)$ (or `0` if skipped).

For each cell $(i, j)$:

$$\text{leftScore} = \text{scoreMatrix}[i, j-1]$$
$$\text{diagScore} = \text{scoreMatrix}[i-1, j-1]$$

If $P[i]$ and $T[j]$ match (case-insensitively):
$$\text{currentScore} = \text{diagScore} + \text{CalculateScore}(P[i], T, j, \text{consecutiveLength})$$

The algorithm transitions by picking the best path:
- If $\text{currentScore} \ge \text{leftScore}$, $(i, j)$ is marked as a match and records `consecutiveLength + 1`.
- Otherwise, $(i, j)$ inherits $\text{leftScore}$ and resets the consecutive counter to `0`.

### 3. Match Position Reconstruction (Backtracking)

To retrieve the exact indices of matched characters for UI highlighting, the matcher performs a single backward traversal from `(M-1, N-1)` through `matchMatrix`:

- **Skip (`0`)**: Character was skipped $\rightarrow$ move left (`targetIndex--`).
- **Match (`> 0`)**: Character was part of optimal alignment $\rightarrow$ record index and move diagonally up-left (`patternIndex--`, `targetIndex--`).

```text
Target:   l  a  t  e  s  t  _  t  e  s  t  s
Index:    0  1  2  3  4  5  6  7  8  9 10 11

P[0] = t  .  .  1  .  .  1  . (1) .  .  .  .
P[1] = e  .  .  .  2  .  .  .  . (2) .  .  .
P[2] = s  .  .  .  .  3  .  .  .  . (3) .  .
P[3] = t  .  .  .  .  .  4  .  .  .  . (4)←.  <-- Start at bottom-right
                               ↖  ↖  ↖  ↖
                      Recovered indices: [7, 8, 9, 10]
                      Result: latest_[t][e][s][t]s
```

Because traversal begins at the global maximum score cell, it naturally selects the higher-scoring boundary match (`_[test]`, 42 pts) over the sub-optimal prefix match (`la[test]`, 38 pts) without exploring dead ends. The collected indices are reversed at the end, yielding the final positions in $O(M + N)$ time with zero heap allocations.

---

## Scoring Breakdown

The scoring system assigns points based on where and how a character matches:

| Rule | Bonus Points | Condition / Rationale |
|:---|:---:|:---|
| **Base Match** | `+1` | Awarded for any case-insensitive character match. |
| **Same Case** | `+1` | Additional point when casing matches identically. |
| **First Letter** | `+8` | Match occurs at index 0 of the target string. |
| **Directory Separator** | `+5` | Preceded by `\` or `/` (standard path boundary). |
| **Word Separator** | `+4` | Preceded by a space (` `) or underscore (`_`). |
| **CamelCase Boundary** | `+4` | Uppercase character preceded by a lowercase character (`aB`). |
| **Extension Start** | `+3` | Preceded by a period (`.`). |
| **Consecutive Match** | `+(5 × k)` | Where $k$ is the current consecutive match run length. |

---

## Ranking in Practice (FuzzyFind Scenarios)

The primary role of a fuzzy matcher is ranking a list of candidates in an intuitive order. Here is how `FuzzyMatcher` ranks candidates across typical queries:

### Scenario 1: Acronym Matching (`pattern = "fmt"`)

When filtering file names or symbols with a short acronym:

| Rank | Score | Candidate | Match Detail |
|:---:|:---:|:---|:---|
| **1st** | **22** | `[f]ile_[m]anagement_[t]ool.cpp` | Hits every word start across snake_case (`f` at start, `m` and `t` after `_`). |
| **2nd** | **19** | `[F]ast[M]essage[T]hread.cpp` | Hits every word start across CamelCase boundaries (`F`, `M`, `T`). |
| **3rd** | **18** | `[f]or[m]at_[t]able.cpp` | Hits `f` and `_t`, but `m` is embedded inside `format` without boundary bonus. |

### Scenario 2: Consecutive Runs vs. Scattered Characters (`pattern = "str"`)

Consecutive matches receive quadratic-like score growth ($5 \times k$), heavily penalizing scattered noise:

| Rank | Score | Candidate | Match Detail |
|:---:|:---:|:---|:---|
| **1st** | **28** | `[S][t][r]ingStream.cpp` | Exact 3-character consecutive run at the start of the string. |
| **2nd** | **25** | `parse_[s][t][r]ucture.cpp` | Exact 3-character consecutive run right after a word boundary (`_`). |
| **3rd** | **23** | `[s]ystem_[t][r]ace.cpp` | Matches word boundaries, but `s` and `tr` are split across words. |

### Scenario 3: Eliminating Unrelated Noise (`pattern = "util"`)

Accidental substring matches are pushed to the bottom of the result list:

| Rank | Score | Candidate | Match Detail |
|:---:|:---:|:---|:---|
| **1st** | **46** | `[u][t][i][l]s/string_helper.cpp` | Starts with pattern + 4 consecutive characters. |
| **2nd** | **42** | `src/common_[u][t][i][l]s.cpp` | Word boundary + 4 consecutive characters. |
| **4th** | **17** | `m[u]l[t][i]thread_unit_[l]istener.cpp` | Characters accidentally scattered across 3 separate words. |

---

## Unicode & Multi-Byte Support

The library accepts multiple string representations:

```cpp
// UTF-8 (std::string_view)
FuzzyMatcher matcher8("テスト");
std::vector<size_t> utf8_positions;
matcher8.ScoreMatch("私のテストコード", &utf8_positions);
// utf8_positions receives byte offsets: { 6, 9, 12 }

// UTF-16 / Wide string (std::wstring_view)
FuzzyMatcher matcher16(L"abc");
matcher16.ScoreMatch(L"xaxbxc", &positions);

// UTF-32 (std::u32string_view)
FuzzyMatcher matcher32(U"abc");
matcher32.ScoreMatch(U"xaxbxc", &positions);
```

### Why Byte Offsets for UTF-8?
When working with UTF-8 strings in C++, multi-byte characters take between 1 and 4 bytes. Returning code-point indices forces callers to convert code-point positions back into byte offsets before slicing `std::string` or inserting ANSI escape sequences.

`FuzzyMatcher` converts UTF-8 to UTF-32 internally for correct character-level matching, but maps the indices back to original **byte offsets** before returning:

```
Target:   "あ て い す う と"
Bytes:    [0-2][3-5][6-8][9-11][12-14][15-17]
Pattern:  "てすと"
Result:   positions = { 3, 9, 15 }
```

---

## Complexity

| Metric | Complexity | Note |
|---|---|---|
| **Time Complexity** | $O(M \times N)$ | $M = \text{pattern length}$, $N = \text{target length}$. Early-exits if $M > N$ or either is empty. |
| **Space Complexity** | $O(M \times N)$ | Two flat `std::vector<int>` buffers resized to $M \times N$, reused across matching calls. |

---

## Integration

### Direct File Inclusion
Copy `src/FuzzyMatcher.h` and `src/FuzzyMatcher.cpp` directly into your project:

```
your_project/
  ├── include/
  │   └── FuzzyMatcher.h
  └── src/
      └── FuzzyMatcher.cpp
```

### CMake Integration

```cmake
add_subdirectory(path/to/fuzzy-matcher)
target_link_libraries(your_target PRIVATE FuzzyMatcher)
```

Or using `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    FuzzyMatcher
    GIT_REPOSITORY https://github.com/funap/fuzzy-matcher.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(FuzzyMatcher)

target_link_libraries(your_target PRIVATE FuzzyMatcher)
```

---

## Building & Tests

Requires CMake 3.14+ and a C++17-compliant compiler.

```bash
# Configure and build
cmake -B build -S .
cmake --build build --config Debug

# Run unit tests
# For single-configuration generators (e.g. Makefiles / Ninja):
ctest --test-dir build --output-on-failure

# For multi-configuration generators (e.g. Visual Studio on Windows):
ctest --test-dir build -C Debug --output-on-failure
```

---

## License

This project is licensed under the [MIT License](LICENSE).
