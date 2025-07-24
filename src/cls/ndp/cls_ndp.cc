#include <cerrno>

#include "objclass/objclass.h"
#include "cls/ndp/cls_ndp_ops.h"
//#include "spdk/include/spdk/nvme.h"
//#include "spdk/include/spdk/env.h"

CLS_VER(1,0)
CLS_NAME(ndp)

namespace rados::cls::ndp{
    /*
    static struct spdk_nvme_ctrlr* g_ctrlr = nullptr;
    static struct spdk_nvme_qpair* g_qpair = nullptr;


    static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
                     struct spdk_nvme_ctrlr_opts *opts) {
        CLS_LOG(5, "Probing NVMe Controller at %s", trid->traddr);
        return true;
    }

    static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
                        struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts) {
        CLS_LOG(5, "Attached to NVMe Controller at %s", trid->traddr);
        struct spdk_nvme_ns *ns = spdk_nvme_ctrlr_get_ns(ctrlr, 1);
        if (!ns || !spdk_nvme_ns_is_active(ns)) {
            CLS_LOG(5, "Namespace is not valid or inactive");
            return;
        }
        g_ctrlr = ctrlr;

        struct spdk_nvme_qpair *qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);
        if (!qpair) {
            CLS_LOG(5, "Failed to allocate I/O qpair");
            return;
        }
        g_qpair = qpair;

        CLS_LOG(5, "Successfully created I/O qpair");
    }

    int spdk_initialize() {
        int rc = spdk_env_init(nullptr);
        if (rc != 0) {
            CLS_ERR("Failed to init SPDK env");
            return rc;
        }

        struct spdk_nvme_transport_id trid = {};
        // snprintf(trid.trtype, sizeof(trid.trtype), "PCIe"); we can choose which type of driver
        rc = spdk_nvme_probe(&trid, nullptr, probe_cb, attach_cb, nullptr);
        if (rc != 0) {
            CLS_LOG(5, "Failed to probe NVMe ctrlr\n");
            return rc;
        }

        if (g_ctrlr == nullptr || g_qpair == nullptr) {
            CLS_LOG(5, "Failed to find ctrlr or qpair");
            return -1;
        }

        return 0;
    }

    static void nvme_cb(void* arg, const struct spdk_nvme_cpl *cpl) {
        if (spdk_nvme_cpl_is_error(cpl)) {
            CLS_ERR("NVMe command failed: SC=0x%x, SCT=0x%x", cpl->status.sc, cpl->status.sct);
        } else {
            CLS_LOG(5, "NVMe command success. CDW0 = 0x%08x", cpl->cdw0);
            uint8_t* response_data = static_cast<uint8_t*>(arg);
            CLS_LOG(5, "Returned data (first 64 bytes):");
            for (int i = 0; i < 64; ++i) {
                CLS_LOG(5, "%02x ", response_data[i]);
            }
        }
    }

    int nvme_send_task(cls_method_context_t hctx,
        ceph::buffer::list* in, ceph::buffer::list* out) {
        op::csd_op op;
        try {
            auto iter = in->cbegin();
            decode(op, iter);   // get the op that we want to send
        } catch (const ceph::buffer::error& err) {
            CLS_ERR("ERROR: %s: failed to decode request: %s", __PRETTY_FUNCTION__,
	            err.what());
            return -EINVAL;
        }
        CLS_LOG(5, "Parsed opcode=%u, object_name=%s", op.opc, op.o_id.c_str());
        
        std::vector<std::pair<uint64_t, uint64_t>> physical_info;
        int r = cls_cxx_get_phyical_info(hctx, &physical_addr_info); // new method defined here to get physical information of target object
        if (r < 0) {
            return r;
        }
    }
    */

    int ndp_getphyinfo(cls_method_context_t hctx,
                   ceph::buffer::list* in, ceph::buffer::list* out) {
    CLS_LOG(5, "ndp_get_physical_info: called");

    ceph::bufferlist physical_extents;
    int r = cls_cxx_get_physical_info(hctx, &physical_extents);
    if (r < 0) {
        CLS_ERR("ndp_get_physical_info: failed to get physical info, err=%d", r);
        return r;
    }

    std::string result = physical_extents.to_str();
    if (result.empty()) {
        result = "no physical extents found\n";
    }

    out->append(result);

    CLS_LOG(5, "ndp_get_physical_info: output %zu extents", result.length());
    return 0;
}


