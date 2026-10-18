/*
 * Project Ambrose by Imjustchico
 * Tests that only learned ordinary spells for a worn deck with valid template data pass the pre-classification admission checks.
 */

#include "DeckAdmission.h"
#include "ItemTemplateStore.h"
#include "PlayerEquipment.h"
#include "PlayerSpellbook.h"
#include "StringHash.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr uint32 DeckTemplateId = 1001;
    constexpr uint32 OtherTemplateId = 1002;
    constexpr uint64 DeckGuid = 1234567;

    uint32 SpellId(std::string_view name)
    {
        return StringHash::KiStringHash(name);
    }

    ItemTemplateRecord Item(uint32 id, bool isDeck = false)
    {
        ItemTemplateRecord item;
        item.TemplateId = id;
        item.ObjectName = "Test item " + std::to_string(id);
        if (isDeck)
        {
            item.Deck.emplace();
            item.Deck->PrimarySchool = "Fire";
            item.Deck->GenericMaxRank = -1;
            item.Deck->SchoolMaxRank = -1;
            item.Deck->GenericMaxInstances = 4;
            item.Deck->SchoolMaxInstances = 6;
            item.Deck->MaxSpells = 64;
            item.Deck->MaxTreasureCards = 36;
        }
        return item;
    }

    PlayerEquipment Worn(uint32 templateId = DeckTemplateId)
    {
        CharacterItem item;
        item.Guid = DeckGuid;
        item.TemplateId = templateId;
        return PlayerEquipment::FromStored({ { item, "Deck" } });
    }

    SpellInfo Spell(std::string name, bool treasure = false)
    {
        SpellInfo spell;
        spell.Name = std::move(name);
        spell.TemplateId = SpellId(spell.Name);
        spell.File = "Spells/" + spell.Name + ".xml";
        spell.Treasure = treasure;
        return spell;
    }

    std::shared_ptr<SpellStore const> Spells()
    {
        std::vector<std::string> errors;
        return SpellStore::Build({ Spell("Fire Cat"), Spell("Pet - Fire Cat"), Spell("Fire Cat TC", true) }, errors);
    }
}

TEST(DeckAdmissionTest, AKnownSpellOnAnEquippedDeckReturnsTheRealTemplateCaps)
{
    auto const items = ItemTemplateStore::Build({ Item(DeckTemplateId, true), Item(OtherTemplateId) });
    auto const spells = Spells();
    ASSERT_TRUE(items);
    ASSERT_TRUE(spells);
    PlayerSpellbook book;
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat")));
    EquippedDeckFacts facts;

    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat"), Worn(), *items, book, *spells, facts),
        DeckPrerequisiteResult::ReadyForClassification);
    EXPECT_EQ(facts.ItemGuid, DeckGuid);
    EXPECT_EQ(facts.ItemTemplateId, DeckTemplateId);
    EXPECT_EQ(facts.Limits.MaxCards, 64u);
    EXPECT_EQ(facts.Limits.MaxGenericCopies, 4u);
    EXPECT_EQ(facts.Limits.MaxSchoolCopies, 6u);
    EXPECT_EQ(facts.PrimarySchool, "Fire");
    EXPECT_EQ(facts.GenericMaxRank, -1);
    EXPECT_EQ(facts.SchoolMaxRank, -1);
}

TEST(DeckAdmissionTest, ADeckInTheBackpackOrAnotherDeckIdIsNotAccepted)
{
    auto const items = ItemTemplateStore::Build({ Item(DeckTemplateId, true) });
    auto const spells = Spells();
    ASSERT_TRUE(items);
    ASSERT_TRUE(spells);
    PlayerSpellbook book;
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat")));
    EquippedDeckFacts facts;

    EXPECT_EQ(DeckAdmission::Check(0, SpellId("Fire Cat"), Worn(), *items, book, *spells, facts), DeckPrerequisiteResult::InvalidDeckId);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid + 1, SpellId("Fire Cat"), Worn(), *items, book, *spells, facts), DeckPrerequisiteResult::NotEquipped);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat"), PlayerEquipment{}, *items, book, *spells, facts),
        DeckPrerequisiteResult::NotEquipped);
    EXPECT_EQ(facts.ItemGuid, 0u);
}

TEST(DeckAdmissionTest, ANonDeckItemAndAMissingItemTemplateAreRejected)
{
    auto const items = ItemTemplateStore::Build({ Item(OtherTemplateId) });
    auto const spells = Spells();
    ASSERT_TRUE(items);
    ASSERT_TRUE(spells);
    PlayerSpellbook book;
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat")));
    EquippedDeckFacts facts;

    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat"), Worn(OtherTemplateId), *items, book, *spells, facts),
        DeckPrerequisiteResult::NotADeck);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat"), Worn(), *items, book, *spells, facts),
        DeckPrerequisiteResult::MissingDeckTemplate);
    EXPECT_EQ(facts.ItemGuid, 0u);
}

TEST(DeckAdmissionTest, AbsentUnlearnedAndTreasureSpellsCannotEnterTheOrdinaryDeck)
{
    auto const items = ItemTemplateStore::Build({ Item(DeckTemplateId, true) });
    auto const spells = Spells();
    ASSERT_TRUE(items);
    ASSERT_TRUE(spells);
    PlayerSpellbook book;
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat")));
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat TC")));
    EquippedDeckFacts facts;

    EXPECT_EQ(DeckAdmission::Check(DeckGuid, 0, Worn(), *items, book, *spells, facts), DeckPrerequisiteResult::InvalidSpell);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Absent Spell"), Worn(), *items, book, *spells, facts), DeckPrerequisiteResult::NoSuchSpell);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Pet - Fire Cat"), Worn(), *items, book, *spells, facts),
        DeckPrerequisiteResult::NotLearned);
    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat TC"), Worn(), *items, book, *spells, facts),
        DeckPrerequisiteResult::TreasureSpell);
    EXPECT_EQ(facts.ItemGuid, 0u);
}

TEST(DeckAdmissionTest, InvalidDeckCapacityLeavesOutputUntouched)
{
    ItemTemplateRecord deck = Item(DeckTemplateId, true);
    deck.Deck->GenericMaxInstances = 0;
    auto const items = ItemTemplateStore::Build({ std::move(deck) });
    auto const spells = Spells();
    ASSERT_TRUE(items);
    ASSERT_TRUE(spells);
    PlayerSpellbook book;
    ASSERT_TRUE(book.Learn(SpellId("Fire Cat")));
    EquippedDeckFacts facts;
    facts.ItemGuid = 444;

    EXPECT_EQ(DeckAdmission::Check(DeckGuid, SpellId("Fire Cat"), Worn(), *items, book, *spells, facts),
        DeckPrerequisiteResult::InvalidDeckLimits);
    EXPECT_EQ(facts.ItemGuid, 444u);
}
