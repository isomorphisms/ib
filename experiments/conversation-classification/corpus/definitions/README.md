# Category and projection definitions

Concept categories and filing destinations are persistent definitions, not
names inferred from a fitted model.  Each category definition must state its
axis, meaning, inclusion and exclusion criteria, adjacent categories, version,
and creation provenance.  The full category inventory must be induced and
reviewed from the acquired corpus; no title-only category list is promoted here.

`category.schema.json` describes concept and filing-destination definitions.
`filing-policy.schema.json` describes a different object: the deterministic
projection from accepted conceptual memberships and weak existing-location
evidence to one preferred ChatGPT location.  A filing policy names destinations
explicitly and gives positive evidence weights for concepts and, optionally,
small weights for inherited locations.  The projection can abstain or return
several plausible destinations.  It must not silently equate a concept name
with a ChatGPT folder/project name.

Classifier margins are retained as explanation evidence but are not summed
across independently calibrated category planes.  Filing-policy support scores
come only from the versioned policy weights and are explicitly not calibrated
probabilities.  High-authority filing corrections override policy output
without rewriting the concept proposal that led to it; explicit negative filing
corrections prevent that destination from being proposed again until superseded.

Changing a category definition creates a new definition version.  Feature
vectors remain reusable when their input grammar is unchanged; only affected
category models need retraining.  Changing filing policy invalidates filing
projections, not conversation parsing, concept features, or fitted concept
planes.
