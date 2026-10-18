/*
 * Project Ambrose by Imjustchico
 * Restores only the selected deck item's stored cards, preserving zero-quantity tombstones and refusing malformed or duplicate rows.
 */

#include "DeckStateRestorer.h"
#include "ItemTemplateStore.h"
#include "PlayerEquipment.h"

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

EquippedDeckRestore DeckStateRestorer::RestoreEquipped(PlayerEquipment const& equipment, ItemTemplateStore const& templates,
    std::vector<CharacterDeckSpell> const& stored)
{
    EquippedDeckRestore result;
    for (CharacterEquippedItem const& worn : equipment.GetItems())
    {
        if (worn.Item.Guid == 0)
            return { false, std::nullopt };
        ItemTemplateRecord const* const item = templates.Find(worn.Item.TemplateId);
        if (!item)
            return { false, std::nullopt };
        if (!item->Deck)
            continue;
        if (result.Deck || item->Deck->MaxSpells <= 0)
            return { false, std::nullopt };
        std::optional<RestoredDeckState> restored = Restore(worn.Item.Guid, static_cast<uint32>(item->Deck->MaxSpells), stored);
        if (!restored)
            return { false, std::nullopt };
        result.Deck = std::move(restored);
    }
    return result;
}
