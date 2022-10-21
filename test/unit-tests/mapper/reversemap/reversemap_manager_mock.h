#include <gmock/gmock.h>

#include <list>
#include <string>
#include <vector>

#include "src/mapper/reversemap/reversemap_manager.h"

namespace pos
{
class MockReverseMapManager : public ReverseMapManager
{
public:
    using ReverseMapManager::ReverseMapManager;
    MOCK_METHOD(void, Init, (), (override));
    MOCK_METHOD(void, Dispose, (), (override));
    MOCK_METHOD(int, Load, (ReverseMapPack * rev, EventSmartPtr cb), (override));
    MOCK_METHOD(int, Flush, (ReverseMapPack * rev, EventSmartPtr cb), (override));
    MOCK_METHOD(int, Flush, (StripeId wblsid, EventSmartPtr cb), (override));
    MOCK_METHOD(int, UpdateReverseMapEntry, (ReverseMapPack * rev, uint64_t offset, BlkAddr rba, uint32_t volumeId), (override));
    MOCK_METHOD(int, UpdateReverseMapEntry, (StripeId wblsid, uint64_t offset, BlkAddr rba, uint32_t volumeId), (override));
    MOCK_METHOD((std::tuple<BlkAddr, uint32_t>), GetReverseMapEntry, (ReverseMapPack * rev, uint64_t offset), (override));
    MOCK_METHOD((std::tuple<BlkAddr, uint32_t>), GetReverseMapEntry, (StripeId wblsid, uint64_t offset), (override));
    MOCK_METHOD(void, Assign, (StripeId wblsid, StripeId vsid), (override));
    MOCK_METHOD(ReverseMapPack*, AllocReverseMapPack, (StripeId vsid, StripeId wblsid), (override));
    MOCK_METHOD(int, ReconstructReverseMap, (uint32_t volumeId, uint64_t totalRba, uint32_t wblsid, uint32_t vsid, uint64_t blockCount, (std::map<uint64_t, BlkAddr> revMapInfos)), (override));
    MOCK_METHOD(uint64_t, GetReverseMapPerStripeFileSize, (), (override));
    MOCK_METHOD(uint64_t, GetWholeReverseMapFileSize, (), (override));
    MOCK_METHOD(int, LoadReverseMapForWBT, (MetaFileIntf * fileLinux, uint64_t offset, uint64_t fileSize, char* buf), (override));
    MOCK_METHOD(int, StoreReverseMapForWBT, (MetaFileIntf * fileLinux, uint64_t offset, uint64_t fileSize, char* buf), (override));
    MOCK_METHOD(char*, GetReverseMapPtrForWBT, (), (override));
    MOCK_METHOD(void, WaitAllPendingIoDone, (), (override));
};

} // namespace pos
