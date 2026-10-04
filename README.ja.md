# FuzzyMatcher

[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[English](README.md) | [日本語](README.ja.md)

動的計画法（DP）を用いて最適な部分列マッチングとコンテキストに応じたスコアリングを行い、マッチ位置のハイライト用オフセットを算出する軽量な C++17 ファジーストリングマッチングライブラリです。

---

## 主な特徴

- **外部依存ゼロ**: 標準 C++17 のみで実装（`FuzzyMatcher.h` と `FuzzyMatcher.cpp` の2ファイル構成）。
- **DP による大域的最適化**: 貪欲法（Greedy）ではなく 2次元 DP マトリクスを用いてマッチ経路を探索するため、同じ文字が重複して現れる場合でも境界や連続一致ボーナスが最大となる最適な組み合わせを見つけます。
- **文脈を考慮したスコアリング**: 単語先頭、キャメルケース境界、ディレクトリ区切り、拡張子、連続一致などを評価し、人間にとって自然な検索結果を上位にランク付けします。
- **マルチエンコーディング対応**: UTF-8（`std::string_view`）、UTF-16 / ワイド文字列（`std::wstring_view`）、UTF-32（`std::u32string_view`）をネイティブにサポート。
- **UTF-8 バイトオフセット返却**: UTF-8 入力時、マッチした文字位置はコードポイント番号ではなく**元のバイトオフセット**で返るため、追加のオフセット変換なしで ANSI カラー挿入やスライス処理が行えます。

---

## クイックスタート

### 基本的なマッチングとスコア取得

```cpp
#include <iostream>
#include <vector>
#include "FuzzyMatcher.h"

int main() {
    FuzzyMatcher matcher("fb");

    std::vector<size_t> positions;
    int score = matcher.ScoreMatch("FooBar", &positions);

    std::cout << "Score: " << score << "\n";
    std::cout << "Matched byte offsets: ";
    for (size_t pos : positions) {
        std::cout << pos << " ";
    }
    std::cout << "\n";
    // 出力:
    // Score: 14
    // Matched byte offsets: 0 3
    return 0;
}
```

### マッチ箇所のハイライト表示例

`positions` には正確なバイトオフセットが格納されるため、CLI や UI でのハイライト処理を簡潔に実装できます。

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
        // 出力: my_[d][o][c]ument.txt
    }
}
```

---

## アルゴリズムの仕組み

### 1. なぜ貪欲法（Greedy）ではなく動的計画法（DP）なのか

単純な貪欲法（先頭から見つかった文字を順に消費する方式）では、前方の部分一致に引っ張られて意図しないマッチをしてしまいます。例えばパターン `"git"` を `"digital_git_repo"` に対して検索した場合：

```
ターゲット: d i g i t a l _ g i t _ r e p o
貪欲法:         ^ ^ ^                     -> di[g][i][t]al_git_repo (スコア: 21点)
DP:                         ^ ^ ^         -> digital_[g][i][t]_repo (スコア: 25点)
```

- **貪欲法**: `"digital"` の中にある `g`, `i`, `t`（インデックス 2, 3, 4）を前方から拾ってしまいます。これらは単語の途中に埋もれているため、単語境界ボーナスが乗りません。
- **DP**: テーブル全体を探索し、単語区切り `_` の直後から始まる `"git"`（インデックス 8, 9, 10）にマッチさせた方が「単語区切りボーナス（`+4`）＋連続一致倍率」によりスコアが高くなる（25点 vs 21点）ことを認識します。局所的な先行一致に惑わされず、人間が意図した独立単語への大域的最適マッチを保証します。

### 2. 状態遷移とスコア計算

パターン $P$（長さ $M$）とターゲット $T$（長さ $N$）に対し、$M \times N$ の平坦化されたテーブルを用います：

- `scoreMatrix[i, j]`: $P[0..i]$ を $T[0..j]$ にマッチさせた場合の最大累積スコア
- `matchMatrix[i, j]`: $(i, j)$ で終了する連続一致文字数（マッチしなかった場合は `0`）

各セル $(i, j)$ の計算：

$$\text{leftScore} = \text{scoreMatrix}[i, j-1]$$
$$\text{diagScore} = \text{scoreMatrix}[i-1, j-1]$$

文字 $P[i]$ と $T[j]$ が（大文字小文字を無視して）一致する場合：
$$\text{currentScore} = \text{diagScore} + \text{CalculateScore}(P[i], T, j, \text{consecutiveLength})$$

遷移の判定：
- $\text{currentScore} \ge \text{leftScore}$ の場合：マッチを採用し、スコアを更新して連続一致数をインクリメント（`consecutiveLength + 1`）
- それ以外の場合：マッチを見送り（文字をスキップ）、$\text{leftScore}$ を引き継いで連続一致数を `0` にリセット

### 3. マッチ位置の復元（バックトラック）

UI 等でマッチした文字をハイライトするために、DP 計算完了後に `matchMatrix` を右下のセル `(M-1, N-1)` から逆方向に辿って最適なマッチ位置を復元します：

- **スキップ（値が `0`）**: 文字を不採用 $\rightarrow$ 左へ進む（`targetIndex--`）
- **マッチ採用（値が `> 0`）**: 最適なアライメントの一部 $\rightarrow$ インデックスを記録して左斜め上へ進む（`patternIndex--`, `targetIndex--`）

```text
ターゲット:  l  a  t  e  s  t  _  t  e  s  t  s
インデックス:0  1  2  3  4  5  6  7  8  9 10 11

 P[0] = t    .  .  1  .  .  1  . (1) .  .  .  .
 P[1] = e    .  .  .  2  .  .  .  . (2) .  .  .
 P[2] = s    .  .  .  .  3  .  .  .  . (3) .  .
 P[3] = t    .  .  .  .  .  4  .  .  .  . (4)←.  <-- 右下から探索開始
                                 ↖  ↖  ↖  ↖
                      復元インデックス: [7, 8, 9, 10]
                      出力結果: latest_[t][e][s][t]s
