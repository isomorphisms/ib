# arXiv 0.2 deterministic fixture

`paper.pdf` is a small but valid PDF used to prove the Cauldron -> `pdftotext` -> Pensieve path. Its expected extracted text is recorded in `pdf-extraction-expected.txt`.

The fixture is synthetic; it is not a copy of the real arXiv paper. The arXiv identifier used by the fake ICU transport is only the deterministic routing key for this test.
