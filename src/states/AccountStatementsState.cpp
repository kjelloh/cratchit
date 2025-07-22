#include "AccountStatementsState.hpp"
#include "CSVFileState.hpp"
#include <format>
#include <spdlog/spdlog.h>

namespace first {
  
  AccountStatementsState::AccountStatementsState() 
    : StateImpl{}, m_file_paths{scan_from_bank_or_skv_directory()}, m_mod10_view{m_file_paths} {
    
    spdlog::info("AccountStatementsState - found {} files: {}", m_file_paths.size(), m_mod10_view.to_string());
  }

  AccountStatementsState::AccountStatementsState(FilePaths file_paths, Mod10View mod10_view)
    : StateImpl{}, m_file_paths{std::move(file_paths)}, m_mod10_view{mod10_view} {
    
    spdlog::info("AccountStatementsState - {} files: {}", m_file_paths.size(), m_mod10_view.to_string());
  }

  std::string AccountStatementsState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return std::format("Account Statements ({} files)", m_file_paths.size());
  }

  AccountStatementsState::FilePaths AccountStatementsState::scan_from_bank_or_skv_directory() const {
    FilePaths result;
    std::filesystem::path dir_path = "from_bank_or_skv";
    
    try {
      if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
          if (entry.is_regular_file()) {
            result.push_back(entry.path());
            spdlog::info("Found file: {}", entry.path().string());
          }
        }
      } else {
        spdlog::info("Directory 'from_bank_or_skv' does not exist or is not a directory");
      }
    } catch (const std::exception& e) {
      spdlog::error("Error scanning directory 'from_bank_or_skv': {}", e.what());
    }
    
    return result;
  }

  StateImpl::UX AccountStatementsState::create_ux() const {
    UX result{};
    result.push_back(std::format("Files in from_bank_or_skv directory:"));
    
    for (size_t i : m_mod10_view) {
      if (i < m_file_paths.size()) {
        result.push_back(std::format("{}. {}", i, m_file_paths[i].filename().string()));
      }
    }
    
    return result;
  }

  StateImpl::UpdateOptions AccountStatementsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    auto subranges = m_mod10_view.subranges();
    for (size_t i = 0; i < subranges.size(); ++i) {
      auto const subrange = subranges[i];
      auto const& [begin, end] = subrange;
      char key = static_cast<char>('0' + i);
      
      if (subrange.second - subrange.first == 1 && begin < m_file_paths.size()) {
        // Single file - direct navigation to CSVFileState
        auto file_path = m_file_paths[begin];
        auto caption = file_path.filename().string();
        result.add(key, {caption, [file_path]() -> StateUpdateResult {
          return {std::nullopt, [file_path]() -> std::optional<Msg> {
            State new_state = make_state<CSVFileState>(file_path);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      }
      else {
        // Subrange - drill down navigation
        auto caption = std::format("{} .. {}", subrange.first, subrange.second - 1);
        result.add(key, {caption, [file_paths = m_file_paths, subrange]() -> StateUpdateResult {
          return {std::nullopt, [file_paths, subrange]() -> std::optional<Msg> {
            Mod10View drilled_view(subrange);
            State new_state = make_state<AccountStatementsState>(file_paths, drilled_view);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      }
    }
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementsState")
        );
      }};
    }});
    
    return result;
  }

}