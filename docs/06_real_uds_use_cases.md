# UDS use cases thực tế — từ tester tới application

Chương này dùng CAN ID giả lập `0x7E0` (tester→ECU) và `0x7E8` (ECU→tester). DID/RID variant là dữ liệu học tập, không phải cấu hình OEM. Mỗi flow tách ba lớp:

```mermaid
flowchart LR
  A["1. UDS semantics<br/>SID, subfunction, DID/RID, NRC"] --> B["2. Transport<br/>SF/FF/FC/CF, timers"]
  B --> C["3. ECU integration<br/>DCM, DEM, NvM, BswM, application"]
```

## 0. Luồng chung của một request

```mermaid
sequenceDiagram
  autonumber
  participant T as Tester
  participant CI as CanIf
  participant TP as CanTp
  participant PR as PduR
  participant DSL as DCM DSL
  participant DSD as DCM DSD
  participant DSP as DCM DSP
  participant APP as Application/BSW
  T->>CI: CAN L-PDU
  CI->>TP: CanTp_RxIndication(RxPduId, PduInfo)
  TP->>PR: StartOfReception + CopyRxData
  PR->>DSL: DCM TP buffer callbacks
  TP->>PR: TpRxIndication(E_OK)
  DSL->>DSD: complete request + connection context
  DSD->>DSD: check SID, session, security, mode
  DSD->>DSP: dispatch supported service
  DSP->>APP: DID/RID/service callback
  APP-->>DSP: data/status/NRC/pending
  DSP-->>DSL: assembled response
  DSL->>PR: PduR_DcmTransmit
  PR->>TP: CanTp_Transmit
  TP-->>T: SF or FF/CF response
```

Nếu CanTp chưa hoàn thành N-SDU, DCM chưa nhìn thấy SID. Vì vậy mất CF/timer transport thường không tạo UDS NRC; tester chỉ thấy timeout.

---

## 1. Đọc VIN bằng `22 F1 90`

### Mục đích

VIN là chuỗi 17 ký tự dùng nhận diện xe. Tester workshop, manufacturing hoặc service tool thường đọc VIN để chọn dataset/variant và gắn log với đúng xe.

### Request UDS

```text
22 F1 90
│  └──┴── DID F190 = VIN
└──────── SID ReadDataByIdentifier
```

Request 3 byte vừa một Single Frame:

```text
CAN 0x7E0 DLC=8: 03 22 F1 90 00 00 00 00
                   │  └──── payload ────┘
                   └ length = 3
```

### Response multi-frame

Positive UDS payload dài `1 + 2 + 17 = 20` byte:

```text
62 F1 90 54 52 41 49 4E 49 4E 47 56 49 4E 30 30 30 30 30 31
│  │  │  └──────────────── "TRAININGVIN000001" ──────────────
│  └──┴── DID echo
└──────── positive SID = 0x22 + 0x40
```

Classic CAN ISO-TP example:

```mermaid
sequenceDiagram
  participant ECU as ECU / 0x7E8
  participant T as Tester / 0x7E0
  ECU->>T: FF: 10 14 62 F1 90 54 52 41
  Note over T: total UDS length = 0x14 = 20
  T-->>ECU: FC CTS: 30 00 00 00 00 00 00 00
  Note over ECU: BS=0, STmin=0
  ECU->>T: CF1: 21 49 4E 49 4E 47 56 49
  ECU->>T: CF2: 22 4E 30 30 30 30 30 31
```

`FF` chứa 6 byte đầu; CF1/CF2 mang phần còn lại. PCI/length không thuộc UDS payload và không được đưa vào DID callback.

### Flow bên trong DCM

```mermaid
flowchart TD
  R["Request 22 F190"] --> S{"0x22 supported<br/>in current service table?"}
  S -->|No| N11["7F 22 11"]
  S -->|Yes| D{"DID F190 configured?"}
  D -->|No| N31["7F 22 31"]
  D -->|Yes| A{"Session/security/<br/>mode allowed?"}
  A -->|No| NEG["Configured NRC"]
  A -->|Yes| C["ConditionCheckRead callback"]
  C -->|Fail| N22["7F 22 22"]
  C -->|OK| READ["ReadData callback: 17 bytes"]
  READ --> RESP["DCM builds 62 F190 + data"]
  RESP --> TP["CanTp segments to FF/CF"]
```

### Configuration cần có

- DSD service entry `0x22` reachable từ protocol row.
- `DcmDspDid` identifier F190, read access, fixed length.
- `DcmDspData` size 17 byte, read callback/RTE operation.
- Default/extended session references và security policy.
- DCM response buffer tối thiểu 20 byte.
- DCM TxPdu → PduR → CanTp TxNSdu → CanIf TxPdu route.

