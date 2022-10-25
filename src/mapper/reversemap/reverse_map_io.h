#pragma once

#include <atomic>

#include "src/meta_file_intf/async_context.h"
#include "src/include/smart_ptr_type.h"
#include "src/mapper/include/mapper_const.h"

namespace pos
{

enum IoDirection
{
    IO_FLUSH = 1,
    IO_LOAD,
};

class ReverseMapPack;
class MetaFileIntf;
class TelemetryPublisher;
class EventScheduler;

class ReverseMapIo
{
public:
    ReverseMapIo(ReverseMapPack* pack, EventSmartPtr clientCallback,
        MetaFileIntf* file, uint64_t offset, IoDirection direction,
        TelemetryPublisher* tp, EventScheduler* eventScheduler,
        std::function<void(ReverseMapIo*)> notify);
    virtual ~ReverseMapIo(void) = default;

    virtual int Load(void);
    virtual int Flush(void);
    virtual void WaitPendingIoDone(void);

    uint64_t GetNumIssuedIoCnt(void)
    {
        return issuedIoCnt;
    }
    MapFlushState GetMapFlushState(void)
    {
        return mapFlushState;
    }

private:
    void _RevMapPageIoDone(AsyncMetaFileIoCtx* ctx);
    
    ReverseMapPack* revMapPack;
    int ioError;
    int ioDirection;
    uint64_t fileOffset;
    MetaFileIntf* revMapFile;
    std::atomic<uint32_t> mfsAsyncIoDonePages;
    std::atomic<MapFlushState> mapFlushState;
    EventSmartPtr callback;
    std::atomic<uint64_t> issuedIoCnt;
    uint64_t totalIoCnt;
    std::function<void(ReverseMapIo*)> notifyIoDone;

    TelemetryPublisher* telemetryPublisher;
    EventScheduler* eventSchceduler;
};

using ReverseMapIoPtr = std::shared_ptr<ReverseMapIo>;

class RevMapPageAsyncIoCtx : public AsyncMetaFileIoCtx
{
public:
    void HandleIoComplete(void* data) override {
        if (ioDoneCheckCallback)
            error = ioDoneCheckCallback(data);
        if (callback)
            callback(this);
        delete this;
    }

    int mpageNum;
};
    
} // namespace pos
