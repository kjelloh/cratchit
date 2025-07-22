#pragma once

#include "StateImpl.hpp"
#include <string>
#include <filesystem>

namespace first {
  class CSVFileState : public StateImpl {
  private:
    std::filesystem::path m_file_path;

  public:
    CSVFileState(const CSVFileState&) = delete;
    CSVFileState(std::filesystem::path file_path);
    CSVFileState(std::optional<std::string> caption, std::filesystem::path file_path);

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;

    const std::filesystem::path& file_path() const { return m_file_path; }

  }; // CSVFileState
}