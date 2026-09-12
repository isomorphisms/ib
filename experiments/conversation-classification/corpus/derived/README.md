# Derived generations

Vectors, fitted encoders, affine planes, thresholds, predictions, explanations,
and materialized projections are rebuildable generations.  Each fitted model
has a content-derived generation id and hashes for its JSON, NumPy, and trusted
local sklearn artifacts.  A proposal records both that model generation and the
hash of the screened source conversation it classified.

`train-model` changes a model generation and writes no proposals.
`classify-model` never fits.  With `--only`, it parses only named conversation
bodies, validates unchanged rows through the cheap corpus index, and reuses
compatible proposals.  Loading `encoder.joblib` is confined to trusted local
artifacts; it is not a safe or canonical interchange format.
