/*
 * Project Ambrose by Imjustchico
 * Applies authoritative deck copy and size limits before changing a wizard's spell quantities, leaving the deck unchanged when an edit is refused.
 */

#include "PlayerDeck.h"

#include <algorithm>

uint32 PlayerDeck::Count(uint32 spellId) const noexcept
{
    auto const entry = std::find_if(_spells.begin(), _spells.end(), [spellId](DeckSpell const& spell) { return spell.SpellId == spellId; });
    return entry == _spells.end() ? 0 : entry->Quantity;
}

DeckEditResult PlayerDeck::Add(uint32 spellId, DeckLimits limits)
{
    if (spellId == 0)
        return DeckEditResult::InvalidSpell;
    if (limits.MaxCards == 0 || limits.MaxCopiesForSpell == 0)
        return DeckEditResult::InvalidLimits;
    auto const entry = std::find_if(_spells.begin(), _spells.end(), [spellId](DeckSpell const& spell) { return spell.SpellId == spellId; });
    if (entry != _spells.end() && entry->Quantity >= limits.MaxCopiesForSpell)
        return DeckEditResult::CopyLimit;
    if (_totalCards >= limits.MaxCards)
        return DeckEditResult::DeckFull;
    if (entry == _spells.end())
        _spells.push_back({ spellId, 1 });
    else
        ++entry->Quantity;
    ++_totalCards;
    return DeckEditResult::Added;
}

DeckEditResult PlayerDeck::Remove(uint32 spellId)
{
    auto const entry = std::find_if(_spells.begin(), _spells.end(), [spellId](DeckSpell const& spell) { return spell.SpellId == spellId; });
    if (entry == _spells.end())
        return DeckEditResult::NotFound;
    if (--entry->Quantity == 0)
        _spells.erase(entry);
    --_totalCards;
    return DeckEditResult::Removed;
}