    int ndp_grep(cls_method_context_t hctx,  
        ceph::buffer::list* in, ceph::buffer::list* out) {  
        std::string target = in->to_str();  
        
        CLS_LOG(5, "ndp_grep: target=%s", target.c_str());  
        
        const int chunk_size = 4096;
        int offset = 0;  
        size_t match_count = 0;  
        
        // 获取对象大小  
        uint64_t size;  
        int r = cls_cxx_stat(hctx, &size, NULL);  
        if (r < 0) {  
            return r;  
        }  
        
        if (size == 0) {  
            out->append("0");  // 直接添加字符串  
            return 0;  
        }  
        
        // 分块读取并搜索  
        std::string overlap;  
        
        while (offset < size) {  
            int read_size = std::min(chunk_size, (int)(size - offset));  
            
            ceph::bufferlist chunk_data;  
            r = cls_cxx_read(hctx, offset, read_size, &chunk_data);  
            if (r < 0) {  
                CLS_ERR("ndp_grep: failed to read at offset %d",   
                        offset);  
                return r;  
            }  
            
            std::string chunk_str = chunk_data.to_str();  
            std::string search_str = overlap + chunk_str;  
            
            // 计算匹配数量  
            size_t pos = 0;  
            while ((pos = search_str.find(target, pos)) != std::string::npos) {  
                match_count++;  
                pos += 1;  
            }  
            
            // 保存重叠部分  
            if (target.length() > 1 && offset + read_size < size) {  
                size_t overlap_size = std::min(target.length() - 1, chunk_str.length());  
                overlap = chunk_str.substr(chunk_str.length() - overlap_size);  
            } else {  
                overlap.clear();  
            }  
            
            offset += read_size;  
        }  
        
        // 将匹配数量转换为字符串并添加到输出  
        std::string result = std::to_string(match_count);  
        out->append(result);  
        
        CLS_LOG(5, "ndp_grep: found %lu matches", match_count);  
        return 0;  
    }

    int ndp_co_grep(cls_method_context_t hctx,  
        ceph::buffer::list* in, ceph::buffer::list* out) {  
        std::string target = in->to_str();  
        CLS_LOG(5, "ndp_grep: target=%s", target.c_str());  

        const int chunk_size = 4096;
        int offset = 0;  
        size_t match_count = 0;  

        // 获取对象大小  
        uint64_t size;  
        int r = cls_cxx_stat(hctx, &size, NULL);  
        if (r < 0) return r;  
        if (size == 0) {
            out->append("0");  // 空对象
            return 0;
        }

        // 计算 CLS 要处理的 70% 的范围
        uint64_t cutoff = size * 70 / 100;

        std::string overlap;  

        // 读取前 70% 的数据并匹配
        while ((uint64_t)offset < cutoff) {
            int read_size = std::min(chunk_size, (int)(cutoff - offset));  
            
            ceph::bufferlist chunk_data;  
            r = cls_cxx_read(hctx, offset, read_size, &chunk_data);  
            if (r < 0) {
                CLS_ERR("ndp_grep: read failed at offset %d", offset);  
                return r;  
            }

            std::string chunk_str = chunk_data.to_str();  
            std::string search_str = overlap + chunk_str;  

            // 匹配
            size_t pos = 0;  
            while ((pos = search_str.find(target, pos)) != std::string::npos) {
                match_count++;
                pos += 1;
            }

            // 保存重叠
            if (target.length() > 1 && (uint64_t)(offset + read_size) < cutoff) {
                size_t overlap_size = std::min(target.length() - 1, chunk_str.length());  
                overlap = chunk_str.substr(chunk_str.length() - overlap_size);  
            } else {
                overlap.clear();  
            }

            offset += read_size;
        }

        // 处理剩余 30% 数据，直接打包输出到 out 让客户端处理
        if (offset < (int)size) {
            int remaining = size - offset;
            ceph::bufferlist remaining_data;
            r = cls_cxx_read(hctx, offset, remaining, &remaining_data);
            if (r < 0) return r;

            // 我们将结果封装为：<匹配数>\n<剩余原始数据>
            std::string header = std::to_string(match_count) + "\n";
            out->append(header);
            out->claim_append(remaining_data);  // 高效追加剩余数据
        } else {
            // 没有剩余数据，只返回匹配数
            out->append(std::to_string(match_count));
        }

        CLS_LOG(5, "ndp_grep: CLS match_count = %lu, remaining data size = %lu", 
                match_count, size - cutoff);

        return 0;
    }


