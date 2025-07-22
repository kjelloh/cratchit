#include "CSVFileState.hpp"
#include <format>

namespace first {
  
  CSVFileState::CSVFileState(std::filesystem::path file_path)
    : StateImpl{}, m_file_path{std::move(file_path)} {}

  CSVFileState::CSVFileState(std::optional<std::string> caption, std::filesystem::path file_path)
    : StateImpl{caption}, m_file_path{std::move(file_path)} {}

  std::string CSVFileState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return std::format("CSV File: {}", m_file_path.filename().string());
  }

  StateImpl::UpdateOptions CSVFileState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("CSVFileState")
        );
      }};
    }});
    
    return result;
  }

}