#pragma once

#include "StateImpl.hpp"
#include "csv/parse_csv.hpp"
#include <string>
#include <filesystem>
#include <optional>

namespace first {
  class AccountStatementFileState : public StateImpl {
  public:
    AccountStatementFileState(const AccountStatementFileState&) = delete;
    AccountStatementFileState(std::filesystem::path file_path);
    // AccountStatementFileState(std::optional<std::string> caption, std::filesystem::path file_path);

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    virtual std::string caption() const override;

    const std::filesystem::path& file_path() const { return m_file_path; }

  private:
    std::filesystem::path m_file_path;
    CSV::ParseCSVResult m_parse_csv_result;
    
  }; // AccountStatementFileState
}