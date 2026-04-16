# Consider a mechanism to administrate current BAS accounts (plan)?

I discovered that Cratchit does not have a way to add BAS accounts to the ones listed in the imported SIE Archive.

I created a journal entry that posts to BAS::6150 'Skulder till närstående personer,kortfristig del'.

* Cratchit allowed me to post to account 6150.
* But the account did not show any heading
  - It was not listed in imported SIE file

```text
...
#KONTO 6100 "Kontorsmateriel och trycksaker (gruppkonto)"
#SRU 6100 7513
#KONTO 6110 "Kontorsmateriel"
#SRU 6110 7513
...
```

Where is Cratchit looking up the name of an account?

DARN! It is in fact a global storage BAS::detail::global_account_metas!


* But It may accessed though SIEDocument member account_metas.

```c++
BAS::AccountMetas const&  account_metas() const;
```

  - SUPER CONFUSING!

* There is a stream operator

```c++
std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountPosting const& ap) {
	if (BAS::global_account_metas().contains(ap.account_no)) os << std::quoted(BAS::global_account_metas().at(ap.account_no).name) << ":";
  else os << std::quoted("??namn??") << ":";
	os << ap.account_no;
	os << " " << ap.transtext;
	os << " " << to_string(ap.amount);
	return os;
};
```

* There is a set_account_name:

```c++
void SIEDocument::set_account_name(BAS::AccountNo bas_account_no ,std::string const& name) {
  if (BAS::detail::global_account_metas.contains(bas_account_no)) {
    if (BAS::detail::global_account_metas[bas_account_no].name != name) {
      logger::cout_proxy << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
    }
  }
  BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
}
```

  - It is used by the SIE document parser

```c++
std::optional<SIEDocument> sie_from_utf8_sv(std::string_view utf8_sv) {
    // ...
    else if (std::holds_alternative<sie::io::Konto>(entry)) {
      auto& konto = std::get<sie::io::Konto>(entry);
      sie_doc.set_account_name(konto.account_no, konto.name);
    }
    // ...
```