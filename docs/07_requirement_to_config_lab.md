# Requirement → configuration → generated artifacts và lab

Workspace cục bộ cho thấy workflow artifact điển hình:

```mermaid
flowchart LR
  RE["Diagnostic/CanTp requirement"] --> SHEET["Diagnostic spreadsheet/config input"]
  SHEET --> ECUC["Dcm/CanTp ECUC ARXML"]
  ECUC --> GEN["Generator"]
  GEN --> DCMCFG["Dcm_Cfg/Lcfg/PBcfg"]
  GEN --> TPCFG["CanTp_Cfg/Lcfg/PBcfg"]
  GEN --> ROUTE["PduR_Dcm / PduR_CanTp"]
  GEN --> RTE["RTE/SchM integration"]
  MAN["Manual callback/application"] --> BUILD["Build + unit/integration test"]
  DCMCFG --> BUILD
  TPCFG --> BUILD
  ROUTE --> BUILD
  RTE --> BUILD
```

## Synthetic requirement DIAG-COD-001

> ECU shall provide DID `0xF1A0` ConfigurationProfile. Read is allowed in default and extended sessions. Write is allowed only in extended session after security level 1. Data length is 3 bytes: drive-side, powertrain-type, transmission-type. Values outside configured ranges shall be rejected. A successful write shall request persistent storage and update runtime signals after validation.

Requirement này mô phỏng loại variant-coding workflow mà không sử dụng DID/data/symbol thật.

## Breakdown

| Concern | Config/manual artifact |
|---|---|
| DID ID/length | DCM DID/data config |
| read/write ports | callback/RTE client-server config |
| session access | service/DID access config |
| security | DCM security row/level |
| validation | manual callback/domain function |
| persistence | NvM block/config + async job |
| positive/NRC | DCM DSP result mapping |
| test | request matrix session/security/value |

Không sửa generated file để “add DID”. Sửa source configuration/model, regenerate, review diff, implement manual callback và run regression. Generated diff thường lan qua identifiers, tables, access rows, service references và RTE symbols.

## Test sequence

```text
10 03                 enter extended session
27 01                 request seed
27 02 <derived key>   unlock level 1
2E F1 A0 01 02 01     write coding
22 F1 A0              read back
11 01 / simulated reset
22 F1 A0              verify persisted value
```

Production security algorithm/key handling không được public hoặc thay bằng toy XOR.

## Configuration walkthrough

### Read DID/VIN-like data

Tạo DID → DidInfo/access → DidSignal → Data; cấu hình size/type/endianness/read port; map callback; cho phép đúng session/security; kiểm tra SID `0x22`, buffer và CanTp/PduR multi-frame response path.

### Write variant DID

Định nghĩa coding layout/range → session/security/mode rule → condition check → write callback → RAM/NvM atomicity → apply ngay/routine/reset → read-back và power-cycle test.

### Hard reset

Enable SID/subfunction → access/condition → BswM/EcuM/Mcu reset preparation → NvM policy → chỉ reset sau positive response theo requirement.

## Traceability và definition of done

| Requirement | Design/config | Implementation | Test evidence |
|---|---|---|---|
| Read coding DID | DID/Data/read access | read callback | positive, wrong DID/session |
| Write coding DID | write access/NvM | validate/write callback | range, security, persistence |
| Apply coding | RID/start access | routine state machine | pending/result/reset |
| Transport | CanIf/CanTp/PduR/DCM refs | generated config | SF/multi-frame/timeouts |

Lab hoàn tất khi requirement có unique ID, mỗi rule có test, generated diff được review, callback có unit test, transport có negative test và coding sống qua power cycle.

## Generated artifacts cần review

`Dcm_Cfg/Lcfg/PBcfg`, `CanTp_*Cfg`, `PduR_*`, RTE/callback headers, NvM block config và SchM/OS cyclic schedule. Vendor có thể đổi tên file; review theo responsibility và symbolic reference, không học thuộc line number.