```

探索は大域的な最大スコアのセルから開始されるため、スコアの低い局所解（`la[test]` の 38点）に迷い込むことなく、単語境界ボーナスの乗った最適な `_[test]`（42点）だけを確実に回収します。収集したインデックス列を末尾で反転することで、$O(M + N)$ の計算量・追加のヒープ割り当てゼロで昇順の位置リストを復元します。

---

## スコアリング規則

マッチした文字の文脈や種類に応じて加点されます：

| 項目 | 加点 | 条件 / 理由 |
|:---|:---:|:---|
| **基本一致** | `+1` | 大文字小文字を問わず文字が一致した場合のベース点。 |
| **大文字小文字一致** | `+1` | 大文字小文字が完全に一致した場合の追加点。 |
| **先頭文字** | `+8` | ターゲット文字列の先頭（インデックス 0）に一致。 |
| **ディレクトリ区切り** | `+5` | 直前が `\` または `/`（パスの区切り直後）の場合。 |
| **単語区切り** | `+4` | 直前が空白（` `）またはアンダースコア（`_`）の場合。 |
| **キャメルケース境界** | `+4` | 直前が小文字で対象文字が大文字（例: `camelCase` の `C`）。 |
| **拡張子境界** | `+3` | 直前がドット（`.`）の場合。 |
| **連続一致** | `+(5 × k)` | 連続してマッチした文字数 $k$ に比例して線形加算。 |

---

## 実践的なランキング挙動（FuzzyFind シナリオ）

ファジーマッチングの本領は、候補リストの中で「どれが一番ユーザーの意図に近いか」を並び替えるランキングにあります。実際の検索でよくあるシチュエーションごとの挙動です：

### シナリオ 1: 頭文字（アクロニム）検索（`pattern = "fmt"`）

短縮入力でファイル名やシンボルを絞り込む場合：

| 順位 | スコア | 候補 | マッチ詳細 |
|:---:|:---:|:---|:---|
| **1位** | **22点** | `[f]ile_[m]anagement_[t]ool.cpp` | スネークケースの全単語の先頭文字に完全にヒット（先頭 `f` ＋ `_m`, `_t`）。 |
| **2位** | **19点** | `[F]ast[M]essage[T]hread.cpp` | キャメルケースの全単語の先頭大文字に完全にヒット（`F`, `M`, `T`）。 |
| **3位** | **18点** | `[f]or[m]at_[t]able.cpp` | `f` と `_t` は境界だが、`m` が `format` の途中に埋もれているため減点。 |

### シナリオ 2: 連続一致 vs 分散一致（`pattern = "str"`）

連続して一致するとボーナスが二次関数的に累積（$5 \times k$）するため、散らばったマッチよりも単語の塊が優先されます：

| 順位 | スコア | 候補 | マッチ詳細 |
|:---:|:---:|:---|:---|
| **1位** | **28点** | `[S][t][r]ingStream.cpp` | 文字列の先頭から 3文字完全に連続一致。 |
| **2位** | **25点** | `parse_[s][t][r]ucture.cpp` | 単語区切り（`_`）の直後から 3文字完全に連続一致。 |
| **3位** | **23点** | `[s]ystem_[t][r]ace.cpp` | 単語区切りには乗っているが、`s` と `tr` が別の単語に分断されているため下位に。 |

### シナリオ 3: 無関係なノイズの排除（`pattern = "util"`）

たまたま文字列中に該当文字が含まれているだけの候補は、自然に最下位へ押し出されます：

| 順位 | スコア | 候補 | マッチ詳細 |
|:---:|:---:|:---|:---|
| **1位** | **46点** | `[u][t][i][l]s\string_helper.cpp` | 先頭一致 ＋ 4文字連続。 |
| **2位** | **42点** | `src\common_[u][t][i][l]s.cpp` | 単語境界 ＋ 4文字連続。 |
| **4位** | **17点** | `m[u]l[t][i]thread_unit_[l]istener.cpp` | 3単語にまたがって偶発的に文字が分散しているため圧倒的低スコア。 |

---

## Unicode / マルチバイト対応

用途に応じた文字列型を渡すことができます：

```cpp
// UTF-8 (std::string_view)
FuzzyMatcher matcher8("テスト");
std::vector<size_t> utf8_positions;
matcher8.ScoreMatch("私のテストコード", &utf8_positions);
// utf8_positions にはバイトオフセット { 6, 9, 12 } が返る

