# Notes

This prototype is intentionally stupid.

It exists to answer four questions before ib chooses its implementation language or real renderer:

1. Can first paint be produced from browser-owned bytes without starting a full renderer?
2. Can the first paint stay small enough to cache for many sleeping tabs?
3. Can previews be deleted and regenerated without changing durable browsing state?
4. Can the UI progressively ask for metadata, text, one visual screen, more visual screens, and finally a live renderer instead of loading everything at once?

The answer should remain yes even if this Python prototype is thrown away completely.
