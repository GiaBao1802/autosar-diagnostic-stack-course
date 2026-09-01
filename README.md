# AUTOSAR Diagnostic Stack — Theory, Configuration and Practice

[![Build and test](https://github.com/GiaBao1802/autosar-diagnostic-stack-course/actions/workflows/build.yml/badge.svg)](https://github.com/GiaBao1802/autosar-diagnostic-stack-course/actions/workflows/build.yml)

Khóa học từ CAN frame tới UDS service và application callback, kèm lab DCM/ISO-TP độc lập. Case study dựa trên **workflow** quan sát trong workspace Toshiba MICROSAR, nhưng không sao chép requirement, identifier, generated code hay source thương mại.

```mermaid
flowchart LR
  TESTER["Diagnostic tester"] --> CAN["CAN/CAN FD"] --> CANIF["CanIf"]
  CANIF --> CANTP["CanTp / ISO-TP"] --> PDUR["PduR"] --> DCM["DCM"]
  DCM --> APP["DID/RID callbacks"]
  DCM --> DEM["DEM / DTC"]
  APP --> NVM["NvM / coding"]
```

## Nội dung

1. [UDS protocol và services](docs/01_uds_protocol.md)
2. [DCM DSL–DSD–DSP](docs/02_dcm_architecture.md)
3. [CanTp/ISO-TP và timers](docs/03_cantp_isotp.md)
4. [End-to-end RX/TX flow](docs/04_full_stack_flow.md)
5. [Toshiba-style requirement/config workflow](docs/05_requirement_to_config.md)
6. [Testing, NRC và debugging](docs/06_testing_debugging.md)
7. [Lab requirement và traceability](docs/07_lab_requirement.md)
8. [DEM deep dive](docs/08_dem_deep_dive.md)
9. [OBD fundamentals](docs/09_obd_fundamentals.md)
10. [Automotive Ethernet và DoIP](docs/10_doip_ethernet.md)
11. [DCM configuration deep dive](docs/11_dcm_configuration_deep_dive.md)
12. [CANoe, ODX/PDX và test workflow](docs/12_tools_odx_canoe.md)
13. [AUTOSAR diagnostic API reference](docs/13_autosar_api_reference.md)
14. [DCM ECUC parameters](docs/14_dcm_ecuc_parameters.md)
15. [CanTp ECUC parameters](docs/15_cantp_ecuc_parameters.md)
16. [DEM ECUC parameters](docs/16_dem_ecuc_parameters.md)
17. [VIN, variant, reset và DTC use cases](docs/17_real_uds_use_cases.md)
18. [Configuration walkthrough](docs/18_configuration_walkthrough.md)
19. [PduR ECUC parameters](docs/19_pdur_ecuc_parameters.md)

## Nguồn tham khảo

Curriculum được rà gap bằng [Udemy AUTOSAR Diagnostics course](https://www.udemy.com/course/autosar-diagnostics-dem-dcm-obd-uds/) từ phần mô tả công khai. Nội dung kỹ thuật trong repo được viết độc lập và đối chiếu AUTOSAR DCM/DEM/DoIP specifications; không sao chép bài giảng trả phí.

## Lab services

- `0x10` DiagnosticSessionControl
- `0x11` ECUReset (Hard Reset)
- `0x22` ReadDataByIdentifier
- `0x2E` WriteDataByIdentifier
- `0x27` SecurityAccess với seed/key học tập
- `0x31` RoutineControl
- `0x3E` TesterPresent
- ISO-TP SF/FF/CF/FC segmentation/reassembly model
- DID `0xF190` VIN response 17 ASCII bytes, tự động thành multi-frame trên Classic CAN

Toy security algorithm không được dùng trong sản phẩm thật.

## Build và chạy lab

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/diag_demo
```
