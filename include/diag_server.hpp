#pragma once

#include "isotp.hpp"

#include <array>
#include <cstdint>

namespace diag {
enum class Session : std::uint8_t { Default = 1u, Extended = 3u };

class Server final {
public:
    Bytes process(const Bytes& request);
    void main_function();
    void simulated_reset();
    std::array<std::uint8_t, 3> persistent_coding() const noexcept { return nvm_; }
private:
    Bytes negative(std::uint8_t sid, std::uint8_t nrc) const;
    Bytes session_control(const Bytes& request);
    Bytes ecu_reset(const Bytes& request);
    Bytes security_access(const Bytes& request);
    Bytes read_did(const Bytes& request) const;
    Bytes write_did(const Bytes& request);
    Bytes routine_control(const Bytes& request);
    Session session_{Session::Default};
    bool unlocked_{};
    bool seed_requested_{};
    bool coding_valid_{};
    bool nvm_pending_{};
    bool reset_pending_{};
    std::array<std::uint8_t, 3> runtime_{};
    std::array<std::uint8_t, 3> nvm_{};
};
}
