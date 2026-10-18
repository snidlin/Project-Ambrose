/*
 * Project Ambrose by Imjustchico
 * Stores one ordinary spell quantity for one owned deck item, retaining a revision and order so removed cards remain removed across delayed saves.
 */

#ifndef AMBROSE_CHARACTERDECKSPELL_H
#define AMBROSE_CHARACTERDECKSPELL_H

#include "Types.h"

struct CharacterDeckSpell
{
    uint64 DeckItemGuid = 0;
    uint32 SpellId = 0;
    uint32 Quantity = 0;
    uint32 SortOrder = 0;
    uint64 Revision = 0;

    bool operator==(CharacterDeckSpell const&) const = default;
};

#endif
