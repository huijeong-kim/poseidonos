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

#include "src/logger/logger.h"
#include "src/allocator/context_manager/segment_ctx/segment_list.h"

namespace pos
{
SegmentList::SegmentList(int arrayId, SegmentState state)
: arrayId(arrayId), state(state), numSegments(0)
{
}

void
SegmentList::Reset(void)
{
    std::lock_guard<std::mutex> lock(m);
    auto sizeBeforeClear = segments.size();
    segments.clear();
    numSegments = 0;
    POS_TRACE_INFO(EID(ALLOCATOR_SEGMENTLIST_RESET), "prev_size: {}, array_id: {}, state: {}, num_segments: {}",
        sizeBeforeClear, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
}

SegmentId
SegmentList::PopSegment(void)
{
    std::lock_guard<std::mutex> lock(m);
    SegmentId ret;

    if (segments.empty() == true)
    {
        ret = UNMAP_SEGMENT;
    }
    else
    {
        auto it = segments.begin();
        ret = *it;

        segments.erase(it);
        numSegments = segments.size();
        POS_TRACE_INFO(EID(ALLOCATOR_SEGMENTLIST_POP), "segment_id: {}, array_id: {}, state: {}, num_segments: {}",
            ret, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
    }

    return ret;
}

void
SegmentList::AddToList(SegmentId segId)
{
    std::lock_guard<std::mutex> lock(m);
    auto itor = segments.find(segId);
    if (itor != segments.end()) {
        POS_TRACE_WARN(EID(ALLOCATOR_SEGMENTLIST_DUPLICATED_ADD), "segment_id: {}, array_id: {}, state: {}, num_segments: {}",
            segId, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
    }
    segments.insert(segId);
    numSegments = segments.size();
    POS_TRACE_INFO(EID(ALLOCATOR_SEGMENTLIST_ADD), "segment_id: {}, array_id: {}, state: {}, num_segments: {}",
        segId, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
}

bool
SegmentList::RemoveFromList(SegmentId segId)
{
    std::lock_guard<std::mutex> lock(m);
    bool removed = false;
    auto it = segments.find(segId);

    if (it != segments.end())
    {
        segments.erase(it);
        numSegments = segments.size();

        removed = true;
        POS_TRACE_INFO(EID(ALLOCATOR_SEGMENTLIST_REMOVE), "segment_id: {}, array_id: {}, state: {}, num_segments: {}",
            segId, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
    } else {
        POS_TRACE_WARN(EID(ALLOCATOR_SEGMENTLIST_FAILED_TO_REMOVE), "segment_id: {}, array_id: {}, state: {}, num_segments: {}",
            segId, arrayId, SegmentInfo::ToSegmentStateString(state), numSegments);
    }

    return removed;
}

uint32_t
SegmentList::GetNumSegments(void)
{
    std::lock_guard<std::mutex> lock(m);
    return numSegments;
}

uint32_t
SegmentList::GetNumSegmentsWoLock(void)
{
    return numSegments;
}

bool
SegmentList::Contains(SegmentId segId)
{
    std::lock_guard<std::mutex> lock(m);
    return (segments.find(segId) != segments.end());
}

std::set<SegmentId>
SegmentList::GetList(void)
{
    std::lock_guard<std::mutex> lock(m);
    return segments;
}
} // namespace pos
