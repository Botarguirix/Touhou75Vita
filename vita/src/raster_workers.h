#pragma once
#include "d3d8_raster_executor.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/cpu.h>
#include <psp2/kernel/processmgr.h>
#include <condition_variable>
#include <mutex>
#include <cstdio>
#include <cstring>

// Main/core 0 retains the only x86 CPU/JIT. Workers on user cores 1 and 2
// touch immutable texture bytes and disjoint destination rows, never the VM.
// Sleep between draws; lower priority than PCM output and the watchdog.
class RasterWorkers {
public:
    explicit RasterWorkers(FILE* log):log_(log){}
    ~RasterWorkers(){shutdown();}
    RasterWorkers(const RasterWorkers&)=delete;
    RasterWorkers& operator=(const RasterWorkers&)=delete;
    static d3d8_quad::Result execute(void* context,const d3d8_quad::Vertex (&v)[4],
        const d3d8_quad::Image& source,const d3d8_quad::Target& target,
        const d3d8_quad::Viewport& vp,const d3d8_quad::Settings& requested,
        d3d8_quad::Stats& stats) {
        return static_cast<RasterWorkers*>(context)->draw(v,source,target,vp,requested,stats);
    }
    void shutdown() {
        for(auto& worker:workers_)if(worker.started) {
            {std::lock_guard<std::mutex> lock(worker.mutex);worker.quit=true;}
            worker.wake.notify_one();
        }
        for(auto& worker:workers_)if(worker.id>=0) {
            if(worker.started) {
                const int rc=sceKernelWaitThreadEnd(worker.id,nullptr,nullptr);
                fprintf(log_,"startup_raster_worker_join=core:%u rc:0x%08X jobs:%llu\n",
                    worker.core,unsigned(rc),(unsigned long long)worker.jobs);
                if(rc<0)sceKernelExitProcess(1); // Cannot free memory still used by a worker.
            }
            const int rc=sceKernelDeleteThread(worker.id);
            fprintf(log_,"startup_raster_worker_delete=core:%u rc:0x%08X\n",worker.core,unsigned(rc));
            if(rc<0)sceKernelExitProcess(1);
            worker.id=-1;worker.started=false;
        }
        ready_=false;
    }
    void report()const {
        fprintf(log_,"startup_raster_parallel_summary=draws:%llu serial:%llu configured_workers:2 threshold_pixels:32768 pixel_hashes:disabled gpu:no scope:synchronous_disjoint_rows\n",
            (unsigned long long)parallel_,(unsigned long long)serial_);
    }
private:
    struct Job {
        d3d8_quad::Vertex vertices[4]{};
        d3d8_quad::Image source{};d3d8_quad::Target target{};
        d3d8_quad::Viewport viewport{};d3d8_quad::Settings settings{};
    };
    struct Worker {
        SceUID id=-1;unsigned core=0;bool started=false,initialized=false,pending=false,quit=false;
        int affinity=-1,cpu=-1;
        std::mutex mutex;std::condition_variable wake,done;
        Job job{};d3d8_quad::Stats stats{};
        d3d8_quad::Result result=d3d8_quad::Result::Unsupported;
        uint64_t jobs=0;
    };
    static int entry(SceSize size,void* arg) {
        if(size!=sizeof(Worker*))return -1;
        Worker* worker=nullptr;std::memcpy(&worker,arg,sizeof(worker));
        if(!worker)return -1;
        std::unique_lock<std::mutex> lock(worker->mutex);
        worker->affinity=sceKernelGetThreadCpuAffinityMask(sceKernelGetThreadId());
        worker->cpu=sceKernelGetCpuId();worker->initialized=true;worker->done.notify_one();
        while(true) {
            worker->wake.wait(lock,[&]{return worker->pending || worker->quit;});
            if(worker->quit)return 0;
            const Job job=worker->job;lock.unlock();
            d3d8_quad::Stats stats{};
            const auto result=d3d8_quad::rasterize(job.vertices,job.source,job.target,job.viewport,job.settings,stats);
            lock.lock();worker->stats=stats;worker->result=result;
            ++worker->jobs;worker->pending=false;worker->done.notify_one();
        }
    }
    bool initialize() {
        if(attempted_)return ready_;
        attempted_=true;
        for(unsigned i=0;i<2;++i) {
            auto& worker=workers_[i];worker.core=i+1;
            const int mask=i?SCE_KERNEL_CPU_MASK_USER_2:SCE_KERNEL_CPU_MASK_USER_1;
            worker.id=sceKernelCreateThread(i?"th075_raster_core2":"th075_raster_core1",
                entry,0x10000110,128*1024,0,mask,nullptr);
            if(worker.id<0){fprintf(log_,"startup_raster_worker_create=failed core:%u rc:0x%08X fallback:serial\n",worker.core,unsigned(worker.id));shutdown();return false;}
            auto* arg=&worker;
            const int started=sceKernelStartThread(worker.id,sizeof(arg),&arg);
            if(started<0){fprintf(log_,"startup_raster_worker_start=failed core:%u rc:0x%08X fallback:serial\n",worker.core,unsigned(started));shutdown();return false;}
            worker.started=true;
            std::unique_lock<std::mutex> lock(worker.mutex);
            worker.done.wait(lock,[&]{return worker.initialized;});
            const bool matches=worker.affinity==mask && worker.cpu==int(worker.core);
            fprintf(log_,"startup_raster_worker=core:%u affinity:0x%08X actual_cpu:%d priority:0x10000110 stack:131072 verified:%s\n",
                worker.core,unsigned(worker.affinity),worker.cpu,matches?"yes":"no");
            lock.unlock();
            if(!matches){shutdown();return false;}
        }
        ready_=true;return true;
    }
    d3d8_quad::Result draw(const d3d8_quad::Vertex (&v)[4],const d3d8_quad::Image& source,
        const d3d8_quad::Target& target,const d3d8_quad::Viewport& vp,
        d3d8_quad::Settings settings,d3d8_quad::Stats& stats) {
        settings.hash_pixels=false;
        // Complete common and convexity validation before any worker can write.
        const auto preflight=d3d8_quad::rasterize(v,source,target,vp,settings,stats,true);
        if(preflight!=d3d8_quad::Result::Rendered)return preflight;
        const auto bands=d3d8_quad::row_bands(v,vp);
        if(d3d8_quad::clipped_box_pixels(v,vp)<32768 ||
            !bands[0].height || !bands[1].height || !bands[2].height || !initialize()) {
            ++serial_;return d3d8_quad::rasterize(v,source,target,vp,settings,stats);
        }
        for(unsigned i=0;i<2;++i) {
            auto& worker=workers_[i];
            {std::lock_guard<std::mutex> lock(worker.mutex);
                std::memcpy(worker.job.vertices,v,sizeof(worker.job.vertices));
                worker.job.source=source;worker.job.target=target;worker.job.viewport=bands[i];
                worker.job.settings=settings;worker.pending=true;}
            worker.wake.notify_one();
        }
        std::array<d3d8_quad::Stats,3> parts{};
        const auto main_result=d3d8_quad::rasterize(v,source,target,bands[2],settings,parts[2]);
        bool good=main_result==d3d8_quad::Result::Rendered;
        for(unsigned i=0;i<2;++i) {
            auto& worker=workers_[i];std::unique_lock<std::mutex> lock(worker.mutex);
            worker.done.wait(lock,[&]{return !worker.pending;});
            good=good && worker.result==d3d8_quad::Result::Rendered;parts[i]=worker.stats;
        }
        if(!good){fprintf(log_,"startup_raster_parallel=postvalidation_failure execution:refused\n");sceKernelExitProcess(1);}
        stats=d3d8_quad::combine_rows(parts);++parallel_;
        return d3d8_quad::Result::Rendered;
    }
    FILE* log_;std::array<Worker,2> workers_{};
    bool attempted_=false,ready_=false;uint64_t parallel_=0,serial_=0;
};
