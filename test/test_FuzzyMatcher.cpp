#include <gtest/gtest.h>
#include "FuzzyMatcher.h"

class FuzzyMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FuzzyMatcherTest, BasicMatching) {
    FuzzyMatcher matcher(L"abc");
    std::vector<size_t> positions;

    // Exact match
    EXPECT_GT(matcher.ScoreMatch(L"abc", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));

    // Partial match
    EXPECT_GT(matcher.ScoreMatch(L"xaxbxc", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({1, 3, 5}));
}

TEST_F(FuzzyMatcherTest, CaseInsensitiveMatching) {
    FuzzyMatcher matcher(L"abc");
    std::vector<size_t> positions;

    EXPECT_GT(matcher.ScoreMatch(L"ABC", &positions), 0);
    EXPECT_GT(matcher.ScoreMatch(L"aBc", &positions), 0);
}

TEST_F(FuzzyMatcherTest, BonusScoring) {
    // First letter bonus
    {
        FuzzyMatcher matcher(L"a");
        std::vector<size_t> positions;
        int scoreStart = matcher.ScoreMatch(L"abc", &positions);
        int scoreMid = matcher.ScoreMatch(L"bac", &positions);
        EXPECT_GT(scoreStart, scoreMid);
    }

    // Camel case bonus
    {
        FuzzyMatcher matcher(L"c");
        std::vector<size_t> positions;
        int scoreCamel = matcher.ScoreMatch(L"testCase", &positions);
        int scoreNormal = matcher.ScoreMatch(L"testcase", &positions);
        EXPECT_GT(scoreCamel, scoreNormal);
    }
}

TEST_F(FuzzyMatcherTest, EdgeCases) {
    // Empty pattern
    {
        FuzzyMatcher matcher(L"");
        std::vector<size_t> positions;
        EXPECT_EQ(matcher.ScoreMatch(L"test", &positions), 0);
    }

    // Empty target
    {
        FuzzyMatcher matcher(L"test");
        std::vector<size_t> positions;
        EXPECT_EQ(matcher.ScoreMatch(L"", &positions), 0);
    }

    // Pattern longer than target
    {
        FuzzyMatcher matcher(L"toolong");
        std::vector<size_t> positions;
        EXPECT_EQ(matcher.ScoreMatch(L"short", &positions), 0);
    }
}

TEST_F(FuzzyMatcherTest, SeparatorBonus) {
    // Directory separator bonus (backslash)
    {
        FuzzyMatcher matcher(L"f");
        std::vector<size_t> positions;
        int scoreSep = matcher.ScoreMatch(L"test\\file", &positions);
        int scoreNormal = matcher.ScoreMatch(L"testfile", &positions);
        EXPECT_GT(scoreSep, scoreNormal);
    }

    // Directory separator bonus (slash)
    {
        FuzzyMatcher matcher(L"f");
        std::vector<size_t> positions;
        int scoreSep = matcher.ScoreMatch(L"test/file", &positions);
        int scoreNormal = matcher.ScoreMatch(L"testfile", &positions);
        EXPECT_GT(scoreSep, scoreNormal);
    }

    // Underscore separator bonus
    {
        FuzzyMatcher matcher(L"t");
        std::vector<size_t> positions;
        int scoreSep = matcher.ScoreMatch(L"some_test", &positions);
        int scoreNormal = matcher.ScoreMatch(L"sometest", &positions);
        EXPECT_GT(scoreSep, scoreNormal);
    }
}

TEST_F(FuzzyMatcherTest, NullPositionsSupport) {
    FuzzyMatcher matcher("abc");
    // Verifying it doesn't crash and returns the correct score
    EXPECT_GT(matcher.ScoreMatch("abc", nullptr), 0);
    EXPECT_EQ(matcher.ScoreMatch("xyz", nullptr), 0);
}

TEST_F(FuzzyMatcherTest, InvalidUtf8Handling) {
    FuzzyMatcher matcher("abc");
    std::vector<size_t> positions = {1, 2, 3};
    
    // Invalid UTF-8 sequence (0xFF is not a valid lead byte)
    int score = matcher.ScoreMatch("\xFF\xFE\xFD", &positions);
    EXPECT_EQ(score, 0);
    EXPECT_TRUE(positions.empty());
}


TEST_F(FuzzyMatcherTest, ConsecutiveMatches) {
    FuzzyMatcher matcher(L"abc");
    std::vector<size_t> positions;

    int scoreConsecutive = matcher.ScoreMatch(L"abc", &positions);
    int scoreNonConsecutive = matcher.ScoreMatch(L"axbxc", &positions);
    EXPECT_GT(scoreConsecutive, scoreNonConsecutive);
}

TEST_F(FuzzyMatcherTest, UTF8BasicMatching) {
    // UTF-8 constructor and UTF-8 ScoreMatch
    FuzzyMatcher matcher("abc");
    std::vector<size_t> positions;

    // Exact match
    EXPECT_GT(matcher.ScoreMatch("abc", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));

    // Partial match
    positions.clear();
    EXPECT_GT(matcher.ScoreMatch("xaxbxc", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({1, 3, 5}));

    // Case insensitive UTF-8 match
    positions.clear();
    EXPECT_GT(matcher.ScoreMatch("ABC", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));
}

TEST_F(FuzzyMatcherTest, UTF8JapaneseMatching) {
    // Matching Japanese characters
    FuzzyMatcher matcher("てすと");
    std::vector<size_t> positions;

    // "てすと" is:
    // て (3 bytes: 0xE3 0x81 0xA6)
    // す (3 bytes: 0xE3 0x81 0x99)
    // と (3 bytes: 0xE3 0x81 0xA8)
    //
    // Target: "あていすうと"
    // あ (3 bytes: 0xE3 0x81 0x82) [0-2]
    // て (3 bytes: 0xE3 0x81 0xA6) [3-5]
    // い (3 bytes: 0xE3 0x81 0x84) [6-8]
    // す (3 bytes: 0xE3 0x81 0x99) [9-11]
    // う (3 bytes: 0xE3 0x81 0x86) [12-14]
    // と (3 bytes: 0xE3 0x81 0xA8) [15-17]
    EXPECT_GT(matcher.ScoreMatch("あていすうと", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({3, 9, 15}));
}

TEST_F(FuzzyMatcherTest, UTF32BasicMatching) {
    // Using char32_t / std::u32string_view
    FuzzyMatcher matcher(U"abc");
    std::vector<size_t> positions;

    EXPECT_GT(matcher.ScoreMatch(U"xaxbxc", &positions), 0);
    EXPECT_EQ(positions, std::vector<size_t>({1, 3, 5}));
}

TEST_F(FuzzyMatcherTest, PositionsVectorHandling) {
    // UTF-8
    {
        FuzzyMatcher matcher("abc");
        std::vector<size_t> positions = {99, 100};
        
        // Should clear on failure
        EXPECT_EQ(matcher.ScoreMatch("xyz", &positions), 0);
        EXPECT_TRUE(positions.empty());

        // Should clear on success (pre-existing elements removed)
        positions = {99, 100};
        EXPECT_GT(matcher.ScoreMatch("abc", &positions), 0);
        EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));
    }

    // UTF-16
    {
        FuzzyMatcher matcher(L"abc");
        std::vector<size_t> positions = {99, 100};

        // Should clear on failure
        EXPECT_EQ(matcher.ScoreMatch(L"xyz", &positions), 0);
        EXPECT_TRUE(positions.empty());

        // Should clear on success
        positions = {99, 100};
        EXPECT_GT(matcher.ScoreMatch(L"abc", &positions), 0);
        EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));
    }

    // UTF-32
    {
        FuzzyMatcher matcher(U"abc");
        std::vector<size_t> positions = {99, 100};

        // Should clear on failure
        EXPECT_EQ(matcher.ScoreMatch(U"xyz", &positions), 0);
        EXPECT_TRUE(positions.empty());

        // Should clear on success
        positions = {99, 100};
        EXPECT_GT(matcher.ScoreMatch(U"abc", &positions), 0);
        EXPECT_EQ(positions, std::vector<size_t>({0, 1, 2}));
    }
}