// UTF-16 / ワイド文字列 (std::wstring_view)
FuzzyMatcher matcher16(L"abc");
matcher16.ScoreMatch(L"xaxbxc", &positions);

// UTF-32 (std::u32string_view)
FuzzyMatcher matcher32(U"abc");
matcher32.ScoreMatch(U"xaxbxc", &positions);
```

### UTF-8 におけるバイトオフセットの利点
日本語などのマルチバイト文字を含む UTF-8 文字列では、文字数（コードポイント数）とバイト数が一致しません。もし文字インデックスが返されると、呼び出し側で文字列をスライスしたりエスケープ文字を挿入する際に、都度コードポイントとバイト位置の再計算が必要になります。

`FuzzyMatcher` は内部で UTF-32 に変換してコードポイント単位で正確に判定を行いつつ、戻り値の位置情報としては**元の UTF-8 のバイトオフセット**へ再マッピングして返します：

```
ターゲット: "あ  て  い  す  う  と"
バイト範囲: [0-2][3-5][6-8][9-11][12-14][15-17]
パターン:   "てすと"
結果:       positions = { 3, 9, 15 }
```

---

## 計算量とメモリ効率

| 項目 | 計算量 | 補足 |
|---|---|---|
| **時間計算量** | $O(M \times N)$ | $M$: パターン長, $N$: ターゲット長。$M > N$ または空文字時は即座にリターン。 |
| **空間計算量** | $O(M \times N)$ | 平坦化された 1次元の `std::vector<int>` バッファを内部で保持・再利用。 |

---

## 導入方法

### ファイルの直接コピー
`src/FuzzyMatcher.h` と `src/FuzzyMatcher.cpp` をプロジェクトへ追加するだけで動作します。

```
your_project/
  ├── include/
  │   └── FuzzyMatcher.h
  └── src/
      └── FuzzyMatcher.cpp
```

### CMake による組み込み

```cmake
add_subdirectory(path/to/fuzzy-matcher)
target_link_libraries(your_target PRIVATE FuzzyMatcher)
```

または `FetchContent` を利用:

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

## ビルドとテストの実行

CMake 3.14 以上および C++17 対応コンパイラが必要です。

```bash
# ビルドディレクトリの作成とビルド
cmake -B build -S .
cmake --build build --config Debug

# テスト実行
# シングルコンフィグ（Makefiles / Ninja など）の場合:
ctest --test-dir build --output-on-failure

# マルチコンフィグ（Windows / Visual Studio など）の場合:
ctest --test-dir build -C Debug --output-on-failure
```

---

## ライセンス

本プロジェクトは [MIT License](LICENSE) のもとで公開されています。
