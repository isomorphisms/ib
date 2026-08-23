# Product, price, and vector index

IB should treat products and prices as durable browser-owned data rather than as state owned by a merchant renderer.

The merchant is a source, not the object model.

## Product identity

A product has an IB identity and zero or more external keys:

- ISBN
- UPC
- GTIN
- ASIN
- source-specific keys

An ASIN is therefore useful without making Amazon the canonical namespace.

```text
product p-0001
key isbn:978...
key asin:B0...
term category-theory
term paperback
```

`src/IB/Product.idric` contains the first typed representation.

## Price observations

Prices are observations, not properties of a product.

```text
observed 2026-08-23T13:42:00-04:00
product p-0001
source merchant-x
currency USD
minor_units 1799
```

Store money in integer minor units. A price observation belongs in canonical history only when its source permits durable storage.

Provider data with a short legal or technical lifetime is a `CurrentOffer`, not a historical observation. It belongs in disposable cache and carries an expiry time.

This distinction lets IB keep a real price history without pretending that every provider grants a license to archive its API responses.

## Amazon

Amazon is one provider.

The Creators API is useful for live catalog lookup and current offers, but its Program Content has unusually tight use and caching restrictions. The durable model should therefore keep Amazon identifiers such as ASINs while treating Creators API offer data as volatile provider cache.

Do not make the IB canonical price ledger depend on Amazon Creators API response retention.

The Amazon adapter should eventually expose roughly:

```text
search(query) -> product candidates
item(asin) -> current metadata
current_offer(asin) -> expiring offer
outbound_link(asin, partner_tag) -> Amazon link
```

The core does not need to know how Amazon authentication works.

Affiliate identity is distribution configuration, not product identity. A downstream build can supply its own partner tag or no tag. A distribution operated by one Associate can supply that Associate's tag where Amazon permits that distribution surface.

## Vector index

Vector search is a normal rebuildable index.

Canonical product text and identifiers remain inspectable records. Embeddings derived from canonical data live under an index namespace, for example:

```text
state/
  products/
  prices/
  indexes/
    products/
      <embedding-model>/
        vectors
        metadata
```

The first implementation in `src/IB/VectorIndex.idric` uses `Float` vectors and exact dot-product search. Normalize vectors before insertion when cosine-like ranking is desired. Exact scanning is intentionally simple and gives us a correct baseline. If corpus size makes it necessary, the same rebuildable index can later gain an ANN representation such as HNSW without changing canonical product records.

Do not derive the persistent embedding corpus from provider content whose license forbids repurposing, analysis, or long-term storage. Vectorize IB-owned/canonical product text and independently usable metadata instead.

## Browser integration

This follows the existing IB storage rule: canonical state is separate from rebuildable indexes and disposable caches.

A visit to a product page can therefore connect:

```text
visit -> URL -> product identity -> current offers
                         |-> durable permitted price observations
                         |-> rebuildable semantic vector
```

The UI can show a current Amazon price next to a book, tool, replacement part, or other object without making Amazon the place where the user must buy it.
