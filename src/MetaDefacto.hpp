#pragma once
#include <memory> // std::unique_ptr

// TODO: Refactor away at some future time? /KoH
// 250610: Used by BAS::MetaEntry, BAS::MetaAccountTransaction and BAS::TypedMetaEntry

namespace detail {
  template <typename T>
  struct raw_type { using type = T; };

  template <typename T>
  struct raw_type<std::unique_ptr<T>> { using type = T; };
}

template <typename Meta_,typename Defacto_>
class MetaDefacto {
public:
  using Meta = Meta_;
  using Defacto = Defacto_;
  using defacto_value_type = detail::raw_type<Defacto>::type;
	Meta meta;
	Defacto defacto;
  bool operator==(MetaDefacto const& other) const {
    return (meta == other.meta) and (defacto == other.defacto);
  }
private:
};
