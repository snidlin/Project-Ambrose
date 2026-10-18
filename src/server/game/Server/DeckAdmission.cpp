/*
 * Project Ambrose by Imjustchico
 * Checks an equipped deck item's template, capacity, and a learned ordinary spell before any school classification or deck mutation.
 */

#include "DeckAdmission.h"
#include "ItemTemplateStore.h"
#include "PlayerEquipment.h"
#include "PlayerSpellbook.h"

DeckPrerequisiteResult DeckAdmission::Check(uint64 deckGuid, uint32 spellId, PlayerEquipment const& equipment, ItemTemplateStore const& items,
    PlayerSpellbook const& book, SpellStore const& spells, EquippedDeckFacts& facts)
{
    if (deckGuid == 0)
        return DeckPrerequisiteResult::InvalidDeckId;
    CharacterEquippedItem const* const worn = equipment.Find(deckGuid);
    if (!worn)
        return DeckPrerequisiteResult::NotEquipped;
    ItemTemplateRecord const* const item = items.Find(worn->Item.TemplateId);
    if (!item)
        return DeckPrerequisiteResult::MissingDeckTemplate;
    if (!item->Deck)
        return DeckPrerequisiteResult::NotADeck;

    DeckTemplateSettings const& templateSettings = *item->Deck;
    if (templateSettings.MaxSpells <= 0 || templateSettings.GenericMaxInstances <= 0 || templateSettings.SchoolMaxInstances <= 0)
        return DeckPrerequisiteResult::InvalidDeckLimits;
    if (spellId == 0)
        return DeckPrerequisiteResult::InvalidSpell;
    SpellInfo const* const spell = spells.Find(spellId);
    if (!spell)
        return DeckPrerequisiteResult::NoSuchSpell;
    if (!book.Knows(spellId))
        return DeckPrerequisiteResult::NotLearned;
    if (spell->Treasure)
        return DeckPrerequisiteResult::TreasureSpell;

    EquippedDeckFacts accepted;
    accepted.ItemGuid = worn->Item.Guid;
    accepted.ItemTemplateId = worn->Item.TemplateId;
    accepted.Limits = { static_cast<uint32>(templateSettings.MaxSpells), static_cast<uint32>(templateSettings.GenericMaxInstances),
        static_cast<uint32>(templateSettings.SchoolMaxInstances) };
    accepted.PrimarySchool = templateSettings.PrimarySchool;
    accepted.GenericMaxRank = templateSettings.GenericMaxRank;
    accepted.SchoolMaxRank = templateSettings.SchoolMaxRank;
    facts = std::move(accepted);
    return DeckPrerequisiteResult::ReadyForClassification;
}
