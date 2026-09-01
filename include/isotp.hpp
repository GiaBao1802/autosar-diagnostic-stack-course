#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace diag {
using Bytes = std::vector<std::uint8_t>;

inline std::vector<Bytes> segment(const Bytes& payload)
{
    std::vector<Bytes> frames;
    if (payload.size() <= 7u) {
        Bytes sf{static_cast<std::uint8_t>(payload.size())};
        sf.insert(sf.end(), payload.begin(), payload.end());
        frames.push_back(std::move(sf));
        return frames;
    }
    Bytes ff{static_cast<std::uint8_t>(0x10u | ((payload.size() >> 8u) & 0x0Fu)),
             static_cast<std::uint8_t>(payload.size() & 0xFFu)};
    const auto first = std::min<std::size_t>(6u, payload.size());
    ff.insert(ff.end(), payload.begin(), payload.begin() + static_cast<std::ptrdiff_t>(first));
    frames.push_back(std::move(ff));
    std::size_t offset = first;
    std::uint8_t sn = 1u;
    while (offset < payload.size()) {
        Bytes cf{static_cast<std::uint8_t>(0x20u | (sn & 0x0Fu))};
        const auto count = std::min<std::size_t>(7u, payload.size() - offset);
        cf.insert(cf.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset),
                  payload.begin() + static_cast<std::ptrdiff_t>(offset + count));
        frames.push_back(std::move(cf));
        offset += count;
        sn = static_cast<std::uint8_t>((sn + 1u) & 0x0Fu);
    }
    return frames;
}

inline std::optional<Bytes> reassemble(const std::vector<Bytes>& frames)
{
    if (frames.empty() || frames[0].empty()) return std::nullopt;
    const auto type = frames[0][0] >> 4u;
    if (type == 0u) {
        const auto length = frames[0][0] & 0x0Fu;
        if (frames[0].size() != static_cast<std::size_t>(length) + 1u) return std::nullopt;
        return Bytes(frames[0].begin() + 1, frames[0].end());
    }
    if ((type != 1u) || (frames[0].size() < 2u)) return std::nullopt;
    const std::size_t total = (static_cast<std::size_t>(frames[0][0] & 0x0Fu) << 8u) | frames[0][1];
    Bytes result(frames[0].begin() + 2, frames[0].end());
    std::uint8_t expected = 1u;
    for (std::size_t i = 1u; i < frames.size() && result.size() < total; ++i) {
        if (frames[i].empty() || frames[i][0] != static_cast<std::uint8_t>(0x20u | expected)) return std::nullopt;
        result.insert(result.end(), frames[i].begin() + 1, frames[i].end());
        expected = static_cast<std::uint8_t>((expected + 1u) & 0x0Fu);
    }
    if (result.size() < total) return std::nullopt;
    result.resize(total);
    return result;
}
}
