#pragma once

#include <optional>
#include <map>
#include <vector>
#include <string>

namespace SKV {
	namespace SRU {
		using AccountNo = unsigned int;
		using OptionalAccountNo = std::optional<AccountNo>;
		using SRUValueMap = std::map<AccountNo,std::optional<std::string>>;
		using OptionalSRUValueMap = std::optional<SRUValueMap>;
		using SRUValueMaps = std::vector<SRUValueMap>;

    } // namespace SRU
} // namespace SKV

