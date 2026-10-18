/*
 * Project Ambrose by Imjustchico
 * Stores a wizard's ordinary deck spell quantities and refuses additions or removals that exceed the equipped deck's supplied limits or refer to absent spells.
 */

#ifndef AMBROSE_PLAYERDECK_H
#define AMBROSE_PLAYERDECK_H

#include "Types.h"

#include <cstddef>
#include <vector>

struct DeckSpell
{
    uint32 SpellId = 0;
    uint32 Quantity = 0;

    bool operator==(DeckSpell const&) const = default;
};

struct DeckLimits
{
    uint32 MaxCards = 0;
    uint32 MaxCopiesForSpell = 0;
};

enum class DeckEditResult : uint8
{
    Added,
    Removed,
    InvalidSpell,
    InvalidLimits,
    DeckFull,
    CopyLimit,
    NotFound
};

class PlayerDeck
{
public:
    DeckEditResult Add(uint32 spellId, DeckLimits limits);
    DeckEditResult Remove(uint32 spellId);

    std::vector<DeckSpell> const& GetSpells() const noexcept { return _spells; }
    std::size_t TotalCards() const noexcept { return _totalCards; }
    uint32 Count(uint32 spellId) const noexcept;

private:
    std::vector<DeckSpell> _spells;
    std::size_t _totalCards = 0;
};

#endif