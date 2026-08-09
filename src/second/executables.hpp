#pragma once

#include <variant>


namespace tea {

  // #TEA::Cmd
  enum class CmdResponseType {
    Unknown
    ,ProgressReport
    ,Done
    ,Undefined
  }; // Type

  class NoCmd {
  public:
    // make us work as 'key' (comparable)
    auto operator<=>(NoCmd const&) const = default;
  }; // NoCmd

  class TestCmdDescriptor {
  public:
    // make us work as 'key' (comparable)
    auto operator<=>(TestCmdDescriptor const&) const = default;
    size_t arg;
    struct payload_type  {
      CmdResponseType response_type;
      size_t progress_ix;
    }; // result_type
  private:
  };

  using Executable = std::variant<
     NoCmd
    ,TestCmdDescriptor
  >;

} // tea