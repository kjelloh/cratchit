#pragma once

#include "StateImpl.hpp"
#include "Mod10View.hpp"
#include <vector>
#include <filesystem>

namespace first {
  class AccountStatementFilesState : public StateImpl {
  public:
    using FilePaths = std::vector<std::filesystem::path>;

    AccountStatementFilesState();
    AccountStatementFilesState(FilePaths file_paths, Mod10View mod10_view);

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    virtual std::string caption() const override;

  private:
    FilePaths m_file_paths;
    Mod10View m_mod10_view;

    FilePaths scan_from_bank_or_skv_directory() const;

  }; // AccountStatementFilesState
}