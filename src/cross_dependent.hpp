#pragma once

#include <memory> // std::shared_ptr
#include <functional> // std::function

namespace first {
  // ----------------------------------
  struct StateImpl; // Forward

  // ----------------------------------
  using State = std::shared_ptr<StateImpl>;

  // ----------------------------------
  using StateFactory = std::function<State()>;
  
  // ----------------------------------
  struct MsgImpl {
    virtual ~MsgImpl() = default;
  };

  // ----------------------------------
  using Msg = std::shared_ptr<MsgImpl>;

  // Cmd: () -> std::optional<Msg>
  // ----------------------------------
  using Cmd = std::function<std::optional<Msg>()>;

} // namespace first