# arXiv 0.2 deterministic fixture

`paper-valid.pdf.b64` contains a small valid PDF used to prove the Cauldron -> `pdftotext` -> Pensieve path. `tests/fake_icu_0_2` decodes it when the deterministic arXiv PDF URL is requested, so the Cauldron still receives ordinary PDF bytes. Its expected extracted text is recorded in `pdf-extraction-expected.txt`.

The older `paper.pdf` is the original intentionally non-parseable boundary fixture and is no longer used by the deterministic round-trip fetch.

The extractable fixture is synthetic; it is not a copy of the real arXiv paper. The arXiv identifier used by the fake ICU transport is only the deterministic routing key for this test.
