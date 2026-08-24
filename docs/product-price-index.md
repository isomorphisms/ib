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

## Price retrieval

For price data, prefer the ordinary web path over a restricted merchant API: ICU fetches the page and Grease handles the retrieval/extraction workflow. If the useful price value is easy to isolate, a small grep-like extraction is enough. If the page needs structured parsing, feed the ICU-fetched text to a small Idriç parser rather than moving browser policy into shell code.

This rule is specifically about price data. Product metadata, identifiers, outbound links, and other provider integrations may use different adapters when appropriate.

Fetched page bodies are not the durable price ledger. Reduce them to the minimal price fact needed by the core, then apply the existing storage boundary: a permitted durable fact becomes a `PriceObservation`; data that must remain short-lived stays a `CurrentOffer` in disposable cache.

## Amazon

Amazon is one provider.

Do not make the IB price path depend on Amazon Creators API offer retention. For current Amazon prices, use the ICU + Grease retrieval path above and retain only the minimal parsed price fact according to the source's storage policy.

Amazon identifiers such as ASINs remain useful durable product keys. An Amazon adapter can still provide non-price catalog metadata or outbound-link behavior without making Amazon the canonical product namespace.

The Amazon adapter should eventually expose roughly:

```text
search(query) -> product candidates
item(asin) -> current metadata
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

The intended scalar for this index is 32-bit `Float`. The Idriç compiler revision currently pinned by IB exposes `Double` but no primitive `Float`, so the first executable baseline uses `Double` inside the rebuildable index. This is deliberately not canonical state: when Idriç gains `Float`, the index can be rebuilt with 32-bit vectors without migrating product or price records.

The baseline uses exact dot-product search. Normalize vectors before insertion when cosine-like ranking is desired. Exact scanning is intentionally simple and gives us a correct reference implementation. If corpus size makes it necessary, the same rebuildable index can later gain an ANN representation such as HNSW without changing canonical product records.

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