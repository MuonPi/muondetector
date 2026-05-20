#ifndef UBX_PARSER_H
#define UBX_PARSER_H

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

class UbxParser {
  public:
    enum class State { SYNC1, SYNC2, CLASS, ID, LEN1, LEN2, PAYLOAD, CK_A, CK_B };

    struct UbxMessageView {
        std::uint16_t msg_id;
        const std::uint8_t* payload;
        std::uint16_t len;
    };

    void feed(const uint8_t* data, std::size_t size, std::function<void(UbxMessageView&&)> on_msg);

  private:
    void reset();
    void ck_update(uint8_t b);

    State state_ = State::SYNC1;

    std::uint8_t class_id_ = 0;
    std::uint8_t msg_id_ = 0;

    std::uint16_t len_ = 0;
    std::uint16_t len_idx_ = 0;

    std::uint8_t ck_a_calc_ = 0;
    std::uint8_t ck_b_calc_ = 0;

    std::uint8_t ck_a_recv_ = 0;
    std::uint8_t ck_b_recv_ = 0;

    std::vector<std::uint8_t> payload_; // reused buffer (NO allocations per packet)
    inline static constexpr uint16_t MAX_LEN = 4096;
};

#endif // UBX_PARSER_H