#pragma once
#include "fiscal/BASFramework.hpp" // BAS::MDJournalEntry

std::size_t hash_of_id_and_all_content(BAS::MDJournalEntry const& entry);
