#include "src/allocator/block_manager/block_manager.h"

#include <gtest/gtest.h>

#include "src/allocator/address/allocator_address_info.h"
#include "src/include/address_type.h"
#include "test/unit-tests/allocator/address/allocator_address_info_mock.h"
#include "test/unit-tests/allocator/block_manager/block_manager_spy.h"
#include "test/unit-tests/allocator/context_manager/allocator_ctx/allocator_ctx_mock.h"
#include "test/unit-tests/allocator/context_manager/context_manager_mock.h"
#include "test/unit-tests/allocator/context_manager/rebuild_ctx/rebuild_ctx_mock.h"
#include "test/unit-tests/allocator/context_manager/segment_ctx/segment_ctx_mock.h"
#include "test/unit-tests/allocator/context_manager/wbstripe_ctx/wbstripe_ctx_mock.h"
#include "test/unit-tests/allocator/i_wbstripe_internal_mock.h"
#include "test/unit-tests/allocator/stripe/stripe_mock.h"
#include "test/unit-tests/mapper/i_stripemap_mock.h"
#include "test/unit-tests/mapper/reversemap/reverse_map_mock.h"
#include "test/unit-tests/mapper/reversemap/reversemap_manager_mock.h"
#include "test/unit-tests/volume/i_volume_manager_mock.h"

using ::testing::_;
using ::testing::AtLeast;
using testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
namespace pos
{
TEST(BlockManager, Init_TestSimpleFunc)
{
    // given
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    BlockManager blkManager;
    // when
    blkManager.Init(iWbstripe);
    delete iWbstripe;
}

TEST(BlockManager, AllocateWriteBufferBlks_TestFunc)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    BlockManager blkManager(nullptr, nullptr, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbCtxLock;
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbCtxLock));
    // when 1.
    VirtualBlks ret = blkManager.AllocateWriteBufferBlks(0, 1);
    // then 1.
    EXPECT_EQ(1, ret.numBlks);
    // given 2.
    blkManager.ProhibitUserBlkAlloc();
    // when 2.
    ret = blkManager.AllocateWriteBufferBlks(0, 1);
    // then 2.
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);
    // given 3.
    blkManager.BlockAllocating(0);
    // when 3.
    ret = blkManager.AllocateWriteBufferBlks(0, 1);
    // then 3.
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);
    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
}

TEST(BlockManager, AllocateGcDestStripe_TestFunc)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    BlockManager blkManager(nullptr, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    NiceMock<MockReverseMapPack>* revMapPack = new NiceMock<MockReverseMapPack>();
    std::mutex Lock;
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(Lock));
    EXPECT_CALL(*reverseMap, AllocReverseMapPack).WillOnce(Return(revMapPack));
    EXPECT_CALL(*revMapPack, LinkVsid).WillOnce(Return(0));
    // when
    blkManager.AllocateGcDestStripe(0);
    // given 2.
    blkManager.BlockAllocating(0);
    // when 2.
    blkManager.AllocateGcDestStripe(0);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete revMapPack;
}

TEST(BlockManager, AllocateGcDestStripe_TestFuncFailCase1)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    BlockManager blkManager(nullptr, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    NiceMock<MockReverseMapPack>* revMapPack = new NiceMock<MockReverseMapPack>();
    std::mutex Lock;
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(Lock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(20));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_URGENT_GC));
    EXPECT_CALL(*ctxManager, AllocateFreeSegment).WillOnce(Return(UNMAP_SEGMENT));
    EXPECT_CALL(*reCtx, IsRebuidTargetSegmentsEmpty).WillOnce(Return(false));
    // when
    Stripe* ret = blkManager.AllocateGcDestStripe(0);
    // then
    EXPECT_EQ(nullptr, ret);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete revMapPack;
    delete reCtx;
}

TEST(BlockManager, AllocateGcDestStripe_TestFuncFailCase2)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    BlockManager blkManager(nullptr, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    NiceMock<MockReverseMapPack>* revMapPack = new NiceMock<MockReverseMapPack>();
    std::mutex Lock;
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(Lock));
    EXPECT_CALL(*reverseMap, AllocReverseMapPack).WillOnce(Return(revMapPack));
    EXPECT_CALL(*revMapPack, LinkVsid).WillOnce(Return(-1));
    // when
    blkManager.AllocateGcDestStripe(0);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete revMapPack;
}

