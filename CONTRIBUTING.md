# Contributing to EmbedPluginUI

Thank you for considering a contribution.

EmbedPluginUI is a **source-available dual-licensed project**. The public repository uses the non-commercial license in `LICENSE`, while the project maintainer may also offer separate commercial licenses. Because of that model, contributions need an explicit inbound license that preserves the project's ability to distribute the combined work under both licensing paths.

## Before opening a pull request

- Keep portable framework code under `include/epui` and `src` free of unnecessary OS/vendor SDK dependencies.
- Prefer fixed-capacity or application-owned storage over hidden heap allocation.
- Keep public headers self-contained.
- Add or update tests for behavior changes.
- Update the English/Chinese Wiki source under `docs/wiki/` when public APIs or user-facing behavior changes.
- Run:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Contribution license

By intentionally submitting a contribution to this repository (for example through a pull request), You represent that:

1. You have the legal right to submit the contribution and to grant the permissions below;
2. the contribution does not knowingly include third-party material that You are not permitted to contribute on these terms; and
3. You identify any third-party code, assets, or license obligations included in the contribution.

You grant the project maintainer and copyright holder(s) of EmbedPluginUI a perpetual, worldwide, non-exclusive, irrevocable, royalty-free, transferable and sublicensable copyright license to use, reproduce, modify, prepare derivative works of, publicly display, publicly perform, distribute, and otherwise exploit Your contribution, and to license or relicense Your contribution as part of EmbedPluginUI under:

- the project's public non-commercial/source-available license;
- separate commercial licenses offered by the project maintainer; and
- future versions or replacements of those licensing terms that preserve the project's source-available / commercial dual-license model.

You retain copyright in Your contribution unless You separately agree to an assignment.

If You cannot or do not wish to grant these permissions, please do not submit the contribution for inclusion in the main project; opening an issue or discussion without contributing code remains welcome.

## Developer Certificate statement

By submitting a pull request, You also certify that the contribution is Your original work or that You otherwise have sufficient rights to submit it, and that You understand the contribution will be public and may be redistributed according to the contribution license above.

## Style and architecture

Core rules:

```text
Application
    |
Page / Widget / Overlay plugins
    |
Ui + animation
    |
Canvas
    |
Display/controller
    |
Transport / platform adapter
```

Please avoid moving vendor APIs upward into Page/Widget code. Add new boards and buses through adapters/plugins wherever possible.

## Documentation language

The Wiki source is bilingual. Each user-facing page under `docs/wiki/` contains both `中文` and `English` sections. Public behavior changes should update both sections in the same pull request.

## Licensing questions

For contributor or commercial licensing questions:

```text
alaskachinese@outlook.com
```

> This contribution policy is intended to document the project's inbound licensing expectations. Contributors with legal or employer-specific concerns should obtain their own legal advice before contributing.
