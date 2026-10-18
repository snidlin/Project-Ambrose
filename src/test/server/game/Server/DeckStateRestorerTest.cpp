/*
 * Project Ambrose by Imjustchico
 * Tests exact per-item deck reconstruction, tombstones, order, high-bit spell IDs, and fail-closed handling of corrupt saved quantities.
 */

#include "DeckStateRestorer.h"

#include <gtest/gtest.h>

#include <vector>

TEST(DeckStateRestorerTest, RestoresOnlyChosenDeckAndKeepsItsOrderAndTombstones)
{
    std::vector<CharacterDeckSpell> const rows{
        { 7002, 101, 4, 0, 1 },
        { 7001, 202, 0, 4, 3 },
        { 7001, 303, 2, 2, 5 },
        { 7001, 101, 1, 1, 7 },
        { 7002, 303, 6, 1, 2 }
    };
    std::optional<RestoredDeckState> const restored = DeckStateRestorer::Restore(7001, 64, rows);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->ItemGuid, 7001u);
    EXPECT_EQ(restored->Cards.TotalCards(), 3u);
    EXPECT_EQ(restored->Cards.GetSpells(), (std::vector<DeckSpell>{ { 101, 1 }, { 303, 2 } }));
    EXPECT_EQ(restored->Cards.Count(202), 0u);
    EXPECT_EQ(restored->Rows.size(), 3u);
    EXPECT_EQ(restored->Rows[0].SpellId, 101u);
    EXPECT_EQ(restored->Rows[1].SpellId, 303u);
    EXPECT_EQ(restored->Rows[2].SpellId, 202u);
    EXPECT_EQ(restored->RevisionOf(202), 3u);
    EXPECT_EQ(restored->RevisionOf(101), 7u);
    EXPECT_EQ(restored->RevisionOf(999), 0u);
}

TEST(DeckStateRestorerTest, SameTemplateDeckInstancesNeverShareSavedCards)
{
    uint32 constexpr highId = 0xF08E1234u;
    std::vector<CharacterDeckSpell> const rows{ { 7001, highId, 3, 0, 1 }, { 7002, highId, 1, 0, 8 } };
    auto const first = DeckStateRestorer::Restore(7001, 64, rows);
    auto const second = DeckStateRestorer::Restore(7002, 64, rows);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->Cards.Count(highId), 3u);
    EXPECT_EQ(second->Cards.Count(highId), 1u);
    EXPECT_EQ(first->RevisionOf(highId), 1u);
    EXPECT_EQ(second->RevisionOf(highId), 8u);
}

TEST(DeckStateRestorerTest, InvalidOrRepeatedRowsAreRefusedEvenForTombstones)
{
    EXPECT_FALSE(DeckStateRestorer::Restore(0, 64, {}));
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 0, {}));
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 64, { { 7001, 0, 1, 0, 1 } }));
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 64, { { 7001, 101, 1, 0, 0 } }));
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 64, { { 7001, 101, 0, 0, 1 }, { 7001, 101, 1, 0, 2 } }));
}

TEST(DeckStateRestorerTest, TooManyStoredCardsAndNonpositiveEntriesFailClosed)
{
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 4, { { 7001, 101, 5, 0, 1 } }));
    EXPECT_FALSE(DeckStateRestorer::Restore(7001, 4, { { 7001, 101, 3, 0, 1 }, { 7001, 202, 2, 1, 1 } }));
    auto const tombstoneOnly = DeckStateRestorer::Restore(7001, 4, { { 7001, 101, 0, 0, 3 } });
    ASSERT_TRUE(tombstoneOnly);
    EXPECT_TRUE(tombstoneOnly->Cards.GetSpells().empty());
    EXPECT_EQ(tombstoneOnly->RevisionOf(101), 3u);
}

TEST(DeckStateRestorerTest, MissingStoredRowsAreAnEmptyDeckWithoutAssumingAnySchoolCategory)
{
    std::vector<CharacterDeckSpell> const rows{ { 7002, 101, 2, 0, 2 } };
    auto empty = DeckStateRestorer::Restore(7001, 14, rows);
    ASSERT_TRUE(empty);
    EXPECT_EQ(empty->ItemGuid, 7001u);
    EXPECT_TRUE(empty->Cards.GetSpells().empty());
    EXPECT_TRUE(empty->Rows.empty());
    EXPECT_EQ(empty->Cards.Add(101, DeckCopyCategory::Generic, { 14, 3, 3 }), DeckEditResult::Added);
}
