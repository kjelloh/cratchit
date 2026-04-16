# Consider to fix Unknown JournalEntryVATType blocking journal entry stage?

I tried to define a journal entry where cost and shipping aws separated.

* But Cratchit refused due to failure to recognise 'JournalEntryVATType'?

```text
"gross" count:1
"vat" count:1
Failed to recognise the VAT type
DESIGN INSUFFICIENCY - Unknown JournalEntryVATType Undefined
  _  "Account:NORDEA::Anonymous Text:Kortköp 260330 SP LAMYSHOP | SEK" 20260331
         gross = sort_code: 0x3 : "PlusGiro":1920  -157.00
         vat = sort_code: 0x6 : "Debiterad ingående moms":2641  31.00
         ? = sort_code: 0x0 : "Kontorsmateriel":6110  118.00
         ? = sort_code: 0x0 : "Kontorsmateriel":6110 "Frakt" 39.00
SORRY - Failed to stage entry
```

This is not ok. I beleive this journal entry is perfectly valid.

I wonder what my idea was about having to identify a valid JournalEntryVATType?

* Can I let this through if the entry balance?
* Or can I fix so we can identify a valid JournalEntryVATType?
  - I beleive the problem is that I mix the price and shipping in the same entry.
  - And these have diufferent VAT rates
  - So the total VAT is not a solid 6,12,25%?