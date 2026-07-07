#include "ublox/ubx_parser.h"

#include <cstdint>
#include <optional>

void UbxParser::reset() {
    state_ = State::SYNC1;
    class_id_ = 0;
    msg_id_ = 0;
    len_ = 0;
    len_idx_ = 0;
    ck_a_calc_ = 0;
    ck_b_calc_ = 0;
    ck_a_recv_ = 0;
    ck_b_recv_ = 0;
}

void UbxParser::ck_update(uint8_t b) {
    ck_a_calc_ += b;
    ck_b_calc_ += ck_a_calc_;
}

void UbxParser::feed(const std::uint8_t* data, std::size_t size,
                     std::function<void(UbxMessageView&&)> on_msg) {
    for (size_t i = 0; i < size; i++) {
        const uint8_t b = data[i];

        switch (state_) {

            // ---------------- SYNC 0xB5 ----------------
            case State::SYNC1:
                if (b == 0xB5)
                    state_ = State::SYNC2;
                break;

            // ---------------- SYNC 0x62 ----------------
            case State::SYNC2:
                if (b == 0x62) {
                    state_ = State::CLASS;
                } else if (b == 0xB5) {
                    state_ = State::SYNC2;
                } else {
                    state_ = State::SYNC1;
                }
                break;

            // ---------------- CLASS ----------------
            case State::CLASS:
                class_id_ = b;
                ck_a_calc_ = ck_b_calc_ = 0;
                ck_update(b);
                state_ = State::ID;
                break;

            // ---------------- ID ----------------
            case State::ID:
                msg_id_ = b;
                ck_update(b);
                state_ = State::LEN1;
                break;

            // ---------------- LEN1 ----------------
            case State::LEN1:
                len_ = b;
                ck_update(b);
                state_ = State::LEN2;
                break;

            // ---------------- LEN2 ----------------
            case State::LEN2:
                len_ |= (uint16_t(b) << 8);
                if (len_ > MAX_LEN) {
                    reset();
                    state_ = State::SYNC1;
                    break;
                }
                ck_update(b);

                if (len_ > MAX_LEN) {
                    reset();
                    break;
                }

                payload_.resize(len_);
                len_idx_ = 0;

                state_ = (len_ == 0) ? State::CK_A : State::PAYLOAD;
                break;

            // ---------------- PAYLOAD ----------------
            case State::PAYLOAD:
                payload_[len_idx_++] = b;
                ck_update(b);

                if (len_idx_ == len_)
                    state_ = State::CK_A;
                break;

            // ---------------- CK_A ----------------
            case State::CK_A:
                ck_a_recv_ = b;
                state_ = State::CK_B;
                break;

            // ---------------- CK_B ----------------
            case State::CK_B:
                ck_b_recv_ = b;

                if (ck_a_recv_ == ck_a_calc_ && ck_b_recv_ == ck_b_calc_) {

                    uint16_t msg_id = (uint16_t(class_id_) << 8) | uint16_t(msg_id_);

                    UbxMessageView msg{msg_id, payload_.data(), len_};

                    on_msg(std::move(msg));
                }

                reset();
                state_ = State::SYNC1;
                break;
        }
    }
}