#include "diag_server.hpp"

#include <cassert>

int main()
{
    diag::Server ecu;
    assert(ecu.process({0x99u}) == diag::Bytes({0x7Fu, 0x99u, 0x11u}));
    assert(ecu.process({0x22u, 0xF1u}) == diag::Bytes({0x7Fu, 0x22u, 0x13u}));
    const auto vin = ecu.process({0x22u, 0xF1u, 0x90u});
    assert(vin.size() == 20u && vin[0] == 0x62u);
    assert(diag::reassemble(diag::segment(vin)) == vin);
    assert(ecu.process({0x2Eu, 0xF1u, 0xA0u, 1u, 2u, 1u}) == diag::Bytes({0x7Fu, 0x2Eu, 0x7Fu}));
    assert(ecu.process({0x10u, 0x03u})[0] == 0x50u);
    assert(ecu.process({0x27u, 0x02u, 0u, 0u}) == diag::Bytes({0x7Fu, 0x27u, 0x24u}));
    assert(ecu.process({0x27u, 0x01u}) == diag::Bytes({0x67u, 1u, 0x5Au, 0x3Cu}));
    assert(ecu.process({0x27u, 0x02u, 0u, 0u}) == diag::Bytes({0x7Fu, 0x27u, 0x35u}));
    assert(ecu.process({0x27u, 0x02u, 0xFFu, 0x66u})[0] == 0x67u);
    assert(ecu.process({0x2Eu, 0xF1u, 0xA0u, 2u, 2u, 1u}) == diag::Bytes({0x7Fu, 0x2Eu, 0x31u}));
    assert(ecu.process({0x2Eu, 0xF1u, 0xA0u, 1u, 2u, 1u})[0] == 0x6Eu);
    assert(ecu.process({0x31u, 1u, 0xFFu, 0x10u})[0] == 0x71u);
    ecu.main_function();
    ecu.simulated_reset();
    assert(ecu.process({0x22u, 0xF1u, 0xA0u}) == diag::Bytes({0x62u, 0xF1u, 0xA0u, 1u, 2u, 1u}));
    assert(ecu.process({0x11u, 0x02u}) == diag::Bytes({0x7Fu, 0x11u, 0x12u}));
    assert(ecu.process({0x11u, 0x01u}) == diag::Bytes({0x51u, 0x01u}));
    ecu.main_function();
    assert(ecu.process({0x2Eu, 0xF1u, 0xA0u, 1u, 2u, 1u}) == diag::Bytes({0x7Fu, 0x2Eu, 0x7Fu}));

    diag::Bytes payload(40u);
    for (std::size_t i = 0u; i < payload.size(); ++i) payload[i] = static_cast<std::uint8_t>(i);
    const auto frames = diag::segment(payload);
    assert(frames.size() > 2u);
    assert(diag::reassemble(frames) == payload);
    auto bad = frames;
    bad[1][0] = 0x22u;
    assert(!diag::reassemble(bad));
    return 0;
}
