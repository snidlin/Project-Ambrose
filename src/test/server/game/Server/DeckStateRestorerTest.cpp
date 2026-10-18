/*
 * Project Ambrose by Imjustchico
 * Tests exact per-item deck reconstruction, tombstones, order, high-bit spell IDs, and fail-closed handling of corrupt saved quantities.
 */

#include "DeckStateRestorer.h"
#include "ItemTemplateStore.h"
#include "PlayerEquipment.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace
{
    ItemTemplateRecord DeckTemplate(uint32 id, int32 maxCards)
    {
        ItemTemplateRecord item;
        item.TemplateId = id;
        item.Adjectives = { "Deck" };
        item.Deck.emplace();
        item.Deck->MaxSpells = maxCards;
        return item;
    }

    PlayerEquipment WornDecks(std::vector<std::pair<uint64, uint32>> const& ids)
    {
        std::vector<CharacterEquippedItem> worn;
        for (auto const& [guid, templateId] : ids)
        {
            CharacterItem item;
            item.Guid = guid;
            item.TemplateId = templateId;
            worn.push_back({ item, "Deck" });
        }
        return PlayerEquipment::FromStored(std::move(worn));
    }
}

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

TEST(DeckStateRestorerTest, EquippedSelectionUsesTheWornItemGuidAndItsOwnCapacity)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 14), DeckTemplate(102, 64) });
    ASSERT_TRUE(templates);
    auto const selected = DeckStateRestorer::RestoreEquipped(WornDecks({ { 7002, 102 } }), *templates,
        { { 7001, 301, 5, 0, 1 }, { 7002, 301, 2, 0, 3 } });
    ASSERT_TRUE(selected.Valid);
    ASSERT_TRUE(selected.Deck);
    EXPECT_EQ(selected.Deck->ItemGuid, 7002u);
    EXPECT_EQ(selected.Deck->Cards.TotalCards(), 2u);
    EXPECT_EQ(selected.Deck->RevisionOf(301), 3u);
}

TEST(DeckStateRestorerTest, NoWornDeckIsValidAndDoesNotExposeBackpackRows)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 14) });
    ASSERT_TRUE(templates);
    auto const selected = DeckStateRestorer::RestoreEquipped(PlayerEquipment{}, *templates, { { 7001, 301, 1, 0, 1 } });
    EXPECT_TRUE(selected.Valid);
    EXPECT_FALSE(selected.Deck);
}

TEST(DeckStateRestorerTest, MissingTemplateAndTwoWornDecksAreRefused)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 14), DeckTemplate(102, 64) });
    ASSERT_TRUE(templates);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(WornDecks({ { 7001, 999 } }), *templates, {}).Valid);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(WornDecks({ { 7001, 101 }, { 7002, 102 } }), *templates, {}).Valid);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(WornDecks({ { 0, 101 } }), *templates, {}).Valid);
}

TEST(DeckStateRestorerTest, MalformedSelectedDeckAndInvalidTemplateCapacityAreRefused)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 14), DeckTemplate(102, 0) });
    ASSERT_TRUE(templates);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(WornDecks({ { 7001, 101 } }), *templates, { { 7001, 301, 15, 0, 1 } }).Valid);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(WornDecks({ { 7001, 102 } }), *templates, {}).Valid);
}

