#include "diag_server.hpp"

#include <algorithm>

namespace diag {
namespace {
constexpr std::uint16_t coding_did = 0xF1A0u;
constexpr std::uint16_t apply_rid = 0xFF10u;
constexpr std::uint16_t seed = 0x5A3Cu;
constexpr std::uint16_t toy_key = static_cast<std::uint16_t>(seed ^ 0xA55Au);
bool coding_did_matches(const Bytes& r) { return r.size() >= 3u && r[1] == 0xF1u && r[2] == 0xA0u; }
bool vin_did_matches(const Bytes& r) { return r.size() >= 3u && r[1] == 0xF1u && r[2] == 0x90u; }
}

Bytes Server::negative(std::uint8_t sid, std::uint8_t nrc) const { return {0x7Fu, sid, nrc}; }

Bytes Server::process(const Bytes& request)
{
    if (request.empty()) return {};
    switch (request[0]) {
    case 0x10u: return session_control(request);
    case 0x11u: return ecu_reset(request);
    case 0x22u: return read_did(request);
    case 0x27u: return security_access(request);
    case 0x2Eu: return write_did(request);
    case 0x31u: return routine_control(request);
    case 0x3Eu: return request.size() == 2u ? Bytes{0x7Eu, request[1]} : negative(0x3Eu, 0x13u);
    default: return negative(request[0], 0x11u);
    }
}

Bytes Server::ecu_reset(const Bytes& r)
{
    if (r.size() != 2u) return negative(0x11u, 0x13u);
    if (r[1] != 0x01u) return negative(0x11u, 0x12u);
    reset_pending_ = true;
    return {0x51u, 0x01u};
}

Bytes Server::session_control(const Bytes& r)
{
    if (r.size() != 2u) return negative(0x10u, 0x13u);
    if (r[1] == 0x01u) session_ = Session::Default;
    else if (r[1] == 0x03u) session_ = Session::Extended;
    else return negative(0x10u, 0x12u);
    unlocked_ = false;
    seed_requested_ = false;
    return {0x50u, r[1], 0x00u, 0x32u, 0x01u, 0xF4u};
}

Bytes Server::security_access(const Bytes& r)
{
    if (session_ != Session::Extended) return negative(0x27u, 0x7Fu);
    if (r.size() == 2u && r[1] == 0x01u) {
        seed_requested_ = true;
        return {0x67u, 0x01u, static_cast<std::uint8_t>(seed >> 8u), static_cast<std::uint8_t>(seed)};
    }
    if (r.size() != 4u || r[1] != 0x02u) return negative(0x27u, 0x13u);
    if (!seed_requested_) return negative(0x27u, 0x24u);
    const auto key = static_cast<std::uint16_t>((static_cast<std::uint16_t>(r[2]) << 8u) | r[3]);
    if (key != toy_key) return negative(0x27u, 0x35u);
    unlocked_ = true;
    seed_requested_ = false;
    return {0x67u, 0x02u};
}

Bytes Server::read_did(const Bytes& r) const
{
    if (r.size() != 3u) return negative(0x22u, 0x13u);
    if (vin_did_matches(r)) {
        static constexpr char vin[] = "TRAININGVIN000001";
        Bytes response{0x62u, 0xF1u, 0x90u};
        response.insert(response.end(), vin, vin + 17);
        return response;
    }
    if (!coding_did_matches(r)) return negative(0x22u, 0x31u);
    return {0x62u, 0xF1u, 0xA0u, runtime_[0], runtime_[1], runtime_[2]};
}

Bytes Server::write_did(const Bytes& r)
{
    if (r.size() != 6u) return negative(0x2Eu, 0x13u);
    if (!coding_did_matches(r)) return negative(0x2Eu, 0x31u);
    if (session_ != Session::Extended) return negative(0x2Eu, 0x7Fu);
    if (!unlocked_) return negative(0x2Eu, 0x33u);
    if (r[3] > 1u || r[4] > 2u || r[5] > 1u) return negative(0x2Eu, 0x31u);
    std::copy_n(r.begin() + 3, 3, runtime_.begin());
    coding_valid_ = true;
    nvm_pending_ = true;
    return {0x6Eu, 0xF1u, 0xA0u};
}

Bytes Server::routine_control(const Bytes& r)
{
    if (r.size() != 4u) return negative(0x31u, 0x13u);
    const auto rid = static_cast<std::uint16_t>((static_cast<std::uint16_t>(r[2]) << 8u) | r[3]);
    if (rid != apply_rid || r[1] != 0x01u) return negative(0x31u, 0x31u);
    if (!coding_valid_) return negative(0x31u, 0x22u);
    return {0x71u, 0x01u, 0xFFu, 0x10u, 0x00u};
}

void Server::main_function()
{
    if (nvm_pending_) { nvm_ = runtime_; nvm_pending_ = false; }
    if (reset_pending_) { simulated_reset(); reset_pending_ = false; }
}
void Server::simulated_reset() { session_ = Session::Default; unlocked_ = seed_requested_ = false; runtime_ = nvm_; }
}
