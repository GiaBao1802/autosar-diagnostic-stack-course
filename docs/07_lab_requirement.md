# Lab specification và traceability

## Synthetic coding data

| Byte | Field | Valid values |
|---:|---|---|
| 0 | driver side | 0 left, 1 right |
| 1 | powertrain | 0 ICE, 1 hybrid, 2 electric |
| 2 | transmission | 0 manual, 1 automatic |

## Requirements

| ID | Requirement | Test |
|---|---|---|
| LAB-10 | support default/extended session | session positive/invalid subfunction |
| LAB-22 | read F1A0 in default/extended | read positive/unknown DID |
| LAB-2E | write only extended + unlocked | access matrix |
| LAB-27 | seed before key; invalid key rejected | sequence/key tests |
| LAB-31 | ApplyCoding routine only after valid coding | start/result tests |
| LAB-TP | segment/reassemble payload > SF capacity | SF/FF/CF/SN tests |
| LAB-NVM | successful write becomes persistent after main function | reset/readback test |

## Definition of done

All positive/NRC paths tested; no proprietary identifier; sanitizer clean; request/response examples documented; configuration table contains access policy rather than hard-coded scattered checks; build uses same CI as course repo.