### Test matrix

| Case | Request/condition | Expected |
|---|---|---|
| positive | `22 F1 90` | exactly 17 VIN ASCII bytes |
| wrong length | `22 F1` | `7F 22 13` |
| unknown DID | `22 F1 91` | `7F 22 31` |
| callback unavailable | data source invalid | `7F 22 22` or configured behavior |
| no FC after FF | tester transport fault | ECU abort after N_Bs; no complete response |
| wrong CF sequence | transport corruption | receiver rejects; DCM payload not delivered |

---

## 2. Write variant DID `2E F1 A0`

### Synthetic requirement

Ba byte coding: driver side `0..1`, powertrain `0..2`, transmission `0..1`. Write chỉ ở extended session, security level 1 và safe vehicle condition. Dữ liệu phải persist qua reset.

```mermaid
flowchart LR
  REQ["2E F1A0 + 3 bytes"] --> G1["Session gate"] --> G2["Security gate"]
  G2 --> G3["Vehicle/mode condition"] --> VAL["Field + dependency validation"]
  VAL --> RAM["Update RAM mirror"] --> NVM["NvM write"] --> APPLY["Publish/apply coding"]
```

### End-to-end sequence

```mermaid
sequenceDiagram
  autonumber
  participant T as Tester
  participant D as DCM
  participant SEC as Security callback
  participant APP as Coding component
  participant N as NvM
  participant RTE as Runtime consumers
  T->>D: 10 03
  D-->>T: 50 03 P2 P2*
  T->>D: 27 01
  D->>SEC: GetSeed(level 1)
  SEC-->>D: seed
  D-->>T: 67 01 + seed
  T->>D: 27 02 + key
  D->>SEC: CompareKey
  SEC-->>D: valid
  D-->>T: 67 02
  T->>D: 2E F1 A0 01 02 01
  D->>APP: ConditionCheckWrite + WriteData
  APP->>APP: range/dependency validation
  APP->>N: NvM_WriteBlock
  alt asynchronous persistence
    APP-->>D: DCM_E_PENDING
    D-->>T: 7F 2E 78 when P2 expires
    D->>APP: poll with OpStatus PENDING
    N-->>APP: job finished
  end
  APP->>RTE: publish validated coding
  APP-->>D: E_OK
  D-->>T: 6E F1 A0
```

### RAM, NvM và positive response

```mermaid
stateDiagram-v2
  [*] --> OldCoding
  OldCoding --> Validating: WriteData request
  Validating --> OldCoding: invalid → NRC
  Validating --> RamUpdated: valid
  RamUpdated --> NvPending: request NvM
  NvPending --> Persisted: job OK
  NvPending --> RollbackOrFault: job failed
  Persisted --> Applied: publish now/apply routine/reset policy
```

Không có một rule universal rằng DCM phải đợi NvM. Requirement quyết định completion semantics. Nếu positive response hứa “stored successfully”, callback phải chỉ trả `E_OK` sau khi lower job complete hoặc có transaction design tương đương.

### NRC decision tree

```mermaid
flowchart TD
  W["2E F1A0 data"] --> L{"length = 6?"}
  L -->|No| N13["7F 2E 13"]
  L -->|Yes| DID{"DID configured writable?"}
  DID -->|No| N31["7F 2E 31"]
  DID -->|Yes| SES{"Extended?"}
  SES -->|No| N7F["7F 2E 7F"]
  SES -->|Yes| SEC{"Unlocked?"}
  SEC -->|No| N33["7F 2E 33"]
  SEC -->|Yes| COND{"Safe vehicle condition?"}
  COND -->|No| N22["7F 2E 22"]
  COND -->|Yes| RANGE{"Values/dependencies valid?"}
  RANGE -->|No| N31B["7F 2E 31"]
  RANGE -->|Yes| WRITE["write/persist → 6E F1A0"]
```

### Verification sau write

Read back trước reset chỉ chứng minh RAM mirror. Muốn chứng minh persistence:

```text
2E F1 A0 01 02 01 → positive
22 F1 A0          → verify runtime/RAM
11 01             → hard reset
wait boot/reconnect
22 F1 A0          → verify restored NvM value
verify application output/NMSG behavior
```

---

## 3. Hard Reset `11 01`

### Vì sao không reset ngay khi nhận request?

Nếu MCU reset trước khi response được gửi/confirmed, tester chỉ thấy timeout. DCM/BswM/EcuM phải phối hợp delayed reset.

