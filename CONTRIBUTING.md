# Contributing

Use the GitHub Actions workflow for builds; this project's current delivery policy forbids local compilation. Work on the session branch, keep each numbered request scoped, and do not claim GPU verification from a CPU-only CI run.

Before pushing: inspect `git diff --check`, validate workflow YAML/JSON, and ensure no build outputs, logs, credentials, model weights or private recordings are staged. After pushing: wait for the Windows build and ctest, inspect failures, and report the artifact and manual checks honestly.

Production paths must handle Windows/HRESULT failures, own resources with RAII, avoid CPU frame readback, and avoid injection/hooks/drivers. Add CPU tests for logic before adding hardware-specific behavior. Document optional dependency licenses and provenance before shipping them.
