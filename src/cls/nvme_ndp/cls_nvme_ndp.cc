#include <cerrno>

#include "objclass/objclass.h"
#include "cls/nvme_ndp/cls_nvme_ndp_ops.h"
//#include "spdk/include/spdk/nvme.h"
//#include "spdk/include/spdk/env.h"

CLS_VER(1,0)
CLS_NAME(nvme_ndp)

namespace rados::cls::nvme_ndp{
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
        
        int rc = spdk_initialize();  // init environment for spdk
        if (rc != 0) {
            CLS_LOG(5, "Failed to initialize spdk env");
            return -1;
        }

        struct spdk_nvme_cmd cmd = {};
        
        uint64_t buf_size = 4096;
        void *buf = spdk_dma_zmalloc(buf_size, 4096, nullptr);
        if(!buf) {
            CLS_ERR("Falied to allocate DMA buffer");
            return -ENOMEM;
        }
        cmd.opc = op.opc;
        cmd.nsid = spdk_nvme_ctrlr_get_first_active_ns(g_ctrlr);
        cmd.dptr.prp.prp1 = spdk_vtophys(buf, &buf_size);
        if (cmd.dptr.prp.prp1 == SPDK_VTOPHYS_ERROR) {
            CLS_ERR("Failed to translate virtual address to physical");
            rc = -1;
            goto cleanup;
        }

        rc = spdk_nvme_ctrlr_cmd_io_raw(g_ctrlr, g_qpair, &cmd, buf, 4096, nvme_cb, buf);
        if (rc != 0) {
            CLS_ERR("spdk_nvme_ctrlr_cmd_io_raw falied: %d", rc);
            rc = -EIO;
            goto cleanup;
        }

        while(spdk_nvme_qpair_process_completions(g_qpair, 0) == 0){
            
        }
        CLS_LOG(5, "nvmd_ndp command has been completed");
        return 0;
    cleanup:
        spdk_dma_free(buf);
        return rc;
    }
    */

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
}

CLS_INIT(nvme_ndp)
{
    using namespace::rados::cls::nvme_ndp;
    CLS_LOG(0, "Loading nvme_ndp class!");

    cls_handle_t h_class;
    // cls_method_handle_t h_nvme_send_task;
    cls_method_handle_t h_ndp_grep;
    cls_method_handle_t h_ndp_stat64;
    cls_method_handle_t h_ndp_stat32;

    cls_register("nvme_ndp", &h_class);
    /*
    cls_register_cxx_method(h_class, "nvme_send_task",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            nvme_send_task, &h_nvme_send_task);
    */

    cls_register_cxx_method(h_class, "ndp_grep",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_grep, &h_ndp_grep);
    
    cls_register_cxx_method(h_class, "ndp_stat64",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_stat64, &h_ndp_stat64);
    
    cls_register_cxx_method(h_class, "ndp_stat32",
                            CLS_METHOD_RD | CLS_METHOD_WR,
                            ndp_stat32, &h_ndp_stat32);
}