# UDS protocol và services

UDS (ISO 14229) định nghĩa application protocol; transport có thể là ISO-TP trên CAN, DoIP trên Ethernet hoặc transport khác. Request thường có `SID + optional subfunction/DID/RID + data`.

```text
Positive response SID = Request SID + 0x40
Negative response     = 0x7F, Request SID, NRC
```

| SID | Service | Ví dụ payload |
|---:|---|---|
| 10 | session control | `10 03` extended session |
| 11 | ECU reset | `11 01` hard reset |
| 14 | clear DTC | group of DTC |
| 19 | read DTC | subfunction + mask |
| 22 | read DID | DID high, DID low |
| 27 | security access | requestSeed/sendKey |
| 28 | communication control | subfunction + comm type |
| 2E | write DID | DID + data |
| 2F | IO control | DID + control parameter |
| 31 | routine control | start/stop/result + RID |
| 34/36/37 | download/transfer/exit | flashing sequence |
| 3E | tester present | keep session alive |
| 85 | control DTC setting | on/off |

NRC thường gặp: `0x11` serviceNotSupported, `0x12` subFunctionNotSupported, `0x13` incorrectLengthOrFormat, `0x22` conditionsNotCorrect, `0x24` requestSequenceError, `0x31` requestOutOfRange, `0x33` securityAccessDenied, `0x35` invalidKey, `0x36` exceedNumberOfAttempts, `0x37` requiredTimeDelayNotExpired, `0x78` responsePending.

Session, security và conditions là ba gate khác nhau. Service có thể được support nhưng chỉ trong extended session, cần security level và vehicle state phù hợp.
