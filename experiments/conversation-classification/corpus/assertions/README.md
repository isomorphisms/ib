# Assertion events

Labels are append-only JSON Lines events.  An event identifies a corpus object,
one axis (`existing_location`, `concept`, or `filing_destination`), a category
value, positive or explicit-negative polarity, provenance, authority, time,
source reference, and reason.  Absence is unlabeled, never negative.

The reducer selects the highest-authority, then latest, event for each
conversation/category pair.  A correction therefore changes the materialized
view without editing the conversation, earlier evidence, or classifier output.
The `correct`, `evidence`, and `project` commands exercise this behavior.
