/*
 * Project Ambrose by Imjustchico
 * Holds a wizard's ordinary spell quantities with caller-supplied deck limits.
 */

#ifndef AMBROSE_PLAYERDECK_H
#define AMBROSE_PLAYERDECK_H

#include "Types.h"

#include <cstddef>
#include <optional>
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
    uint32 MaxGenericCopies = 0;
    uint32 MaxSchoolCopies = 0;
};

enum class DeckCopyCategory : uint8
{
    Generic,
    School
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
    DeckEditResult Add(uint32 spellId, DeckCopyCategory category, DeckLimits limits);
    DeckEditResult Remove(uint32 spellId);
    static std::optional<PlayerDeck> FromStored(std::vector<DeckSpell> spells, uint32 maxCards);

    std::vector<DeckSpell> const& GetSpells() const noexcept { return _spells; }
    std::size_t TotalCards() const noexcept { return _totalCards; }
    uint32 Count(uint32 spellId) const noexcept;

private:
    std::vector<DeckSpell> _spells;
    std::size_t _totalCards = 0;
};

#endif
