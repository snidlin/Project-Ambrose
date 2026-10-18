/*
 * Project Ambrose by Imjustchico
 * Verifies the observed Wizard service deck edit wire fields and signed spell-id bit pattern against the synthetic DML contracts.
 */

#include "GameMessages.h"
#include "GameTestHarness.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace
{
    template<class Message>
    void CheckDeckWire(uint8 expectedOrder)
    {
        GameTesting::GameDefinitions definitions;
        MessageCatalogPtr const catalog = sMessageRegistry.GetCatalog();
        ASSERT_TRUE(catalog);
        ASSERT_TRUE(catalog->IsDeclared<Message>());
        MessageInfo const& info = catalog->GetInfo<Message>();
        ASSERT_TRUE(info.Protocol);
        ASSERT_TRUE(info.Definition);
        EXPECT_EQ(info.Protocol->ServiceId, 12u);
        EXPECT_EQ(info.Definition->Order, expectedOrder);

        Message sent;
        sent.SpellId = static_cast<int32>(0xF08E1234u);
        sent.DeckId = 0x0102030405060708ull;
        sent.Success = 1;

        ByteBuffer encoded;
        catalog->Encode(sent, encoded);
        ByteBuffer wire(encoded.GetData());
        EXPECT_EQ(wire.Read<int32>(), sent.SpellId);
        EXPECT_EQ(wire.Read<uint64>(), sent.DeckId);
        EXPECT_EQ(wire.Read<uint8>(), 1u);
        EXPECT_EQ(wire.GetRemaining(), 0u);

        ByteBuffer again(encoded.GetData());
        Message decoded;
        ASSERT_TRUE(catalog->Decode(again, decoded));
        EXPECT_EQ(decoded.SpellId, sent.SpellId);
        EXPECT_EQ(decoded.DeckId, sent.DeckId);
        EXPECT_EQ(decoded.Success, sent.Success);
    }
}

TEST(DeckWireTest, AddSpellToDeckUsesWizardServiceOrderEleven)
{
    CheckDeckWire<GameMessages::AddSpellToDeck>(11);
}

TEST(DeckWireTest, RemoveSpellFromDeckUsesWizardServiceOrderOneHundredFiftySix)
{
    CheckDeckWire<GameMessages::RemoveSpellFromDeck>(156);
}
