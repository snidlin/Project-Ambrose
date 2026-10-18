/*
 * Project Ambrose by Imjustchico
 * Restores only the selected deck item's stored cards, preserving zero-quantity tombstones and refusing malformed or duplicate rows.
 */

#include "DeckStateRestorer.h"

#include <algorithm>
#include <utility>
#include <vector>

uint64 RestoredDeckState::RevisionOf(uint32 spellId) const noexcept
{
    auto const found = std::find_if(Rows.begin(), Rows.end(), [spellId](CharacterDeckSpell const& row) { return row.SpellId == spellId; });
    return found == Rows.end() ? 0 : found->Revision;
}

std::optional<RestoredDeckState> DeckStateRestorer::Restore(uint64 deckItemGuid, uint32 maxCards, std::vector<CharacterDeckSpell> const& stored)
{
    if (deckItemGuid == 0 || maxCards == 0)
        return std::nullopt;

    RestoredDeckState state;
    state.ItemGuid = deckItemGuid;
    for (CharacterDeckSpell const& row : stored)
    {
        if (row.DeckItemGuid != deckItemGuid)
            continue;
        if (row.SpellId == 0 || row.Revision == 0)
            return std::nullopt;
        if (std::any_of(state.Rows.begin(), state.Rows.end(), [&](CharacterDeckSpell const& existing) { return existing.SpellId == row.SpellId; }))
            return std::nullopt;
        state.Rows.push_back(row);
    }
    std::sort(state.Rows.begin(), state.Rows.end(), [](CharacterDeckSpell const& lhs, CharacterDeckSpell const& rhs)
    {
        if (lhs.SortOrder != rhs.SortOrder)
            return lhs.SortOrder < rhs.SortOrder;
        return lhs.SpellId < rhs.SpellId;
    });

    std::vector<DeckSpell> cards;
    cards.reserve(state.Rows.size());
    for (CharacterDeckSpell const& row : state.Rows)
    {
        if (row.Quantity != 0)
            cards.push_back({ row.SpellId, row.Quantity });
    }
    std::optional<PlayerDeck> const deck = PlayerDeck::FromStored(std::move(cards), maxCards);
    if (!deck)
        return std::nullopt;
    state.Cards = *deck;
    return state;
}
