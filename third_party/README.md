# Third-party code

Code here is vendored without changes. Record the exact version of each package.

| Package | Version | Purpose | Source |
|---|---|---|---|
| Unity | v2.7.0 (2026-07-16) | unit test framework | https://github.com/ThrowTheSwitch/Unity |
| CMock | v2.7.0 (2026-07-16) | mock generation from headers (needs Ruby) | https://github.com/ThrowTheSwitch/CMock |

Layout: `third_party/unity/`, `third_party/cmock/`.

## How they were vendored (TASK-003)

Each directory is the complete GitHub source archive of the release tag, extracted with the
top-level directory stripped, with no file added, removed or changed:

| Archive | SHA-256 |
|---|---|
| `https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.7.0.tar.gz` | `e84eb301ca7967831e68b1728f911e87fa2d345d8ddb64f897bc2f2ee24a321c` |
| `https://github.com/ThrowTheSwitch/CMock/archive/refs/tags/v2.7.0.tar.gz` | `d96282cf0286682f7628afc31cf2e3ed6ecb66944d63e098824d98196904f04c` |

```sh
curl -L -o Unity-v2.7.0.tar.gz https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.7.0.tar.gz
curl -L -o CMock-v2.7.0.tar.gz https://github.com/ThrowTheSwitch/CMock/archive/refs/tags/v2.7.0.tar.gz
sha256sum Unity-v2.7.0.tar.gz CMock-v2.7.0.tar.gz
tar xzf Unity-v2.7.0.tar.gz -C third_party/unity --strip-components=1
tar xzf CMock-v2.7.0.tar.gz -C third_party/cmock --strip-components=1
```

Notes:

- CMock v2.7.0 pins Unity v2.7.0 as its `vendor/unity` submodule. GitHub archives do not
  contain submodules, so `cmock/vendor/unity/` and `cmock/vendor/c_exception/` are empty and
  git does not store them. The build uses `third_party/unity/` instead
  (`cmake/xat_unit_test.cmake`).
- Unity's own `.gitignore` ignores `Makefile`. The three Makefiles of the archive
  (`test/Makefile`, `extras/fixture/test/Makefile`, `extras/memory/test/Makefile`) are
  added with `git add -f`, so the tree stays identical to the archive.
- To check that the tree is pristine, extract the archive elsewhere and run
  `diff -r <extracted>/Unity-2.7.0 third_party/unity` (and the same for CMock). The only
  difference allowed is the two empty CMock `vendor/` directories.
- CMock and the Unity runner generator need only Ruby's standard library (`yaml`,
  `fileutils`, `erb`); Ruby is installed as in `docs/setup_host.md`.
