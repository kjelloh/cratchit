#pragma once

// TODO: Refactor away at some future time? /KoH
// 250610: Used by BAS::MetaEntry, BAS::MetaAccountTransaction and BAS::TypedMetaEntry

template <typename Meta,typename Defacto>
class MetaDefacto {
public:
	Meta meta;
	Defacto defacto;
  bool operator==(MetaDefacto const& other) const {
    return (meta == other.meta) and (defacto == other.defacto);
  }
private:
};
