#pragma once

#include "StateImpl.hpp"
#include "csv/parse_csv.hpp"
#include "../PeriodConstrainedContent.hpp" 
#include <string>
#include <filesystem>
#include <optional>

namespace first {
  class AccountStatementFileState : public StateImpl {
  public:
    using PeriodPairedFilePath = PeriodPairedT<std::filesystem::path>;
    AccountStatementFileState(const AccountStatementFileState&) = delete;
    // AccountStatementFileState(std::filesystem::path file_path);
    AccountStatementFileState(PeriodPairedFilePath period_paired_file_path);

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    virtual std::string caption() const override;

    const std::filesystem::path& file_path() const { return m_period_paired_file_path.content(); }

  private:
    // std::filesystem::path m_file_path;
    PeriodPairedFilePath m_period_paired_file_path;
    AnnotatedMaybe<CSV::Table> m_maybe_table_result;
    
  }; // AccountStatementFileState
}