    int ndp_stat64(cls_method_context_t hctx,  
        ceph::buffer::list* in, ceph::buffer::list* out) {    
        CLS_LOG(5, "ndp_stat64: calculating min, max, and sum from binary data");  
        
        // 获取对象大小  
        uint64_t size;  
        int r = cls_cxx_stat(hctx, &size, NULL);  
        if (r < 0) {  
            CLS_ERR("ndp_stat64: failed to stat object");  
            return r;  
        }  
        
        if (size == 0) {  
            out->append("min=0,max=0,sum=0,count=0");  
            return 0;  
        }  
        
        // 读取整个对象数据  
        ceph::bufferlist data;  
        r = cls_cxx_read(hctx, 0, size, &data);  
        if (r < 0) {  
            CLS_ERR("ndp_stat64: failed to read object");  
            return r;  
        }  
        
        // 计算需要补0的字节数  
        uint64_t padding_needed = 0;  
        if (size % sizeof(int64_t) != 0) {
            padding_needed = sizeof(int64_t) - (size % sizeof(int64_t));  
            data.append_zero(padding_needed);  // 向后补0  
        }
        
        // 现在数据大小是8字节的倍数  
        uint64_t total_size = size + padding_needed;  
        uint64_t num_values = total_size / sizeof(int64_t);  
        
        // 获取原始数据指针  
        const char* raw_data = data.c_str();  
        const int64_t* values = reinterpret_cast<const int64_t*>(raw_data);  
        
        int64_t min_val = values[0];  
        int64_t max_val = values[0];  
        int64_t sum = 0;  
        
        // 遍历所有64位数值  
        for (uint64_t i = 0; i < num_values; i++) {  
            int64_t value = values[i];  
            
            min_val = std::min(min_val, value);  
            max_val = std::max(max_val, value);  
            sum += value;  
        }  
        
        // 格式化结果  
        std::string result = "min=" + std::to_string(min_val) +   
                        ",max=" + std::to_string(max_val) +   
                        ",sum=" + std::to_string(sum) +   
                        ",count=" + std::to_string(num_values);  
        out->append(result);  
        
        CLS_LOG(5, "ndp_stat64: processed %lu 64-bit values (padded %lu bytes)",   
                num_values, padding_needed);  
        return 0;  
    }

