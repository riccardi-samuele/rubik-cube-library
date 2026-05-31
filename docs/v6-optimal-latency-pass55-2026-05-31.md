# V6 optimal latency pass 55

## Goal

Close out the `phase2_tiebreak` conservative-root ordering candidate after the
three dense `lb8` bucket replays from pass52 through pass54.

This pass does not introduce a new solver policy. It aggregates the measured
bucket results and records the decision so future work does not keep retesting a
candidate that failed to generalize.

## Inputs

| Pass | Bucket | Cases | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| pass52 | `lb8_s5-8_fd1` | 6 | 4,303 | 4,282 | -21 | -0.49% | 23,801,758 | 23,648,352 | -153,406 | candidate |
| pass53 | `lb8_s9-12_fd1` | 4 | 7,778 | 7,836 | 58 | 0.75% | 44,420,899 | 44,609,202 | 188,303 | baseline |
| pass54 | `lb8_s13-16_fd1` | 4 | 9,938 | 9,976 | 38 | 0.38% | 42,475,751 | 42,486,041 | 10,290 | baseline |

## Aggregate Result

Across the three dense `lb8` bucket replays:

| Cases | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 14 | 22,019 | 22,094 | 75 | 0.34% | 110,698,408 | 110,743,595 | 45,187 |

At case level, the candidate split evenly:

| Candidate wins | Candidate losses |
| ---: | ---: |
| 7 | 7 |

The aggregate is still negative because the candidate lost on the heavier
cases. The only positive bucket, `lb8_s5-8_fd1`, was small and weak compared to
the two later negative buckets.

## Decision

Do not promote `phase2_tiebreak` into solver behavior.

Do not run additional broad `lb8` replays for this candidate unless a new
discriminator is found first. More bucket replay without a stronger selector
would mostly add noise around a candidate that already lost the aggregate.

## Next Direction

Shift away from this root ordering tweak. The next useful optimization step is
to mine conservative-root cases for a stronger signal before changing the
solver. Good candidates are:

- root order position of the eventual solution move
- whether the slow case is dominated by one or two heavy root children
- profiles where baseline and strong-bound first moves differ but the solution
  rank is high
- cases where root workers leave expensive children unvisited after a found
  solution

The next implementation-oriented step should produce a data-mining report over
the existing pass50/pass52-pass54 artifacts before proposing another solver
policy.
