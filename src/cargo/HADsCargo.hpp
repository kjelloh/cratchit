#pragma once

#include "CargoBase.hpp"
#include "fiscal/amount/HADFramework.hpp"

// Cargo visit/apply double dispatch removed (cargo now message passed)
// namespace first {
//   namespace cargo {
//     using HADsCargo = ConcreteCargo<HeadingAmountDateTransEntries>;    
//   } // namespace cargo
// } // namespace first