#include "core/ar/disposition.hpp"

namespace cratchit::ar {

Disposition build_disposition(Statement const& statement) {
  Disposition disposition;
  auto const& lines = statement.lines;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    auto const& heading = lines[index];
    if (heading.in_notes || heading.kind != LineKind::heading || heading.text != "Fritt eget kapital") {
      continue;
    }
    for (auto at = index + 1; at < lines.size() && lines[at].kind == LineKind::item; ++at) {
      if (lines[at].shown) {
        disposition.funds.push_back({lines[at].text, lines[at].exact.front()});
        disposition.total += lines[at].exact.front();
      }
    }
    break;
  }
  return disposition;
}

} // namespace cratchit::ar
