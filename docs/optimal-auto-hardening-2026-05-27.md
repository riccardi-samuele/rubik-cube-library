# Auto Optimal Hardening - 2026-05-27

This run validates the selective Auto optimal move-ordering policy after the
LB `8` promotion rule was tightened.

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-hardening
```

Configuration:

- Build preset: `release-native-lto`
- Mode: `optimal`
- Requested profile: `auto`
- Effective profile: `large-local`
- Threads: `16`
- Cache mode: warm
- Max memory: `2048 MB`
- Seeds: `12345`, `20260525`, `42`, `314159`, `271828`,
  `987654321`, `7`, `99`, `123456789`, `424242`, `8675309`, and
  `20240525`
- Cases: two depth-14 cases and one depth-15 case per seed

Result:

- Solved: `36/36`
- Failed: `0`
- Total elapsed: `35395 ms`
- Total nodes expanded: `117274456`
- Slowest case: `random_987654321_1`
- Slowest elapsed: `7539 ms`
- Slowest nodes expanded: `26358286`

Ordering distribution:

| Ordering | Cases |
| --- | ---: |
| `auto_strong_bound` | 21 |
| `base_bound` | 15 |

LB `8` distribution:

| Ordering | LB `8` cases |
| --- | ---: |
| `auto_strong_bound` | 2 |
| `base_bound` | 13 |

Slowest cases:

| Case | Depth | Moves | Initial lower bound | Elapsed ms | Nodes | Ordering |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `random_987654321_1` | 15 | 15 | 8 | 7539 | 26358286 | `base_bound` |
| `random_8675309_1` | 15 | 15 | 9 | 5285 | 18713698 | `auto_strong_bound` |
| `random_12345_1` | 15 | 15 | 9 | 5139 | 17696947 | `auto_strong_bound` |
| `random_42_1` | 15 | 15 | 8 | 3912 | 13268553 | `base_bound` |
| `random_424242_1` | 15 | 15 | 9 | 3063 | 10193888 | `auto_strong_bound` |
| `random_99_1` | 15 | 15 | 8 | 1995 | 6151687 | `base_bound` |

LB `8` audit:

| Case | Elapsed ms | Ordering | Root signal |
| --- | ---: | --- | --- |
| `random_987654321_1` | 7539 | `base_bound` | `first_diff=0`, `strong_min_count=7` |
| `random_42_1` | 3912 | `base_bound` | `first_diff=1`, `strong_min_count=11` |
| `random_99_1` | 1995 | `base_bound` | `first_diff=1`, `strong_min_count=7` |

Additional check:

Seed `42` depth-15 was replayed with strong ordering forced because it is the
slowest newly observed LB `8` base case in this hardening run.

| Variant | Elapsed ms | Nodes | Ordering |
| --- | ---: | ---: | --- |
| Auto policy | 4009 | 13564039 | `base_bound` |
| Forced strong | 6281 | 22139186 | `forced_strong_bound` |

Decision:

Keep the current LB `8` rule unchanged. The hardening run found no failures and
confirmed that the `strong_min_count <= 6` guard avoids at least one additional
regression pattern: seed `42` has `first_diff=1`, but its strong minimum-bound
bucket is too wide and forced strong is slower.

Next useful work:

- Add a repeatable hardening gate for this suite, using a conservative threshold
  derived from the current slowest case.
- Continue discovery for slow LB `8` base cases before expanding the policy.