TEST(BlockManager, InvalidateBlks_TestSimpleSetter)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    BlockManager blkManager(nullptr, nullptr, segCtx, nullptr, nullptr, &addrInfo, ctxManager, 0);
    VirtualBlkAddr vsa = {.stripeId = 0, .offset = 0};
    VirtualBlks blks = {.startVsa = vsa, .numBlks = 1};
    // given 1.
    EXPECT_CALL(*segCtx, DecreaseValidBlockCount).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, FreeUserDataSegment).Times(1);
    // when 1.
    blkManager.InvalidateBlks(blks);
    // given 2.
    EXPECT_CALL(*segCtx, DecreaseValidBlockCount).WillOnce(Return(1));
    // when 2.
    blkManager.InvalidateBlks(blks);
    delete segCtx;
    delete ctxManager;
}

TEST(BlockManager, ValidateBlks_TestSimpleSetter)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    BlockManager blkManager(nullptr, nullptr, segCtx, nullptr, nullptr, &addrInfo, nullptr, 0);
    // given
    VirtualBlkAddr vsa = {.stripeId = 0, .offset = 0};
    VirtualBlks blks = {.startVsa = vsa, .numBlks = 1};
    EXPECT_CALL(*segCtx, IncreaseValidBlockCount).Times(1);
    // when
    blkManager.ValidateBlks(blks);
    delete segCtx;
}

TEST(BlockManager, ProhibitUserBlkAlloc_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    // when
    blkManager.ProhibitUserBlkAlloc();
}

TEST(BlockManager, PermitUserBlkAlloc_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    // when
    blkManager.PermitUserBlkAlloc();
}

TEST(BlockManager, BlockAllocating_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    // when
    blkManager.BlockAllocating(0);
}

TEST(BlockManager, UnblockAllocating_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    // when
    blkManager.UnblockAllocating(0);
}

TEST(BlockManager, TurnOffBlkAllocation_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    blkManager.TurnOnBlkAllocation();
    // when
    blkManager.TurnOffBlkAllocation();
}

TEST(BlockManager, TurnOnBlkAllocation_TestSimpleSetter)
{
    // given
    BlockManager blkManager;
    // when
    blkManager.TurnOnBlkAllocation();
}

TEST(BlockManager, _AllocateBlks_TestCase1)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};
    // given 0.
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(UNMAP_STRIPE));
    // when 0.
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    // then 0.
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);
    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase2)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};

    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(ctxLock)).WillOnce(ReturnRef(ctxLock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(10));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_URGENT_GC));
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    // then
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);
    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase3_1)
{
    // given
    NiceMock<MockAllocatorAddressInfo>* addrInfo = new NiceMock<MockAllocatorAddressInfo>();
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};
    // given 2.
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(ctxLock)).WillOnce(ReturnRef(ctxLock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(10));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_NO_GC));
    EXPECT_CALL(*ctxManager, AllocateFreeSegment).WillOnce(Return(UNMAP_SEGMENT));
    EXPECT_CALL(*reCtx, IsRebuidTargetSegmentsEmpty).WillOnce(Return(true));
    EXPECT_CALL(*addrInfo, IsUT).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(*addrInfo, GetstripesPerSegment).WillOnce(Return(10));
    EXPECT_CALL(*addrInfo, GetblksPerStripe).WillOnce(Return(5));
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    // then
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
    delete addrInfo;
}