TEST(DeckStateRestorerTest, SwappingTwoIdenticalTemplatesUsesEachPhysicalDeckAndReequippingRestoresOriginalCards)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 14) });
    ASSERT_TRUE(templates);
    EquipSlot slot;
    slot.Name = "Deck";
    slot.AdjectivesAnd = { "Deck" };
    EquipmentSlots const slots = EquipmentSlots::FromSlots({ slot });
    EquipWizard const wizard("Fire", 20);
    CharacterItem first;
    first.Guid = 7001;
    first.TemplateId = 101;
    CharacterItem second;
    second.Guid = 7002;
    second.TemplateId = 101;
    PlayerBackpack backpack = PlayerBackpack::FromStored({ second });
    PlayerEquipment equipment = PlayerEquipment::FromStored({ { first, "Deck" } });
    std::vector<CharacterDeckSpell> const rows{ { 7001, 301, 3, 0, 4 }, { 7002, 301, 1, 0, 9 }, { 7002, 302, 2, 1, 7 } };

    auto selected = DeckStateRestorer::RestoreEquipped(equipment, *templates, rows);
    ASSERT_TRUE(selected.Valid);
    ASSERT_TRUE(selected.Deck);
    EXPECT_EQ(selected.Deck->ItemGuid, first.Guid);
    EXPECT_EQ(selected.Deck->Cards.Count(301), 3u);
    EXPECT_EQ(selected.Deck->RevisionOf(301), 4u);

    EquipChange const swap = equipment.Equip(backpack, 2, slots, *templates, wizard, second.Guid, "Deck");
    ASSERT_EQ(swap.Result, EquipResult::Equipped);
    ASSERT_TRUE(swap.Returned);
    EXPECT_EQ(swap.Returned->Guid, first.Guid);
    selected = DeckStateRestorer::RestoreEquipped(equipment, *templates, rows);
    ASSERT_TRUE(selected.Valid);
    ASSERT_TRUE(selected.Deck);
    EXPECT_EQ(selected.Deck->ItemGuid, second.Guid);
    EXPECT_EQ(selected.Deck->Cards.Count(301), 1u);
    EXPECT_EQ(selected.Deck->Cards.Count(302), 2u);
    EXPECT_EQ(selected.Deck->RevisionOf(301), 9u);

    EXPECT_EQ(equipment.Unequip(backpack, 2, second.Guid).Result, UnequipResult::Unequipped);
    selected = DeckStateRestorer::RestoreEquipped(equipment, *templates, rows);
    EXPECT_TRUE(selected.Valid);
    EXPECT_FALSE(selected.Deck);
    EXPECT_EQ(equipment.Equip(backpack, 2, slots, *templates, wizard, first.Guid, "Deck").Result, EquipResult::Equipped);
    selected = DeckStateRestorer::RestoreEquipped(equipment, *templates, rows);
    ASSERT_TRUE(selected.Valid);
    ASSERT_TRUE(selected.Deck);
    EXPECT_EQ(selected.Deck->ItemGuid, first.Guid);
    EXPECT_EQ(selected.Deck->Cards.Count(301), 3u);
    EXPECT_EQ(selected.Deck->Cards.Count(302), 0u);
    EXPECT_EQ(selected.Deck->RevisionOf(301), 4u);
}

TEST(DeckStateRestorerTest, InvalidSavedReplacementDeckCanBeRejectedBeforeMovingAnyRealItems)
{
    auto const templates = ItemTemplateStore::Build({ DeckTemplate(101, 4) });
    ASSERT_TRUE(templates);
    EquipSlot slot;
    slot.Name = "Deck";
    slot.AdjectivesAnd = { "Deck" };
    EquipmentSlots const slots = EquipmentSlots::FromSlots({ slot });
    EquipWizard const wizard("Ice", 20);
    CharacterItem first;
    first.Guid = 7001;
    first.TemplateId = 101;
    CharacterItem second;
    second.Guid = 7002;
    second.TemplateId = 101;
    PlayerBackpack const backpack = PlayerBackpack::FromStored({ second });
    PlayerEquipment const equipment = PlayerEquipment::FromStored({ { first, "Deck" } });

    PlayerBackpack candidateBackpack = backpack;
    PlayerEquipment candidateEquipment = equipment;
    EquipChange const candidate = candidateEquipment.Equip(candidateBackpack, 2, slots, *templates, wizard, second.Guid, "Deck");
    ASSERT_EQ(candidate.Result, EquipResult::Equipped);
    EXPECT_FALSE(DeckStateRestorer::RestoreEquipped(candidateEquipment, *templates, { { 7002, 301, 5, 0, 1 } }).Valid);
    EXPECT_NE(backpack.Find(second.Guid), nullptr);
    EXPECT_EQ(equipment.Find(first.Guid)->Item.Guid, first.Guid);
    EXPECT_EQ(equipment.Find(second.Guid), nullptr);
}
