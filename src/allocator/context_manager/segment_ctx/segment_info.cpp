/*
 *   BSD LICENSE
 *   Copyright (c) 2021 Samsung Electronics Corporation
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Samsung Electronics Corporation nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "segment_info.h"

#include <cassert>
#include <memory.h>

#include "src/include/pos_event_id.h"
#include "src/logger/logger.h"

namespace pos
{
SegmentInfo::SegmentInfo(void)
: SegmentInfo(0, 0, SegmentState::FREE)
{
}

// Constructor for UT to inject precondition values
SegmentInfo::SegmentInfo(uint32_t blkCount, uint32_t stripeCount, SegmentState segmentState)
: validBlockCount(blkCount),
  occupiedStripeCount(stripeCount),
  state(segmentState),
  arrayId(-1),
  segmentId(0)
{
}

SegmentInfo::~SegmentInfo(void)
{
}

uint32_t
SegmentInfo::GetValidBlockCount(void)
{
    return validBlockCount;
}

void
SegmentInfo::SetValidBlockCount(int cnt)
{
    // for wbt
    validBlockCount = cnt;
}

uint32_t
SegmentInfo::IncreaseValidBlockCount(uint32_t inc)
{
    return validBlockCount.fetch_add(inc) + inc;
}

std::pair<bool, SegmentState>
SegmentInfo::DecreaseValidBlockCount(uint32_t dec, bool allowVictimSegRelease)
{
    std::lock_guard<std::mutex> lock(seglock);
    int32_t decreased = validBlockCount.fetch_sub(dec) - dec;

    if (decreased == 0)
    {
        if (true == allowVictimSegRelease)
        {
            if (state == SegmentState::VICTIM || state == SegmentState::SSD)
            {
                std::pair<bool, SegmentState> result = {true, state};
                _MoveToFreeState();

                return result;
            }
        }
        else
        {
            if (state == SegmentState::SSD)
            {
                std::pair<bool, SegmentState> result = {true, state};
                _MoveToFreeState();

                return result;
            }
        }
    }
    else if (decreased < 0)
    {
        POS_TRACE_ERROR(EID(VALID_COUNT_UNDERFLOWED),
            "Valid block count decreasedCount:{} total validCount:{} segmentId:{} : UNDERFLOWED", dec, decreased, segmentId);
        return {false, SegmentState::ERROR};
    }

    return {false, state};
}

void
SegmentInfo::SetOccupiedStripeCount(uint32_t cnt)
{
    occupiedStripeCount = cnt;
}

uint32_t
SegmentInfo::GetOccupiedStripeCount(void)
{
    return occupiedStripeCount;
}

uint32_t
SegmentInfo::IncreaseOccupiedStripeCount(void)
{
    // ++ is equivalent to fetch_add(1) + 1
    return ++occupiedStripeCount;
}

void
SegmentInfo::SetState(SegmentState newState)
{
    std::lock_guard<std::mutex> lock(seglock);
    SegmentState prevState = state;
    state = newState;
    POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_SETSTATE), "segment_id: {}, prev_state: {}, next_state: {}, array_id: {}",
        segmentId, ToSegmentStateString(prevState), ToSegmentStateString(state), arrayId);
}

void
SegmentInfo::SetArrayId(int arrayId) {
    this->arrayId = arrayId;
}

void
SegmentInfo::SetSegmentId(SegmentId segmentId)
{
    this->segmentId = segmentId;
}

SegmentState
SegmentInfo::GetState(void)
{
    std::lock_guard<std::mutex> lock(seglock);
    return state;
}

void
SegmentInfo::_MoveToFreeState(void)
{
    occupiedStripeCount = 0;
    validBlockCount = 0;
    SegmentState prevState = state;
    state = SegmentState::FREE;
    POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_MOVE_TO_FREE), "segment_id: {}, prev_state: {}, next_state: {}, array_id: {}",
        segmentId, ToSegmentStateString(prevState), ToSegmentStateString(state), arrayId);
}

void
SegmentInfo::MoveToNvramState(void)
{
    std::lock_guard<std::mutex> lock(seglock);
    if (state != SegmentState::FREE)
    {
        POS_TRACE_ERROR(EID(UNKNOWN_ALLOCATOR_ERROR),
            "Failed to move to NVRAM state. Segment state {} valid count {} occupied stripe count {} segmentId {} arrayId {}",
            ToSegmentStateString(state), validBlockCount, occupiedStripeCount, segmentId, arrayId);
        assert(false);
    }

    SegmentState prevState = state;
    state = SegmentState::NVRAM;
    POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_MOVE_TO_NVRAM), "segment_id: {}, prev_state: {}, next_state: {}, array_id: {}",
        segmentId, ToSegmentStateString(prevState), ToSegmentStateString(state), arrayId);
}

bool
SegmentInfo::MoveToSsdStateOrFreeStateIfItBecomesEmpty(void)
{
    std::lock_guard<std::mutex> lock(seglock);

    if (validBlockCount == 0)
    {
        _MoveToFreeState();

        return true;
    }
    else
    {
        SegmentState prevState = state;
        state = SegmentState::SSD;
        POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_MOVE_TO_SSD_OR_FREE), "segment_id: {}, prev_state: {}, next_state: {}, array_id: {}",
            segmentId, ToSegmentStateString(prevState), ToSegmentStateString(state), arrayId);
        return false;
    }
}

bool
SegmentInfo::MoveVictimToFree(void)
{
    std::lock_guard<std::mutex> lock(seglock);

    if (state == SegmentState::VICTIM && validBlockCount == 0)
    {
        _MoveToFreeState();

        return true;
    }
    else
    {
        POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_MOVE_TO_SSD_OR_FREE),
            "segment_id: {}, prev_state: {}, next_state: {}, valid_block_count: {}, array_id: {}",
            segmentId, ToSegmentStateString(state), ToSegmentStateString(SegmentState::VICTIM), validBlockCount, arrayId);
        return false;
    }
}

bool
SegmentInfo::MoveToVictimState(void)
{
    std::lock_guard<std::mutex> lock(seglock);
    if (state != SegmentState::SSD)
    {
        POS_TRACE_ERROR(EID(UNKNOWN_ALLOCATOR_ERROR),
            "Cannot move to victim state as it's not SSD state, state: {}, segment_id: {}, array_id: {}", ToSegmentStateString(state), segmentId, arrayId);
        return false;
    }
    else
    {
        SegmentState prevState = state;
        state = SegmentState::VICTIM;
        POS_TRACE_INFO(EID(ALLOCATOR_SEGINFO_MOVE_TO_VICTIM), "segment_id: {}, prev_state: {}, next_state: {}, array_id: {}",
            segmentId, ToSegmentStateString(prevState), ToSegmentStateString(state), arrayId);

        return true;
    }
}

uint32_t
SegmentInfo::GetValidBlockCountIfSsdState(void)
{
    std::lock_guard<std::mutex> lock(seglock);
    if (state != SegmentState::SSD)
    {
        return UINT32_MAX;
    }
    else
    {
        return validBlockCount;
    }
}

std::string
SegmentInfo::ToSegmentStateString(SegmentState state) {
    string stateStr = "Init";
    switch(state) {
        case SegmentState::FREE:
            stateStr = "FREE";
            break;
        case SegmentState::NVRAM:
            stateStr = "NVRAM";
            break;
        case SegmentState::SSD:
            stateStr = "SSD";
            break;
        case SegmentState::VICTIM:
            stateStr = "VICTIM";
            break;
        default:
            stateStr = "OTHER_" + std::to_string(state);
            break;
    }
    return stateStr;
}
} // namespace pos