TEST(BlockManager, _AllocateBlks_TestCase3_2)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};
    // given 2.
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(ctxLock)).WillOnce(ReturnRef(ctxLock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(10));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_NO_GC));
    EXPECT_CALL(*ctxManager, AllocateFreeSegment).WillOnce(Return(UNMAP_SEGMENT));
    EXPECT_CALL(*reCtx, IsRebuidTargetSegmentsEmpty).WillOnce(Return(false));
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    // then
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase4)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};
    NiceMock<MockReverseMapPack>* revMapPack = new NiceMock<MockReverseMapPack>();
    NiceMock<MockStripe>* stripe = new NiceMock<MockStripe>();

    // given
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(ctxLock)).WillOnce(ReturnRef(ctxLock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(10));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_NO_GC));
    EXPECT_CALL(*ctxManager, AllocateFreeSegment).WillOnce(Return(0));

    EXPECT_CALL(*allocCtx, SetSegmentState).Times(1);
    EXPECT_CALL(*iWbstripe, GetStripe).WillOnce(Return(stripe));
    EXPECT_CALL(*stripe, Assign).Times(1);
    EXPECT_CALL(*reverseMap, GetReverseMapPack).WillOnce(Return(revMapPack));
    EXPECT_CALL(*stripe, LinkReverseMap).WillOnce(Return(-1));
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    // then
    EXPECT_EQ(UNMAP_VSA, ret.startVsa);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete revMapPack;
    delete iStripeMap;
    delete stripe;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase5)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 5};
    NiceMock<MockStripe>* stripe = new NiceMock<MockStripe>();
    NiceMock<MockReverseMapPack>* revMapPack = new NiceMock<MockReverseMapPack>();
    // given 4.
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, AllocFreeWbStripe).WillOnce(Return(0));
    EXPECT_CALL(*ctxManager, GetCtxLock).WillOnce(ReturnRef(ctxLock));
    EXPECT_CALL(*ctxManager, GetRebuildCtx).WillOnce(Return(reCtx));
    EXPECT_CALL(*allocCtx, UpdatePrevLsid).WillOnce(Return(10));
    EXPECT_CALL(*ctxManager, GetCurrentGcMode).WillOnce(Return(MODE_NO_GC));
    EXPECT_CALL(*ctxManager, AllocateFreeSegment).WillOnce(Return(0));

    EXPECT_CALL(*allocCtx, SetSegmentState).Times(1);
    EXPECT_CALL(*iWbstripe, GetStripe).WillOnce(Return(stripe));
    EXPECT_CALL(*stripe, Assign).Times(1);
    EXPECT_CALL(*reverseMap, GetReverseMapPack).WillOnce(Return(revMapPack));
    EXPECT_CALL(*stripe, LinkReverseMap).WillOnce(Return(0));
    EXPECT_CALL(*iStripeMap, SetLSA).Times(1);

    EXPECT_CALL(*wbCtx, SetActiveStripeTail).Times(1);
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete revMapPack;
    delete iStripeMap;
    delete stripe;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase6)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    std::mutex wbLock, ctxLock;
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 3};
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, SetActiveStripeTail).Times(1);
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);
    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase7)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 3};
    std::mutex wbLock, ctxLock;
    // given
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, SetActiveStripeTail).Times(1);
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 1);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateBlks_TestCase8)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 3};
    std::mutex wbLock, ctxLock;
    // given
    EXPECT_CALL(*wbCtx, GetActiveStripeTailLock).WillOnce(ReturnRef(wbLock));
    EXPECT_CALL(*wbCtx, GetActiveStripeTail).WillOnce(Return(vsa));
    EXPECT_CALL(*wbCtx, SetActiveStripeTail).Times(1);
    // when
    VirtualBlks ret = blkManager._AllocateBlks(0, 11);
    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateWriteBufferBlksFromNewStripe_TestInternalFunc)
{
    // given
    AllocatorAddressInfo addrInfo;
    addrInfo.SetstripesPerSegment(10);
    addrInfo.SetblksPerStripe(5);
    NiceMock<MockIWBStripeInternal>* iWbstripe = new NiceMock<MockIWBStripeInternal>();
    NiceMock<MockAllocatorCtx>* allocCtx = new NiceMock<MockAllocatorCtx>();
    NiceMock<MockWbStripeCtx>* wbCtx = new NiceMock<MockWbStripeCtx>();
    NiceMock<MockSegmentCtx>* segCtx = new NiceMock<MockSegmentCtx>();
    NiceMock<MockRebuildCtx>* reCtx = new NiceMock<MockRebuildCtx>();
    NiceMock<MockContextManager>* ctxManager = new NiceMock<MockContextManager>();
    NiceMock<MockReverseMapManager>* reverseMap = new NiceMock<MockReverseMapManager>();
    NiceMock<MockIStripeMap>* iStripeMap = new NiceMock<MockIStripeMap>();
    BlockManagerSpy blkManager(iStripeMap, reverseMap, segCtx, allocCtx, wbCtx, &addrInfo, ctxManager, 0);
    blkManager.Init(iWbstripe);
    VirtualBlkAddr vsa = {.stripeId = 10, .offset = 3};
    std::mutex wbLock, ctxLock;
    // when
    VirtualBlks ret = blkManager._AllocateWriteBufferBlksFromNewStripe(0, 0, 11);
    // then
    EXPECT_EQ(5, ret.numBlks);

    delete iWbstripe;
    delete allocCtx;
    delete wbCtx;
    delete segCtx;
    delete ctxManager;
    delete reverseMap;
    delete iStripeMap;
    delete reCtx;
}

TEST(BlockManager, _AllocateStripe_TestInternalFunc)
{
}

TEST(BlockManager, _RollBackStripeIdAllocation_TestInternalFunc)
{
}

} // namespace pos
