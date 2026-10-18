/*
 * Project Ambrose by Imjustchico
 * Describes a candidate deck edit and its revisioned database row without mutating the live deck before storage confirms the change.
 */

#ifndef AMBROSE_DECKEDITPLANNER_H
#define AMBROSE_DECKEDITPLANNER_H

#include "CharacterDeckSpell.h"
#include "PlayerDeck.h"

#include <optional>

enum class DeckEditAction : uint8
{
    Add,
    Remove
};

struct DeckEditCandidate
{
    PlayerDeck Cards;
    CharacterDeckSpell Save;
};

struct DeckEditPlan
{
    DeckEditResult Result = DeckEditResult::InvalidLimits;
    std::optional<DeckEditCandidate> Candidate;
};

class DeckEditPlanner
{
public:
    static DeckEditPlan Prepare(DeckEditAction action, PlayerDeck const& current, uint64 deckItemGuid, uint32 spellId, DeckCopyCategory category,
        DeckLimits limits, uint32 sortOrder, uint64 previousRevision);
};

#endif