    int ndp_stat32(cls_method_context_t hctx,  
        ceph::buffer::list* in, ceph::buffer::list* out) {  
        CLS_LOG(5, "ndp_stat32: calculating min, max, and sum from binary data");  
        
        // 获取对象大小  
        uint64_t size;  
        int r = cls_cxx_stat(hctx, &size, NULL);  
        if (r < 0) {  
            CLS_ERR("ndp_stat32: failed to stat object");  
            return r;  
        }  
        
        if (size == 0) {  
            out->append("min=0,max=0,sum=0,count=0");  
            return 0;  
        }  
        
        // 读取整个对象数据  
        ceph::bufferlist data;  
        r = cls_cxx_read(hctx, 0, size, &data);  
        if (r < 0) {  
            CLS_ERR("ndp_stat32: failed to read object");  
            return r;  
        }  
        
        // 计算需要补0的字节数  
        uint64_t padding_needed = 0;  
        if (size % sizeof(int32_t) != 0) {  
            padding_needed = sizeof(int32_t) - (size % sizeof(int32_t));  
            data.append_zero(padding_needed);  // 向后补0  
        }  
        
        // 现在数据大小是4字节的倍数  
        uint64_t total_size = size + padding_needed;  
        uint64_t num_values = total_size / sizeof(int32_t);  
        
        // 获取原始数据指针  
        const char* raw_data = data.c_str();  
        const int32_t* values = reinterpret_cast<const int32_t*>(raw_data);  
        
        int32_t min_val = values[0];  
        int32_t max_val = values[0];  
        int64_t sum = 0;  // 使用64位避免溢出  
        
        // 遍历所有32位数值  
        for (uint64_t i = 0; i < num_values; i++) {  
            int32_t value = values[i];  
            
            min_val = std::min(min_val, value);  
            max_val = std::max(max_val, value);  
            sum += value;  
        }  
        
        // 格式化结果  
        std::string result = "min=" + std::to_string(min_val) +   
                        ",max=" + std::to_string(max_val) +   
                        ",sum=" + std::to_string(sum) +   
                        ",count=" + std::to_string(num_values);  
        out->append(result);  
        
        CLS_LOG(5, "ndp_stat32: processed %lu 32-bit values (padded %lu bytes)",   
                num_values, padding_needed);  
        return 0;  
    }
    int ndp_bitmap(cls_method_context_t hctx,  
        ceph::buffer::list* in, ceph::buffer::list* out) {  
        CLS_LOG(5, "ndp_bitmap: processing bitmap operations");  
        
        // 获取对象大小  
        uint64_t size;  
        int r = cls_cxx_stat(hctx, &size, NULL);  
        if (r < 0) {  
            CLS_ERR("ndp_bitmap: failed to stat object");  
            return r;  
        }  
        
        if (size == 0) {  
            out->append("bits_set=0,bits_clear=0,total_bits=0");  
            return 0;  
        }  
        
        // 读取整个对象数据  
        ceph::bufferlist data;  
        r = cls_cxx_read(hctx, 0, size, &data);  
        if (r < 0) {  
            CLS_ERR("ndp_bitmap: failed to read object");  
            return r;  
        }  
            
        // 获取原始数据指针  
        const char* raw_data = data.c_str();  
        const uint8_t* bitmap_data = reinterpret_cast<const uint8_t*>(raw_data);  
        
        uint64_t bits_set = 0;  
        uint64_t bits_clear = 0;  
        uint64_t total_bits = size * 8;  // 每字节8位  
        
        // 遍历所有字节，统计位数  
        for (uint64_t i = 0; i < size; i++) {  
            uint8_t byte_val = bitmap_data[i];  
            
            // 统计当前字节中设置的位数  
            for (int bit = 0; bit < 8; bit++) {  
                if (byte_val & (1 << bit)) {  
                    bits_set++;  
                } else {  
                    bits_clear++;  
                }  
            }  
        }  
        
        // 格式化结果  
        std::string result = "bits_set=" + std::to_string(bits_set) +   
                        ",bits_clear=" + std::to_string(bits_clear) +   
                        ",total_bits=" + std::to_string(total_bits);  
        out->append(result);  
        
        CLS_LOG(5, "ndp_bitmap: processed %lu bytes, %lu bits set, %lu bits clear",   
                size, bits_set, bits_clear);  
        return 0;  
    }
}

CLS_INIT(ndp)
{
    using namespace::rados::cls::ndp;
    CLS_LOG(0, "Loading ndp class!");

    cls_handle_t h_class;
    // cls_method_handle_t h_nvme_send_task;
    cls_method_handle_t h_ndp_getphyinfo;
    cls_method_handle_t h_ndp_grep;
    cls_method_handle_t h_ndp_co_grep;
    cls_method_handle_t h_ndp_stat64;
    cls_method_handle_t h_ndp_stat32;
    cls_method_handle_t h_ndp_bitmap;

    cls_register("ndp", &h_class);
    /*
    cls_register_cxx_method(h_class, "nvme_send_task",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            nvme_send_task, &h_nvme_send_task);
    */
    cls_register_cxx_method(h_class, "ndp_getphyinfo",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_getphyinfo, &h_ndp_getphyinfo);

    cls_register_cxx_method(h_class, "ndp_grep",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_grep, &h_ndp_grep);
    
    cls_register_cxx_method(h_class, "ndp_co_grep",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_co_grep, &h_ndp_co_grep);
    
    cls_register_cxx_method(h_class, "ndp_stat64",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_stat64, &h_ndp_stat64);
    
    cls_register_cxx_method(h_class, "ndp_stat32",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_stat32, &h_ndp_stat32);

    cls_register_cxx_method(h_class, "ndp_bitmap",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_bitmap, &h_ndp_bitmap);
}