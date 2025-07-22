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
    
    if (m_mod10_view.empty()) {
      return "Account Statements (no files)";
    }
    
    return std::format("Account Statements ({} files, showing {})", 
                      m_file_paths.size(), m_mod10_view.to_string());
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
    
    if (m_mod10_view.empty()) {
      result.push_back("No files found in from_bank_or_skv directory");
      return result;
    }
    
    result.push_back(std::format("Files in from_bank_or_skv directory ({})", m_mod10_view.to_string()));
    
    if (m_mod10_view.needs_pagination()) {
      result.push_back("Navigate using digits 0-9 to drill down:");
    }
    
    for (size_t i : m_mod10_view) {
      if (i < m_file_paths.size()) {
        result.push_back(std::format("{}. {}", i, m_file_paths[i].filename().string()));
      }
    }
    
    return result;
  }

  StateImpl::UpdateOptions AccountStatementsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    if (m_mod10_view.empty()) {
      spdlog::info("AccountStatementsState: No files to create options for");
      // Only add back navigation when no files
      result.add('-', {"Back", []() -> StateUpdateResult {
        using StringMsg = CargoMsgT<std::string>;
        return {std::nullopt, []() -> std::optional<Msg> {
          return std::make_shared<PopStateMsg>(
            std::make_shared<StringMsg>("AccountStatementsState - no files")
          );
        }};
      }});
      return result;
    }
    
    // Use enhanced API for cleaner digit-based option creation  
    for (char digit = '0'; digit <= '9'; ++digit) {
      if (!m_mod10_view.is_valid_digit(digit)) {
        break; // No more valid digits
      }
      
      // Try to get direct index for single file
      if (auto index = m_mod10_view.direct_index(digit); index.has_value() && *index < m_file_paths.size()) {
        // Single file - direct navigation to CSVFileState
        auto file_path = m_file_paths[*index];
        auto caption = file_path.filename().string();
        result.add(digit, {caption, [file_path]() -> StateUpdateResult {
          return {std::nullopt, [file_path]() -> std::optional<Msg> {
            State new_state = make_state<CSVFileState>(file_path);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      } else {
        // Subrange - use drill_down for navigation
        try {
          auto drilled_view = m_mod10_view.drill_down(digit);
          auto caption = drilled_view.is_single_item() 
            ? std::format("File {}", drilled_view.first())
            : std::format("Files {} .. {}", drilled_view.first(), drilled_view.second() - 1);
          
          result.add(digit, {caption, [file_paths = m_file_paths, drilled_view]() -> StateUpdateResult {
            return {std::nullopt, [file_paths, drilled_view]() -> std::optional<Msg> {
              State new_state = make_state<AccountStatementsState>(file_paths, drilled_view);
              return std::make_shared<PushStateMsg>(new_state);
            }};
          }});
        } catch (const std::exception& e) {
          spdlog::error("AccountStatementsState: Error creating option for digit '{}': {}", digit, e.what());
        }
      }
    }
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", [view_info = m_mod10_view.to_string()]() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, [view_info]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementsState - " + view_info)
        );
      }};
    }});
    
    return result;
  }

}