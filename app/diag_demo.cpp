#include "diag_server.hpp"

#include <iomanip>
#include <iostream>

static void print(const diag::Bytes& bytes)
{
    for (auto b : bytes) std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(b) << ' ';
    std::cout << '\n';
}

int main()
{
    diag::Server ecu;
    const auto vin = ecu.process({0x22u, 0xF1u, 0x90u});
    print(vin);
    for (const auto& frame : diag::segment(vin)) print(frame);
    print(ecu.process({0x10u, 0x03u}));
    print(ecu.process({0x27u, 0x01u}));
    print(ecu.process({0x27u, 0x02u, 0xFFu, 0x66u}));
    print(ecu.process({0x2Eu, 0xF1u, 0xA0u, 0x01u, 0x02u, 0x01u}));
    ecu.main_function();
    ecu.simulated_reset();
    print(ecu.process({0x22u, 0xF1u, 0xA0u}));
    print(ecu.process({0x11u, 0x01u}));
    ecu.main_function();
}
