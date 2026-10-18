/*
 * Project Ambrose by Imjustchico
 * Verifies deck edits are proposed without mutation and yield exact revisioned per-item rows only for accepted changes.
 */

#include "DeckEditPlanner.h"

#include <gtest/gtest.h>

#include <limits>

namespace
{
    DeckLimits const FireDeckLimits{ 64, 4, 6 };

    DeckEditPlan Add(PlayerDeck const& deck, uint64 itemGuid, uint32 spellId, uint64 priorRevision = 0)
    {
        return DeckEditPlanner::Prepare(DeckEditAction::Add, deck, itemGuid, spellId, DeckCopyCategory::Generic, FireDeckLimits, 2, priorRevision);
    }
}

TEST(DeckEditPlannerTest, AcceptedAddPreservesCurrentDeckUntilStorageFinishes)
{
    PlayerDeck current;
    ASSERT_EQ(current.Add(101, DeckCopyCategory::Generic, FireDeckLimits), DeckEditResult::Added);
    DeckEditPlan const plan = Add(current, 7001, 101, 11);
    ASSERT_EQ(plan.Result, DeckEditResult::Added);
    ASSERT_TRUE(plan.Candidate);
    EXPECT_EQ(current.Count(101), 1u);
    EXPECT_EQ(plan.Candidate->Cards.Count(101), 2u);
    EXPECT_EQ(plan.Candidate->Save, (CharacterDeckSpell{ 7001, 101, 2, 2, 12 }));
}

TEST(DeckEditPlannerTest, CopyLimitAndMissingSpellDoNotProduceWrites)
{
    PlayerDeck current;
    for (int i = 0; i < 4; ++i)
        ASSERT_EQ(current.Add(101, DeckCopyCategory::Generic, FireDeckLimits), DeckEditResult::Added);

    DeckEditPlan const refused = Add(current, 7001, 101, 5);
    EXPECT_EQ(refused.Result, DeckEditResult::CopyLimit);
    EXPECT_FALSE(refused.Candidate);
    EXPECT_EQ(current.Count(101), 4u);

    DeckEditPlan const absent = DeckEditPlanner::Prepare(DeckEditAction::Remove, current, 7001, 202, DeckCopyCategory::Generic, FireDeckLimits, 3, 1);
    EXPECT_EQ(absent.Result, DeckEditResult::NotFound);
    EXPECT_FALSE(absent.Candidate);
}

TEST(DeckEditPlannerTest, RemovingFinalCopyCreatesVersionedZeroQuantityTombstone)
{
    PlayerDeck current;
    ASSERT_EQ(current.Add(103, DeckCopyCategory::School, FireDeckLimits), DeckEditResult::Added);
    DeckEditPlan const plan = DeckEditPlanner::Prepare(DeckEditAction::Remove, current, 7002, 103, DeckCopyCategory::School, FireDeckLimits, 5, 3);
    ASSERT_EQ(plan.Result, DeckEditResult::Removed);
    ASSERT_TRUE(plan.Candidate);
    EXPECT_EQ(current.Count(103), 1u);
    EXPECT_EQ(plan.Candidate->Cards.Count(103), 0u);
    EXPECT_EQ(plan.Candidate->Save, (CharacterDeckSpell{ 7002, 103, 0, 5, 4 }));
}

TEST(DeckEditPlannerTest, DeckItemIdAndHighBitSpellIdentifierRemainUnchanged)
{
    PlayerDeck current;
    uint32 constexpr spellId = 0xF08E1234u;
    DeckEditPlan const first = Add(current, 9001, spellId);
    DeckEditPlan const second = Add(current, 9002, spellId);
    ASSERT_TRUE(first.Candidate);
    ASSERT_TRUE(second.Candidate);
    EXPECT_EQ(first.Candidate->Save.SpellId, spellId);
    EXPECT_EQ(second.Candidate->Save.SpellId, spellId);
    EXPECT_EQ(first.Candidate->Save.DeckItemGuid, 9001u);
    EXPECT_EQ(second.Candidate->Save.DeckItemGuid, 9002u);
    EXPECT_EQ(current.TotalCards(), 0u);
}

TEST(DeckEditPlannerTest, BadIdentifiersAndRevisionOverflowFailClosed)
{
    PlayerDeck current;
    EXPECT_FALSE(Add(current, 0, 101).Candidate);
    EXPECT_FALSE(Add(current, 7001, 0).Candidate);
    EXPECT_FALSE(Add(current, 7001, 101, std::numeric_limits<uint64>::max()).Candidate);
    EXPECT_FALSE(DeckEditPlanner::Prepare(static_cast<DeckEditAction>(255), current, 7001, 101, DeckCopyCategory::Generic, FireDeckLimits, 0, 0).Candidate);
    EXPECT_EQ(current.TotalCards(), 0u);
}