```mermaid
sequenceDiagram
  autonumber
  participant T as Tester
  participant D as DCM
  participant APP as Reset condition callback
  participant B as BswM/EcuM
  participant P as PduR/CanTp/CanIf
  participant M as Mcu
  T->>D: 11 01
  D->>APP: check programming/NvM/vehicle conditions
  APP-->>D: E_OK
  D->>B: prepare reset mode/request
  D->>P: transmit 51 01
  P-->>D: TpTxConfirmation(E_OK)
  D->>B: execute reset action
  B->>M: Mcu_PerformReset or configured action
  Note over T,M: communication disappears during reboot
  M-->>T: ECU available after startup
  T->>D: reconnect / 22 F190 / session setup
```

### Reset state impact

```mermaid
stateDiagram-v2
  ExtendedUnlocked --> ResetRequested: 11 01 accepted
  ResetRequested --> ResponseSent: 51 01 confirmed
  ResponseSent --> Boot: MCU reset
  Boot --> DefaultLocked: DCM init complete
```

Sau reset, session thường default và security locked. NvM persistent data còn; RAM state và seed/security state được reinitialize. Reset reason có thể được lưu/đọc theo project requirement.

### Negative/edge cases

- `11 02` không configured → `7F 11 12`.
- vehicle moving/NvM critical job → `7F 11 22`.
- session không cho phép → `7F 11 7F`.
- response Tx failure: project policy quyết định cancel reset hay reset sau timeout.
- suppress-positive-response: chỉ dùng nếu service/config cho phép và reset behavior được tester biết.

---

## 4. TesterPresent `3E 00` và S3 timer

### Timeline

```mermaid
sequenceDiagram
  participant T as Tester
  participant D as DCM DSL
  T->>D: 10 03
  D-->>T: 50 03
  Note over D: enter Extended, start S3
  T->>D: 3E 00 before S3 expiry
  D-->>T: 7E 00
  Note over D: restart S3
  T->>D: 3E 80
  Note over D: process subfunction 00 + suppress bit
  Note over T,D: no positive response, S3 restarted
  Note over D: no further request → S3 expires
  D->>D: transition to Default + reset security state
```

`0x80` là suppress-positive-response bit, effective subfunction vẫn `0x00`. Negative response vẫn có thể được gửi nếu request invalid theo protocol rule; suppress bit không phải “mute mọi lỗi”.

Tester interval phải có margin dưới S3, tính cả scheduling/network jitter. Gửi quá nhanh tạo bus load vô ích; quá chậm làm session rơi về default.

---

## 5. ReadDTCInformation `0x19`

### DTC status path từ monitor tới tester

```mermaid
flowchart LR
  MON["Monitor<br/>PREFAILED/PREPASSED"] --> DEB["DEM debounce"] --> EVT["Qualified event"]
  EVT --> STAT["DTC status byte"]
  EVT --> MEM["Event memory"]
  MEM --> SNAP["Snapshot/freezeframe"]
  MEM --> EXT["Extended data"]
  T["Tester 19 xx"] --> DCM["DCM DSP"] --> DEM["DEM DCM interface"]
  DEM --> STAT
  DEM --> SNAP
  DEM --> EXT
```

### `19 02` report DTC by status mask

```text
Request:  19 02 08
          │  │  └ requested status mask, example confirmedDTC
          │  └ reportDTCByStatusMask
          └ ReadDTCInformation

Response: 59 02 <availabilityMask> <DTC1:3 bytes> <status1> ...
```

```mermaid
sequenceDiagram
  participant T as Tester
  participant D as DCM
  participant DEM
  T->>D: 19 02 statusMask
  D->>DEM: SetDTCFilter(mask, format, origin,...)
  DEM-->>D: filter accepted + count/status
  loop until no matching element
    D->>DEM: GetNextFilteredDTC
    DEM-->>D: DTC + status
  end
  D-->>T: 59 02 availabilityMask + records
```

Vendor/release API names may differ, nhưng concept select/filter/iterate là cốt lõi. DCM không scan raw NvM.

### Snapshot và extended data

`19 04`/`19 06`-style requests chọn DTC và record. Snapshot là data tại trigger capture; extended data là counters/state configured. Record number `0xFF` có special meaning tùy subfunction/spec. Test unknown DTC, wrong record, memory origin và response multi-frame.

### Status byte interpretation

Tester phải AND requested mask với availability mask. `testFailed`, pending, confirmed, notCompleted, sinceLastClear và warningIndicatorRequested có lifecycle khác. Confirmed không tự clear khi current test passes; aging/clear policy xử lý.

---

## 6. RoutineControl `0x31`

### State machine

