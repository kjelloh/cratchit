# Consider HAD+HAD multi-post aggregation Ux, mechanism and template candidate inference?

Cratchit may currently create HADs that in fact are part of the same financial event.

* For transfer betwen SKV and bank account
  - One Credit 'from' account
  - One Debit 'to' account
* For recurring payment receipts
  - E.g., Anthropic Claude Code monthly receipt and withdrawal from bank account
  - The user enters a HAD for the receipt as the voucher of the payment
  - The bank account statement results in a HAD for the withdrawal from the account

Can we imagine a Ux where the user can 'assemble' (aggregate) such HADs into one for the finacial event?

* It seems some HADs are currently 'single account post' place holders?
* And what we want is a way to aggregate such account posts into a seed for a complete journal entry?
* We need to promote the aggregated HAD into a 'financial event' HAD?
  - The user should be able to assign the single post HAD to a BAS account and select Debit/Credit

We also want such a multi-post-HAD (financial event HAD) to be clever when inferring template candidates.

* It should find only existing journal entries that posts to the same accounts and with the same Debit/Credit pattern
* It may or may not prefer existing journal entries that share words in the entry heading (caption)
  - Match the company name
  - Match the words 'Faktura', 'Kvitto', 'Utlägg'?