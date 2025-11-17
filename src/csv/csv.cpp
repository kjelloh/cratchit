#include "csv.hpp"
#include "../logger/log.hpp"

namespace CSV {
  CSV::OptionalFieldRows decoding_in_to_field_rows(text::encoding::DecodingIn& decoding_in) {
    return std::visit(
        [&](auto& is) {
          return CSV::to_field_rows(is, ';');
        },decoding_in);
  }
}
namespace CSV {
  namespace NORDEA {

    // Now in TaggedAmountFramework (upstream projection(s))
    // OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {

  } // namespace NORDEA

  namespace SKV {

      // Now in TaggedAmountFramework (upstream projection(s))
      // OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {

  } // namespace SKV

} // namespace CSV