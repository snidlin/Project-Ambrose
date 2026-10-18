/*
 * Project Ambrose by Imjustchico
 * Validates that a deck is worn and an ordinary spell is learned before editing, without assuming the client's school category or rank-limit semantics.
 */

#ifndef AMBROSE_DECKADMISSION_H
#define AMBROSE_DECKADMISSION_H

#include "PlayerDeck.h"
#include "SpellMgr.h"
#include "Types.h"

#include <string>

class ItemTemplateStore;
class PlayerEquipment;
class PlayerSpellbook;

enum class DeckPrerequisiteResult : uint8
{
    ReadyForClassification,
    InvalidDeckId,
    NotEquipped,
    MissingDeckTemplate,
    NotADeck,
    InvalidDeckLimits,
    InvalidSpell,
    NoSuchSpell,
    NotLearned,
    TreasureSpell
};

struct EquippedDeckFacts
{
    uint64 ItemGuid = 0;
    uint32 ItemTemplateId = 0;
    DeckLimits Limits;
    std::string PrimarySchool;
    int32 GenericMaxRank = 0;
    int32 SchoolMaxRank = 0;
};

class DeckAdmission
{
public:
    static DeckPrerequisiteResult Check(uint64 deckGuid, uint32 spellId, PlayerEquipment const& equipment, ItemTemplateStore const& items,
        PlayerSpellbook const& book, SpellStore const& spells, EquippedDeckFacts& facts);
};

#endif
