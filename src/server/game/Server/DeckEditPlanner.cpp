/*
 * Project Ambrose by Imjustchico
 * Computes a proposed deck change and the owned item's next revisioned spell row, leaving the caller to commit the write before accepting it.
 */

#include "DeckEditPlanner.h"

#include <limits>
#include <utility>

DeckEditPlan DeckEditPlanner::Prepare(DeckEditAction action, PlayerDeck const& current, uint64 deckItemGuid, uint32 spellId,
    DeckCopyCategory category, DeckLimits limits, uint32 sortOrder, uint64 previousRevision)
{
    if (deckItemGuid == 0 || spellId == 0 || previousRevision == std::numeric_limits<uint64>::max())
        return {};
    PlayerDeck candidate = current;
    DeckEditResult result = DeckEditResult::InvalidLimits;
    switch (action)
    {
        case DeckEditAction::Add:
            result = candidate.Add(spellId, category, limits);
            break;
        case DeckEditAction::Remove:
            result = candidate.Remove(spellId);
            break;
        default:
            return {};
    }
    DeckEditPlan plan;
    plan.Result = result;
    if (result != DeckEditResult::Added && result != DeckEditResult::Removed)
        return plan;
    CharacterDeckSpell row;
    row.DeckItemGuid = deckItemGuid;
    row.SpellId = spellId;
    row.Quantity = candidate.Count(spellId);
    row.SortOrder = sortOrder;
    row.Revision = previousRevision + 1;
    plan.Candidate.emplace(DeckEditCandidate{ std::move(candidate), row });
    return plan;
}
