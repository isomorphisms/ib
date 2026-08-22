# ib

An experimental browser built around persistent browsing state rather than a renderer-owned tab model.

The browser core should own navigation history, sleeping/waking, snapshots, organization, and indexes. Rendering engines are adapters that can be replaced without changing the stored browsing model.

Initial design work lives on development branches.
