#include <vector>

#include "src/mapper/reversemap/reverse_map_io.h"
#include "src/mapper/reversemap/reverse_map.h"
#include "src/meta_file_intf/meta_file_intf.h"
#include "src/event_scheduler/event_scheduler.h"

namespace pos
{
ReverseMapIo::ReverseMapIo(ReverseMapPack* pack, EventSmartPtr clientCallback,
    MetaFileIntf* file, uint64_t offset, IoDirection direction,
    TelemetryPublisher* tp, EventScheduler* es,
    std::function<void(ReverseMapIo*)> notify)
: revMapPack(pack),
  ioError(0),
  ioDirection(direction),
  fileOffset(offset),
  revMapFile(file),
  mfsAsyncIoDonePages(0),
  mapFlushState(MapFlushState::FLUSH_DONE),
  callback(clientCallback),
  issuedIoCnt(0),
  totalIoCnt(0),
  notifyIoDone(notify),
  telemetryPublisher(tp),
  eventSchceduler(es)
{
}

int
ReverseMapIo::Load(void)
{
    mapFlushState = MapFlushState::FLUSHING; // TODO: use load enum

    uint64_t offset = fileOffset;
    uint64_t pageNum = 0;
    
    std::vector<ReverseMapPage> revMapPages = revMapPack->GetReverseMapPages();
    totalIoCnt = revMapPages.size();
    for (auto& page : revMapPages)
    {
        RevMapPageAsyncIoCtx* revMapPageAsyncIoReq = new RevMapPageAsyncIoCtx();
        revMapPageAsyncIoReq->opcode = MetaFsIoOpcode::Read;
        revMapPageAsyncIoReq->fd = revMapFile->GetFd();
        revMapPageAsyncIoReq->fileOffset = offset;
        revMapPageAsyncIoReq->length = page.length;;
        revMapPageAsyncIoReq->buffer = page.buffer;
        revMapPageAsyncIoReq->callback = std::bind(&ReverseMapIo::_RevMapPageIoDone, this, std::placeholders::_1);
        revMapPageAsyncIoReq->mpageNum = pageNum++;
        revMapPageAsyncIoReq->vsid = revMapPack->GetVsid();

        int ret = revMapFile->AsyncIO(revMapPageAsyncIoReq);
        if (ret < 0)
        {
            POS_TRACE_ERROR(EID(MFS_ASYNCIO_ERROR), revMapPageAsyncIoReq->ToString());

            ioError = ret;
            mapFlushState = MapFlushState::FLUSH_DONE;
            callback = nullptr;
            break;
        }

        offset += page.length;
        issuedIoCnt++;
    }
    return ioError;
}

int
ReverseMapIo::Flush(void)
{
    mapFlushState = MapFlushState::FLUSHING;

    uint64_t offset = fileOffset;
    uint64_t pageNum = 0;

    std::vector<ReverseMapPage> revMapPages = revMapPack->GetReverseMapPages();
    totalIoCnt = revMapPages.size();
    for (auto& page : revMapPages)
    {
        RevMapPageAsyncIoCtx* revMapPageAsyncIoReq = new RevMapPageAsyncIoCtx();
        revMapPageAsyncIoReq->opcode = MetaFsIoOpcode::Write;
        revMapPageAsyncIoReq->fd = revMapFile->GetFd();
        revMapPageAsyncIoReq->fileOffset = offset;
        revMapPageAsyncIoReq->length = page.length;
        revMapPageAsyncIoReq->buffer = page.buffer;
        revMapPageAsyncIoReq->callback = std::bind(&ReverseMapIo::_RevMapPageIoDone,
            this, std::placeholders::_1);
        revMapPageAsyncIoReq->mpageNum = pageNum++;
        revMapPageAsyncIoReq->vsid = revMapPack->GetVsid();

        int ret = revMapFile->AsyncIO(revMapPageAsyncIoReq);
        if (ret < 0)
        {
            POS_TRACE_ERROR(EID(MFS_ASYNCIO_ERROR),revMapPageAsyncIoReq->ToString());

            ioError = ret;
            mapFlushState = MapFlushState::FLUSH_DONE;
            callback = nullptr;
            break;
        }

        offset += page.length;
        issuedIoCnt++;
    }
    /* TODO: telemetryPublisher can be used after it is deleted due to I/o done
    if (telemetryPublisher)
    {
        POSMetric metric(TEL33011_MAP_REVERSE_FLUSH_IO_ISSUED_CNT, POSMetricTypes::MT_COUNT);
        metric.SetCountValue(issuedIoCnt);
        telemetryPublisher->PublishMetric(metric);
    }*/

    return ioError;
}

void
ReverseMapIo::_RevMapPageIoDone(AsyncMetaFileIoCtx* ctx)
{
    RevMapPageAsyncIoCtx* revMapPageAsyncIoReq = static_cast<RevMapPageAsyncIoCtx*>(ctx);
    if (revMapPageAsyncIoReq->error != 0)
    {
        ioError = revMapPageAsyncIoReq->error;
        POS_TRACE_ERROR(EID(MFS_ASYNCIO_ERROR),
            "[ReverseMapPack] Error!, MFS AsyncIO error, ioError:{} mpageNum:{}", ioError, revMapPageAsyncIoReq->mpageNum);
    }

    if (telemetryPublisher)
    {
        POSMetric metric(TEL33012_MAP_REVERSE_FLUSH_IO_DONE_CNT, POSMetricTypes::MT_COUNT);
        metric.SetCountValue(1);
        telemetryPublisher->PublishMetric(metric);
    }

    uint32_t res = mfsAsyncIoDonePages.fetch_add(1);

    if ((ioDirection == IO_LOAD) && (revMapPageAsyncIoReq->mpageNum == 0))
    {
        int ret = revMapPack->HeaderLoaded();
        if (ret < 0)
        {
            POS_TRACE_ERROR(EID(REVMAP_IO_ERROR), revMapPageAsyncIoReq->ToString());
            assert(false);
        }
    }

    // TODO: handle issue fail case - Should handle zombie request
    if ((res + 1) == totalIoCnt)
    {
        mapFlushState = MapFlushState::FLUSH_DONE;

        if (callback != nullptr)
        {
            EventSmartPtr enqueue = callback;
            callback = nullptr;
            eventSchceduler->EnqueueEvent(enqueue);
        }

        notifyIoDone(this);
    }

    // NOTE that ReverseMapPack should not access to its private variables 
    // after enqueueing callback event as callback might delete this ReverseMapPack
}

void
ReverseMapIo::WaitPendingIoDone(void)
{
    while (mapFlushState == MapFlushState::FLUSHING)
    {
    }
}

} // namespace pos
