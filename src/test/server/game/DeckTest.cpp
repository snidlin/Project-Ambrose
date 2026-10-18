/*
 * Project Ambrose by Imjustchico
 * Tests that the deck's copy and size limits and absent-spell refusal leave all spell quantities unchanged on unsuccessful edits.
 */

#include "PlayerDeck.h"

#include <gtest/gtest.h>

TEST(DeckTest, RefusesTheFifthCopyWhenTheDeckAllowsFour)
{
    PlayerDeck deck;
    DeckLimits const limits{ 64, 4 };
    for (int i = 0; i < 4; ++i)
        ASSERT_EQ(deck.Add(101, limits), DeckEditResult::Added);
    std::vector<DeckSpell> const before = deck.GetSpells();

    EXPECT_EQ(deck.Add(101, limits), DeckEditResult::CopyLimit);
    EXPECT_EQ(deck.GetSpells(), before);
    EXPECT_EQ(deck.Count(101), 4u);
    EXPECT_EQ(deck.TotalCards(), 4u);
}

TEST(DeckTest, RemovingAnAbsentSpellChangesNothing)
{
    PlayerDeck deck;
    ASSERT_EQ(deck.Add(101, { 64, 4 }), DeckEditResult::Added);
    std::vector<DeckSpell> const before = deck.GetSpells();

    EXPECT_EQ(deck.Remove(202), DeckEditResult::NotFound);
    EXPECT_EQ(deck.GetSpells(), before);
    EXPECT_EQ(deck.TotalCards(), 1u);
}

TEST(DeckTest, DeckCapacityCannotBeBypassedWithDifferentSpells)
{
    PlayerDeck deck;
    DeckLimits const limits{ 2, 4 };
    ASSERT_EQ(deck.Add(101, limits), DeckEditResult::Added);
    ASSERT_EQ(deck.Add(202, limits), DeckEditResult::Added);

    EXPECT_EQ(deck.Add(303, limits), DeckEditResult::DeckFull);
    EXPECT_EQ(deck.TotalCards(), 2u);
    EXPECT_EQ(deck.Count(303), 0u);
}

TEST(DeckTest, RemovingLastCopyErasesTheEntryAndAllowsReadmission)
{
    PlayerDeck deck;
    DeckLimits const limits{ 1, 1 };
    ASSERT_EQ(deck.Add(101, limits), DeckEditResult::Added);
    ASSERT_EQ(deck.Remove(101), DeckEditResult::Removed);

    EXPECT_EQ(deck.TotalCards(), 0u);
    EXPECT_TRUE(deck.GetSpells().empty());
    EXPECT_EQ(deck.Add(101, limits), DeckEditResult::Added);
}

TEST(DeckTest, InvalidLimitsAndSpellIdsFailClosed)
{
    PlayerDeck deck;
    EXPECT_EQ(deck.Add(0, { 5, 2 }), DeckEditResult::InvalidSpell);
    EXPECT_EQ(deck.Add(101, { 0, 2 }), DeckEditResult::InvalidLimits);
    EXPECT_EQ(deck.Add(101, { 5, 0 }), DeckEditResult::InvalidLimits);
    EXPECT_TRUE(deck.GetSpells().empty());
}
