#pragma once

#include "StateImpl.hpp"
#include "csv/csv.hpp"
#include "text/encoding.hpp"
#include <string>
#include <filesystem>
#include <optional>

namespace first {
  class CSVFileState : public StateImpl {
  private:
    std::filesystem::path m_file_path;
    mutable std::optional<CSV::OptionalTable> m_cached_table;
    mutable std::optional<std::string> m_cached_encoding;

  public:
    CSVFileState(const CSVFileState&) = delete;
    CSVFileState(std::filesystem::path file_path);
    CSVFileState(std::optional<std::string> caption, std::filesystem::path file_path);

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    virtual std::string caption() const override;

    const std::filesystem::path& file_path() const { return m_file_path; }

  private:
    std::string detect_encoding() const;
    CSV::OptionalTable parse_csv_content() const;

  }; // CSVFileState
}