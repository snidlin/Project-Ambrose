/*
 * Project Ambrose by Imjustchico
 * Checks ordinary deck counts, school copy limits, and refusal without mutation.
 */

#include "PlayerDeck.h"

#include <gtest/gtest.h>

TEST(DeckTest, RefusesTheFifthGenericCopyWhenTheDeckAllowsFour)
{
    PlayerDeck deck;
    DeckLimits const limits{ 64, 4, 6 };
    for (int i = 0; i < 4; ++i)
        ASSERT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::Added);
    std::vector<DeckSpell> const before = deck.GetSpells();

    EXPECT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::CopyLimit);
    EXPECT_EQ(deck.GetSpells(), before);
    EXPECT_EQ(deck.Count(101), 4u);
    EXPECT_EQ(deck.TotalCards(), 4u);
}

TEST(DeckTest, AllowsSixSchoolCopiesButRefusesTheSeventh)
{
    PlayerDeck deck;
    DeckLimits const limits{ 64, 4, 6 };
    for (int i = 0; i < 6; ++i)
        ASSERT_EQ(deck.Add(101, DeckCopyCategory::School, limits), DeckEditResult::Added);
    std::vector<DeckSpell> const before = deck.GetSpells();

    EXPECT_EQ(deck.Add(101, DeckCopyCategory::School, limits), DeckEditResult::CopyLimit);
    EXPECT_EQ(deck.GetSpells(), before);
    EXPECT_EQ(deck.TotalCards(), 6u);
}

TEST(DeckTest, GenericAndSchoolCopiesRespectTheirOwnLimits)
{
    PlayerDeck deck;
    DeckLimits const limits{ 64, 4, 6 };
    for (int i = 0; i < 4; ++i)
        ASSERT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::Added);
    for (int i = 0; i < 6; ++i)
        ASSERT_EQ(deck.Add(202, DeckCopyCategory::School, limits), DeckEditResult::Added);

    EXPECT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::CopyLimit);
    EXPECT_EQ(deck.Add(202, DeckCopyCategory::School, limits), DeckEditResult::CopyLimit);
    EXPECT_EQ(deck.TotalCards(), 10u);
}

TEST(DeckTest, RemovingAnAbsentSpellChangesNothing)
{
    PlayerDeck deck;
    ASSERT_EQ(deck.Add(101, DeckCopyCategory::Generic, { 64, 4, 6 }), DeckEditResult::Added);
    std::vector<DeckSpell> const before = deck.GetSpells();

    EXPECT_EQ(deck.Remove(202), DeckEditResult::NotFound);
    EXPECT_EQ(deck.GetSpells(), before);
    EXPECT_EQ(deck.TotalCards(), 1u);
}

TEST(DeckTest, DeckCapacityCannotBeBypassedWithDifferentSpells)
{
    PlayerDeck deck;
    DeckLimits const limits{ 2, 4, 6 };
    ASSERT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::Added);
    ASSERT_EQ(deck.Add(202, DeckCopyCategory::School, limits), DeckEditResult::Added);

    EXPECT_EQ(deck.Add(303, DeckCopyCategory::School, limits), DeckEditResult::DeckFull);
    EXPECT_EQ(deck.TotalCards(), 2u);
    EXPECT_EQ(deck.Count(303), 0u);
}

TEST(DeckTest, TutorialDeckAllowsOnlyFourteenCardsAndThreeCopies)
{
    PlayerDeck deck;
    DeckLimits const limits{ 14, 3, 3 };
    for (uint32 spellId = 1; spellId <= 14; ++spellId)
        ASSERT_EQ(deck.Add(spellId, DeckCopyCategory::Generic, limits), DeckEditResult::Added);

    EXPECT_EQ(deck.Add(15, DeckCopyCategory::Generic, limits), DeckEditResult::DeckFull);
    EXPECT_EQ(deck.TotalCards(), 14u);

    PlayerDeck copyCheck;
    for (int i = 0; i < 3; ++i)
        ASSERT_EQ(copyCheck.Add(101, DeckCopyCategory::School, limits), DeckEditResult::Added);
    EXPECT_EQ(copyCheck.Add(101, DeckCopyCategory::School, limits), DeckEditResult::CopyLimit);
}

TEST(DeckTest, RemovingLastCopyErasesTheEntryAndAllowsReadmission)
{
    PlayerDeck deck;
    DeckLimits const limits{ 1, 1, 1 };
    ASSERT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::Added);
    ASSERT_EQ(deck.Remove(101), DeckEditResult::Removed);

    EXPECT_EQ(deck.TotalCards(), 0u);
    EXPECT_TRUE(deck.GetSpells().empty());
    EXPECT_EQ(deck.Add(101, DeckCopyCategory::Generic, limits), DeckEditResult::Added);
}

TEST(DeckTest, InvalidLimitsAndSpellIdsFailClosed)
{
    PlayerDeck deck;
    EXPECT_EQ(deck.Add(0, DeckCopyCategory::Generic, { 5, 2, 2 }), DeckEditResult::InvalidSpell);
    EXPECT_EQ(deck.Add(101, DeckCopyCategory::Generic, { 0, 2, 2 }), DeckEditResult::InvalidLimits);
    EXPECT_EQ(deck.Add(101, DeckCopyCategory::Generic, { 5, 0, 2 }), DeckEditResult::InvalidLimits);
    EXPECT_EQ(deck.Add(101, DeckCopyCategory::School, { 5, 2, 0 }), DeckEditResult::InvalidLimits);
    EXPECT_EQ(deck.Add(101, static_cast<DeckCopyCategory>(255), { 5, 2, 2 }), DeckEditResult::InvalidLimits);
    EXPECT_TRUE(deck.GetSpells().empty());
}
