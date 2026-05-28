# V4 Post-Release Verification - 2026-05-28

This document records the post-release smoke test for the published `v4.0.0`
GitHub Release.

## Release

- Tag: `v4.0.0`
- Release URL: `https://github.com/riccardi-samuele/rubik-cube-library/releases/tag/v4.0.0`
- Assets:
  - `rubik_cube_library-4.0.0.tar.gz`
  - `rubik_cube_library-4.0.0.tar.gz.sha256`

## Verification

Commands run from a temporary directory:

```bash
gh release download v4.0.0 --pattern 'rubik_cube_library-4.0.0.tar.gz*'
sha256sum -c rubik_cube_library-4.0.0.tar.gz.sha256
tar -xzf rubik_cube_library-4.0.0.tar.gz
cmake -S rubik_cube_library-4.0.0 -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target rubik_tests rubik-solve rubik-bench
ctest --test-dir build -R "rubik_tests|cli_solve_version|cli_bench_version" --output-on-failure
```

Result:

- checksum: passed
- archive configure: passed
- archive build: passed
- selected archive tests: `3/3` passed

This verification uses the published GitHub Release assets, not the local source
tree.
