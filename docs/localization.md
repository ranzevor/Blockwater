# Localization

[简体中文](zh-CN/localization.md)

- `locales/en-US.lang` is the required English fallback catalog.
- `locales/zh-CN.lang` contains Simplified Chinese translations with identical message keys.
- Each UTF-8 line is `stable.message.key = user-visible text`. Empty lines and lines beginning with `#` are ignored. Keys use lowercase dot-separated identifiers; duplicate or empty messages are invalid.
- `Catalog(directory, requested_locale)` loads the fallback and selected catalogs once during startup. `zh` and `zh-CN` select Simplified Chinese; all other tags currently use `en-US`. Missing translated keys fall back to English. The library does not read files on the game frame loop.
- Pass untranslated event identifiers and numeric data across IPC. Localize messages only at the display boundary; never make a protocol consumer parse translated text.

When editing language resources, update both catalogs and run `python3 tools/validate_repository.py` and the C++ tests. The validator checks UTF-8 validity, keys and translation parity. The core catalog is functional, but no in-game language selector is implemented yet.
