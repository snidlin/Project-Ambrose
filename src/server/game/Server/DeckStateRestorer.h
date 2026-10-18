/*
 * Project Ambrose by Imjustchico
 * Reconstructs the ordinary cards and revision tombstones for exactly one equipped deck item from character database rows.
 */

#ifndef AMBROSE_DECKSTATERESTORER_H
#define AMBROSE_DECKSTATERESTORER_H

#include "CharacterDeckSpell.h"
#include "PlayerDeck.h"

#include <optional>
#include <vector>

class ItemTemplateStore;
class PlayerEquipment;

struct RestoredDeckState
{
    uint64 ItemGuid = 0;
    PlayerDeck Cards;
    std::vector<CharacterDeckSpell> Rows;

    uint64 RevisionOf(uint32 spellId) const noexcept;
};

struct EquippedDeckRestore
{
    bool Valid = true;
    std::optional<RestoredDeckState> Deck;
};

class DeckStateRestorer
{
public:
    static std::optional<RestoredDeckState> Restore(uint64 deckItemGuid, uint32 maxCards, std::vector<CharacterDeckSpell> const& stored);
    static EquippedDeckRestore RestoreEquipped(PlayerEquipment const& equipment, ItemTemplateStore const& templates,
        std::vector<CharacterDeckSpell> const& stored);
};

#endif