```mermaid
stateDiagram-v2
  [*] --> Idle
  Idle --> Running: 31 01 StartRoutine
  Running --> Running: callback returns PENDING
  Running --> Completed: work finished
  Running --> Stopped: 31 02 if supported
  Completed --> Completed: 31 03 RequestResults
  Idle --> SequenceError: 31 03 before Start
```

### Async routine flow

```mermaid
sequenceDiagram
  participant T as Tester
  participant D as DCM DSP
  participant R as Routine callback
  T->>D: 31 01 FF 10 + input record
  D->>R: Start(OpStatus=INITIAL)
  R-->>D: DCM_E_PENDING
  D-->>T: 7F 31 78 before P2 expiry
  loop each Dcm_MainFunction
    D->>R: Start(OpStatus=PENDING)
    R-->>D: PENDING or E_OK + output
  end
  D-->>T: 71 01 FF 10 + routineStatusRecord
  T->>D: 31 03 FF 10
  D->>R: RequestResults
  R-->>D: stored result
  D-->>T: 71 03 FF 10 + result
```

RequestResults không chạy lại routine. Application phải định nghĩa result lifetime, behavior sau reset/session change, concurrent start và cancellation (`DCM_CANCEL`/equivalent OpStatus khi applicable).

### NRC map

| Condition | NRC thường dùng |
|---|---:|
| RID/subfunction/input out of range | `31` |
| result requested before start | `24` |
| vehicle/precondition invalid | `22` |
| security locked | `33` |
| still processing | `78` |
| general execution failure | mapping theo callback/spec |

---

## 7. Physical và functional addressing

```mermaid
flowchart TD
  TESTER["Tester"] -->|Physical request| ECU1["ECU 1 only"]
  TESTER -->|Functional request| F["Functional address"]
  F --> ECU1
  F --> ECU2["ECU 2"]
  F --> ECU3["ECU 3"]
```

Functional broadcast có nguy cơ response storm. Protocol/OEM quy định service nào được phản hồi hoặc suppress khi functional. Write DID, security, reset thường được giới hạn physical, nhưng không coi đó là universal rule—phải xem service/addressing configuration.

Trong AUTOSAR, physical và functional thường là các `DcmDslProtocolRx`/RxPdu configuration khác nhau cùng connection/protocol context. CanIf/CanTp route dựa configured PDU ID; DCM biết request type từ Rx connection metadata, không suy từ SID.

---

## 8. Debug checklist theo symptom

```mermaid
flowchart TD
  X["Tester timeout/NRC sai"] --> CAN{"CAN frame có vào ECU?"}
  CAN -->|No| PHY["bitrate, ID, transceiver, bus-off"]
  CAN -->|Yes| TP{"CanTp complete N-SDU?"}
  TP -->|No| TPD["PCI, FC, SN, BS/STmin, N timers, padding"]
  TP -->|Yes| ROUTE{"PduR→DCM route đúng?"}
  ROUTE -->|No| CFG["PDU refs/generated routing"]
  ROUTE -->|Yes| ACC{"Service/session/security/mode?"}
  ACC -->|No| NRC["Find exact NRC producer"]
  ACC -->|Yes| CB{"Callback result/lifetime?"}
  CB -->|Pending| TIME["P2/P2*, 0x78, main function"]
  CB -->|OK| TX["Response buffer→CanTp→CanIf confirmation"]
```

Thu thập cùng timestamp: CAN trace, DCM service/session/security state, callback entry/result, NvM/DEM job state và reset reason. Không sửa random timeout trước khi biết boundary đầu tiên mất data.

## 9. Bài tập bắt buộc

1. Vẽ lại VIN flow và ghi rõ byte nào thuộc PCI, UDS header và data.
2. Thêm VIN read vào lab rồi làm hỏng CF sequence để chứng minh reassembly fail.
3. Thêm vehicle-speed condition cho WriteDID; test NRC `22`.
4. Thay NvM sync demo thành async 3-cycle, phát `78` đúng P2 model.
5. Thêm S3 timer và chứng minh session/security reset khi hết hạn.
6. Tạo 3 DEM events, status mask filter và response `19 02` multi-frame.
7. Viết test chứng minh reset chỉ xảy ra sau simulated TxConfirmation.
8. Lập trace table requirement → ECUC container → generated artifact → callback → test.

Nguồn chuẩn để đọc sâu: [AUTOSAR DCM SWS R24-11](https://www.autosar.org/fileadmin/standards/R24-11/CP/AUTOSAR_CP_SWS_DiagnosticCommunicationManager.pdf), [AUTOSAR DEM SWS](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticEventManager.pdf).
