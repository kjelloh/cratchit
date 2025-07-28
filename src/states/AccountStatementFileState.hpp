#pragma once

#include "StateImpl.hpp"
#include "csv/projections.hpp"
#include "text/encoding.hpp"
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
    struct ParseCSVResult {
      encoding::icu::EncodingDetectionResult icu_detection_result;
      CSV::project::HeadingId heading_id;
      CSV::OptionalTable maybe_table;
    };    

    std::filesystem::path m_file_path;
    ParseCSVResult m_parse_csv_result;

    std::string encoding_caption() const;
    ParseCSVResult try_parse_csv() const;

  }; // AccountStatementFileState
}