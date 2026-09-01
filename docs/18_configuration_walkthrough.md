# Configuration walkthrough — add VIN DID, coding DID và hard reset

## VIN F190

1. Tạo/enable `DcmDspDid` identifier F190.
2. Thêm data element size 17 byte, uint8 array/string-like representation.
3. Chọn big-endian/endianness semantics nếu numeric; ASCII array không byte-swap.
4. Read port synchronous hoặc async theo data source.
5. Map read callback/RTE operation.
6. Cho default + extended session, thường không security.
7. Đảm bảo 0x22 trong DSD service table.
8. DCM TX buffer ≥ 20 byte; CanTp/PduR response route đúng.
9. Test ISO-TP FF/FC/CF, VIN content/length và unknown DID.

## Variant F1A0 (synthetic)

1. DID fixed length 3, read+write.
2. Read access default/extended; write extended only.
3. Write security level 1 và mode rule (vehicle safe state).
4. Condition-check callback validate global preconditions.
5. Write callback validate per-field range, request NvM and return sync/pending result.
6. Decide update timing and ApplyCoding routine dependency.
7. Test full session×security×range×persistence matrix.

## Hard reset

1. Enable service 0x11 and subfunction hardReset.
2. Add session/security/mode access.
3. Configure/reset callback or BswM action integration.
4. Coordinate NvM pending/write-all policy.
5. Ensure TX confirmation/reset delay permits positive response.
6. Test boot/reconnect/reset reason/default session.

## PDU/transport chain

```text
CanIf RxPdu → CanTp RxNSdu → PduR source route → DCM RxPdu/connection
DCM TxPdu → PduR destination route → CanTp TxNSdu → CanIf TxPdu
```

Mỗi arrow là reference/generated identifier riêng. Không so sánh numeric IDs giữa module nếu chúng thuộc namespace khác; trace symbolic reference trong ARXML/generated header.

## Review generated files

`Dcm_Cfg.h` feature/symbol macros; `Dcm_Lcfg/PBcfg` data/access/service tables; `CanTp_*Cfg` NSdu/timers/addressing; `PduR_Dcm/CanTp` route APIs/IDs; RTE headers/stubs cho callbacks; SchM schedule; NvM block config. Vendor generator có thể tổ chức khác, nên review theo responsibility chứ không thuộc line number.
