#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "include/buffer.h"
#include "include/encoding.h"
#include "include/types.h"

// we can define any operation that we want here
namespace rados::cls::nvme_ndp::op{
    struct csd_op{
        std::string o_id;
        std::uint8_t opc;

        void encode(ceph::buffer::list& bl) const {
            ENCODE_START(1, 1, bl);
            encode(o_id, bl);
            encode(opc, bl);
            ENCODE_FINISH(bl);
        }

        void decode(ceph::buffer::list::const_iterator& bl) {
            DECODE_START(1, bl);
            decode(o_id, bl);
            decode(opc, bl);
            DECODE_FINISH(bl);
        }
    };
    WRITE_CLASS_ENCODER(csd_op)
}