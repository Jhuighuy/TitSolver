# `tit/prop`

General-purpose property management library.

## Architecture

`tit::prop` is built around three public concepts:

- `Spec` trees define the configuration schema.
- `Tree` stores the validated runtime configuration values.
- `App` and `TIT_IMPLEMENT_APP(...)` bind a schema to a command-line binary.

The command-line parser itself is intentionally an implementation detail behind
`TIT_IMPLEMENT_APP(...)`, rather than a separate supported public API.

## Data Flow

The library follows this flow:

1. Raw input is loaded from CLI arguments and optional config files.
2. The schema validates and normalizes the data into a `Tree`.
3. Applications consume only the final validated `Tree`.

`Tree` is meant to stay a simple value object. It does not track whether values
were provided by the user or filled in from defaults.

## Outputs

The schema can be rendered in three ways:

- human CLI help
- machine-readable JSON help for frontend integration
- YAML config templates

The outputs have different intended roles:

- CLI help is short and focused on top-level scalar flags and common service
  options.
- JSON help is the machine-facing schema export used by external tools.
- YAML config templates are the full human-facing configuration artifact.

The JSON help output is the intended machine-facing contract for external tools
such as the frontend and future documentation generation.
