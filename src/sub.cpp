#include "sub.hpp"

namespace first {

  // ----------------------------------
  std::optional<Msg> onKey(Event event) {
    if (event.contains("Key")) {
      return Msg{std::make_shared<NCursesKeyMsg>(std::stoi(event["Key"]))};
    }
    return std::nullopt;
  }

} // namespace first