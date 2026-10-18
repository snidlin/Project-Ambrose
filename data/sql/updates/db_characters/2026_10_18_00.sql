-- Project Ambrose by Imjustchico
-- Holds ordinary spell quantities for each owned deck item, with per-spell revisions and zero-quantity tombstones so older asynchronous saves cannot resurrect removed cards.
CREATE TABLE IF NOT EXISTS `character_deck_spell` (
    `owner_guid` BIGINT UNSIGNED NOT NULL,
    `deck_item_guid` BIGINT UNSIGNED NOT NULL,
    `spell_id` INT UNSIGNED NOT NULL,
    `quantity` INT UNSIGNED NOT NULL DEFAULT 0,
    `sort_order` INT UNSIGNED NOT NULL DEFAULT 0,
    `revision` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`owner_guid`, `deck_item_guid`, `spell_id`),
    KEY `idx_character_deck_item` (`deck_item_guid`),
    CONSTRAINT `fk_character_deck_spell_owner` FOREIGN KEY (`owner_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE,
    CONSTRAINT `fk_character_deck_spell_item` FOREIGN KEY (`deck_item_guid`) REFERENCES `item_instance` (`guid`) ON DELETE CASCADE,
    CONSTRAINT `chk_character_deck_spell_id` CHECK (`spell_id` > 0),
    CONSTRAINT `chk_character_deck_spell_revision` CHECK (`revision` > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
