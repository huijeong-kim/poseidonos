#include <gtest/gtest.h>

#include "src/allocator/context_manager/context_manager.h"
#include "src/allocator/context_manager/rebuild_ctx/rebuild_ctx.h"
#include "src/meta_file_intf/mock_file_intf.h"

#include "test/integration-tests/allocator/allocator_it_common.h"

#include "test/unit-tests/allocator/address/allocator_address_info_mock.h"
#include "test/unit-tests/allocator/context_manager/context_replayer_mock.h"
#include "test/unit-tests/allocator/context_manager/file_io_manager_mock.h"
#include "test/unit-tests/allocator/context_manager/allocator_ctx/allocator_ctx_mock.h"
#include "test/unit-tests/allocator/context_manager/allocator_ctx/segment_lock_mock.h"
#include "test/unit-tests/allocator/context_manager/allocator_ctx/segment_states_mock.h"
#include "test/unit-tests/allocator/context_manager/segment_ctx/segment_ctx_mock.h"
#include "test/unit-tests/allocator/context_manager/wbstripe_ctx/wbstripe_ctx_mock.h"
#include "test/unit-tests/lib/bitmap_mock.h"
#include "test/unit-tests/meta_file_intf/meta_file_intf_mock.h"
#include "test/unit-tests/telemetry/telemetry_client/telemetry_publisher_mock.h"

#include <thread>

using namespace ::testing;

namespace pos
{

TEST(ContextManagerIntegrationTest, GetRebuildTargetSegment_FreeUserDataSegment)
{
    // Constant
    const int TEST_SEG_CNT = 1;
    const int TEST_TRIAL = 100;

    // AllocatorAddressInfo (Mock)
    NiceMock<MockAllocatorAddressInfo>* allocatorAddressInfo = new NiceMock<MockAllocatorAddressInfo>();
    EXPECT_CALL(*allocatorAddressInfo, GetblksPerStripe).WillRepeatedly(Return(BLK_PER_STRIPE));
    EXPECT_CALL(*allocatorAddressInfo, GetchunksPerStripe).WillRepeatedly(Return(CHUNK_PER_STRIPE));
    EXPECT_CALL(*allocatorAddressInfo, GetnumWbStripes).WillRepeatedly(Return(WB_STRIPE));
    EXPECT_CALL(*allocatorAddressInfo, GetnumUserAreaStripes).WillRepeatedly(Return(USER_STRIPE));
    EXPECT_CALL(*allocatorAddressInfo, GetblksPerSegment).WillRepeatedly(Return(USER_BLOCKS));
    EXPECT_CALL(*allocatorAddressInfo, GetstripesPerSegment).WillRepeatedly(Return(STRIPE_PER_SEGMENT));
    EXPECT_CALL(*allocatorAddressInfo, GetnumUserAreaSegments).WillRepeatedly(Return(TEST_SEG_CNT));
    EXPECT_CALL(*allocatorAddressInfo, IsUT).WillRepeatedly(Return(true));

    // SegmentCtx (Mock)
    NiceMock<MockSegmentCtx>* segmentCtx = new NiceMock<MockSegmentCtx>();
    std::mutex segCtxLock;
    EXPECT_CALL(*segmentCtx, GetSegmentCtxLock).WillRepeatedly(ReturnRef(segCtxLock));
    EXPECT_CALL(*segmentCtx, GetOccupiedStripeCount).WillRepeatedly(Return(STRIPE_PER_SEGMENT));

    // WbStripeCtx (Mock)
    NiceMock<MockWbStripeCtx>* wbStripeCtx = new NiceMock<MockWbStripeCtx>();
    std::mutex allocWbLsidbitMapLock;
    EXPECT_CALL(*wbStripeCtx, GetAllocWbLsidBitmapLock).WillRepeatedly(ReturnRef(allocWbLsidbitMapLock));

    // ContextReplayer (Mock)
    NiceMock<MockContextReplayer>* contextReplayer = new NiceMock<MockContextReplayer>();

    // TelemetryPublisher (Mock)
    NiceMock<MockTelemetryPublisher>* telemetryPublisher = new NiceMock<MockTelemetryPublisher>();

    // AllocatorFileIoManager (Real)
    MockFileIntf* file = new MockFileIntf("RebuildContext", ARRAY_NAME);    // "RebuildContext" file
    MetaFileIntf* fileArr[NUM_FILES];
    for (int i = 0; i < NUM_FILES; ++i)
    {
        fileArr[i] = new NiceMock<MockMetaFileIntf>("fileName", ARRAY_NAME);
        if (i == REBUILD_CTX)
        {
            fileArr[REBUILD_CTX] = file;
        }
    }    
    AllocatorFileIoManager* allocatorFileIoManager = new AllocatorFileIoManager(fileArr, allocatorAddressInfo);

    // AllocatorCtx (Real)
    NiceMock<MockBitMapMutex>* allocSegBitmap = new NiceMock<MockBitMapMutex>();
    NiceMock<MockSegmentStates>* segmentStates = new NiceMock<MockSegmentStates>();
    Mock::AllowLeak(segmentStates);
    EXPECT_CALL(*segmentStates, GetState).WillRepeatedly(Return(SegmentState::SSD));
    NiceMock<MockSegmentLock>* segmentLocks = new NiceMock<MockSegmentLock>();
    Mock::AllowLeak(segmentLocks);
    std::mutex segLock;
    EXPECT_CALL(*segmentLocks, GetLock).WillRepeatedly(ReturnRef(segLock));
    AllocatorCtx* allocatorCtx = new AllocatorCtx(nullptr, allocSegBitmap, segmentStates, segmentLocks, allocatorAddressInfo);

    // RebuildCtx (Real)
    RebuildCtx* rebuildCtx = new RebuildCtx(allocatorCtx, allocatorAddressInfo);

    // ContextManager (Real)
    ContextManager contextManager(telemetryPublisher, allocatorCtx, segmentCtx, rebuildCtx,
                                  wbStripeCtx, allocatorFileIoManager, 
                                  contextReplayer, false, allocatorAddressInfo, ARRAY_NAME);

    // Prepare Test
    contextManager.Init();
    rebuildCtx->EmplaceRebuildTargetSegment(0);
    rebuildCtx->SetTargetSegmentCnt(TEST_SEG_CNT);

    // Start Test
    for (int i = 0; i < TEST_TRIAL; ++i)
    {
        int nanoSec = std::rand() % 100;
        std::thread th1(&RebuildCtx::GetRebuildTargetSegment, rebuildCtx);
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSec));
        std::thread th2(&ContextManager::FreeUserDataSegment, &contextManager, 0);
        th1.join();
        th2.join();

        std::thread th3(&ContextManager::FreeUserDataSegment, &contextManager, 0);
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSec));
        std::thread th4(&RebuildCtx::GetRebuildTargetSegment, rebuildCtx);
        th3.join();
        th4.join();
    }

    // Clean up
    delete allocatorAddressInfo;
}

}   // namespace pos
