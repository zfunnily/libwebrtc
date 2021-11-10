/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "modules/video_coding/jitter_buffer.h"

#include <assert.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/video_coding/frame_buffer.h"
#include "modules/video_coding/include/video_coding.h"
#include "modules/video_coding/inter_frame_delay.h"
#include "modules/video_coding/internal_defines.h"
#include "modules/video_coding/jitter_buffer_common.h"
#include "modules/video_coding/jitter_estimator.h"
#include "modules/video_coding/packet.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/system/fallthrough.h"
#include "system_wrappers/include/clock.h"
#include "system_wrappers/include/field_trial.h"

namespace webrtc {
// Interval for updating SS data.
static const uint32_t kSsCleanupIntervalSec = 60;

// Use this rtt if no value has been reported.
static const int64_t kDefaultRtt = 200;

typedef std::pair<uint32_t, VCMFrameBuffer*> FrameListPair;

bool IsKeyFrame(FrameListPair pair) {
  return pair.second->FrameType() == VideoFrameType::kVideoFrameKey;
}

bool HasNonEmptyState(FrameListPair pair) {
  return pair.second->GetState() != kStateEmpty;
}

void FrameList::InsertFrame(VCMFrameBuffer* frame) {
  insert(rbegin().base(), FrameListPair(frame->Timestamp(), frame));
}

VCMFrameBuffer* FrameList::PopFrame(uint32_t timestamp) {
  FrameList::iterator it = find(timestamp);
  if (it == end())
    return NULL;
  VCMFrameBuffer* frame = it->second;
  erase(it);
  return frame;
}

VCMFrameBuffer* FrameList::Front() const {
  return begin()->second;
}

VCMFrameBuffer* FrameList::Back() const {
  return rbegin()->second;
}

int FrameList::RecycleFramesUntilKeyFrame(FrameList::iterator* key_frame_it,
                                          UnorderedFrameList* free_frames) {
  int drop_count = 0;
  FrameList::iterator it = begin();
  while (!empty()) {
    // Throw at least one frame.
    it->second->Reset();
    free_frames->push_back(it->second);
    erase(it++);
    ++drop_count;
    if (it != end() &&
        it->second->FrameType() == VideoFrameType::kVideoFrameKey) {
      *key_frame_it = it;
      return drop_count;
    }
  }
  *key_frame_it = end();
  return drop_count;
}

void FrameList::CleanUpOldOrEmptyFrames(VCMDecodingState* decoding_state,
                                        UnorderedFrameList* free_frames) {
  while (!empty()) {
    VCMFrameBuffer* oldest_frame = Front();
    bool remove_frame = false;
    if (oldest_frame->GetState() == kStateEmpty && size() > 1) {
      // This frame is empty, try to update the last decoded state and drop it
      // if successful.
      remove_frame = decoding_state->UpdateEmptyFrame(oldest_frame);
    } else {
      remove_frame = decoding_state->IsOldFrame(oldest_frame);
    }
    if (!remove_frame) {
      break;
    }
    free_frames->push_back(oldest_frame);
    erase(begin());
  }
}

void FrameList::Reset(UnorderedFrameList* free_frames) {
  while (!empty()) {
    begin()->second->Reset();
    free_frames->push_back(begin()->second);
    erase(begin());
  }
}

Vp9SsMap::Vp9SsMap() {}
Vp9SsMap::~Vp9SsMap() {}

bool Vp9SsMap::Insert(const VCMPacket& packet) {
  const auto& vp9_header =
      absl::get<RTPVideoHeaderVP9>(packet.video_header.video_type_header);
  if (!vp9_header.ss_data_available)
    return false;

  ss_map_[packet.timestamp] = vp9_header.gof;
  return true;
}

void Vp9SsMap::Reset() {
  ss_map_.clear();
}

bool Vp9SsMap::Find(uint32_t timestamp, SsMap::iterator* it_out) {
  bool found = false;
  for (SsMap::iterator it = ss_map_.begin(); it != ss_map_.end(); ++it) {
    if (it->first == timestamp || IsNewerTimestamp(timestamp, it->first)) {
      *it_out = it;
      found = true;
    }
  }
  return found;
}

void Vp9SsMap::RemoveOld(uint32_t timestamp) {
  if (!TimeForCleanup(timestamp))
    return;

  SsMap::iterator it;
  if (!Find(timestamp, &it))
    return;

  ss_map_.erase(ss_map_.begin(), it);
  AdvanceFront(timestamp);
}

bool Vp9SsMap::TimeForCleanup(uint32_t timestamp) const {
  if (ss_map_.empty() || !IsNewerTimestamp(timestamp, ss_map_.begin()->first))
    return false;

  uint32_t diff = timestamp - ss_map_.begin()->first;
  return diff / kVideoPayloadTypeFrequency >= kSsCleanupIntervalSec;
}

void Vp9SsMap::AdvanceFront(uint32_t timestamp) {
  RTC_DCHECK(!ss_map_.empty());
  GofInfoVP9 gof = ss_map_.begin()->second;
  ss_map_.erase(ss_map_.begin());
  ss_map_[timestamp] = gof;
}

// TODO(asapersson): Update according to updates in RTP payload profile.
bool Vp9SsMap::UpdatePacket(VCMPacket* packet) {
  auto& vp9_header =
      absl::get<RTPVideoHeaderVP9>(packet->video_header.video_type_header);
  uint8_t gof_idx = vp9_header.gof_idx;
  if (gof_idx == kNoGofIdx)
    return false;  // No update needed.

  SsMap::iterator it;
  if (!Find(packet->timestamp, &it))
    return false;  // Corresponding SS not yet received.

  if (gof_idx >= it->second.num_frames_in_gof)
    return false;  // Assume corresponding SS not yet received.

  vp9_header.temporal_idx = it->second.temporal_idx[gof_idx];
  vp9_header.temporal_up_switch = it->second.temporal_up_switch[gof_idx];

  // TODO(asapersson): Set vp9.ref_picture_id[i] and add usage.
  vp9_header.num_ref_pics = it->second.num_ref_pics[gof_idx];
  for (uint8_t i = 0; i < it->second.num_ref_pics[gof_idx]; ++i) {
    vp9_header.pid_diff[i] = it->second.pid_diff[gof_idx][i];
  }
  return true;
}

void Vp9SsMap::UpdateFrames(FrameList* frames) {
  for (const auto& frame_it : *frames) {
    uint8_t gof_idx =
        frame_it.second->CodecSpecific()->codecSpecific.VP9.gof_idx;
    if (gof_idx == kNoGofIdx) {
      continue;
    }
    SsMap::iterator ss_it;
    if (Find(frame_it.second->Timestamp(), &ss_it)) {
      if (gof_idx >= ss_it->second.num_frames_in_gof) {
        continue;  // Assume corresponding SS not yet received.
      }
      frame_it.second->SetGofInfo(ss_it->second, gof_idx);
    }
  }
}

VCMJitterBuffer::VCMJitterBuffer(Clock* clock,
                                 std::unique_ptr<EventWrapper> event)
    : clock_(clock),
      running_(false),
      frame_event_(std::move(event)),
      max_number_of_frames_(kStartNumberOfFrames),
      free_frames_(),
      decodable_frames_(),
      incomplete_frames_(),
      last_decoded_state_(),
      first_packet_since_reset_(true),
      num_consecutive_old_packets_(0),
      num_packets_(0),
      num_duplicated_packets_(0),
      jitter_estimate_(clock),
      inter_frame_delay_(clock_->TimeInMilliseconds()),
      rtt_ms_(kDefaultRtt),
      missing_sequence_numbers_(SequenceNumberLessThan()),
      latest_received_sequence_number_(0),
      max_nack_list_size_(0),
      max_packet_age_to_nack_(0),
      max_incomplete_time_ms_(0),
      average_packets_per_frame_(0.0f),
      frame_counter_(0) {
  for (int i = 0; i < kStartNumberOfFrames; i++)
    free_frames_.push_back(new VCMFrameBuffer());
}

VCMJitterBuffer::~VCMJitterBuffer() {
  Stop();
  for (UnorderedFrameList::iterator it = free_frames_.begin();
       it != free_frames_.end(); ++it) {
    delete *it;
  }
  for (FrameList::iterator it = incomplete_frames_.begin();
       it != incomplete_frames_.end(); ++it) {
    delete it->second;
  }
  for (FrameList::iterator it = decodable_frames_.begin();
       it != decodable_frames_.end(); ++it) {
    delete it->second;
  }
}

void VCMJitterBuffer::Start() {
  rtc::CritScope cs(&crit_sect_);
  running_ = true;

  num_consecutive_old_packets_ = 0;
  num_packets_ = 0;
  num_duplicated_packets_ = 0;

  // Start in a non-signaled state.
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_since_reset_ = true;
  rtt_ms_ = kDefaultRtt;
  last_decoded_state_.Reset();

  decodable_frames_.Reset(&free_frames_);
  incomplete_frames_.Reset(&free_frames_);
}

void VCMJitterBuffer::Stop() {
  rtc::CritScope cs(&crit_sect_);
  running_ = false;
  last_decoded_state_.Reset();

  // Make sure we wake up any threads waiting on these events.
  frame_event_->Set();
}

bool VCMJitterBuffer::Running() const {
  rtc::CritScope cs(&crit_sect_);
  return running_;
}

void VCMJitterBuffer::Flush() {
  rtc::CritScope cs(&crit_sect_);
  decodable_frames_.Reset(&free_frames_);
  incomplete_frames_.Reset(&free_frames_);
  last_decoded_state_.Reset();  // TODO(mikhal): sync reset.
  num_consecutive_old_packets_ = 0;
  // Also reset the jitter and delay estimates
  jitter_estimate_.Reset();
  inter_frame_delay_.Reset(clock_->TimeInMilliseconds());
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_since_reset_ = true;
  missing_sequence_numbers_.clear();
}

int VCMJitterBuffer::num_packets() const {
  rtc::CritScope cs(&crit_sect_);
  return num_packets_;
}

int VCMJitterBuffer::num_duplicated_packets() const {
  rtc::CritScope cs(&crit_sect_);
  return num_duplicated_packets_;
}

// Returns immediately or a |max_wait_time_ms| ms event hang waiting for a
// complete frame, |max_wait_time_ms| decided by caller.
VCMEncodedFrame* VCMJitterBuffer::NextCompleteFrame(uint32_t max_wait_time_ms) {
  crit_sect_.Enter();
  if (!running_) {
    crit_sect_.Leave();
    return nullptr;
  }
  CleanUpOldOrEmptyFrames();

  if (decodable_frames_.empty() ||
      decodable_frames_.Front()->GetState() != kStateComplete) {
    const int64_t end_wait_time_ms =
        clock_->TimeInMilliseconds() + max_wait_time_ms;
    int64_t wait_time_ms = max_wait_time_ms;
    while (wait_time_ms > 0) {
      crit_sect_.Leave();
      const EventTypeWrapper ret =
          frame_event_->Wait(static_cast<uint32_t>(wait_time_ms));
      crit_sect_.Enter();
      if (ret == kEventSignaled) {
        // Are we shutting down the jitter buffer?
        if (!running_) {
          crit_sect_.Leave();
          return nullptr;
        }
        // Finding oldest frame ready for decoder.
        CleanUpOldOrEmptyFrames();
        if (decodable_frames_.empty() ||
            decodable_frames_.Front()->GetState() != kStateComplete) {
          wait_time_ms = end_wait_time_ms - clock_->TimeInMilliseconds();
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
  if (decodable_frames_.empty() ||
      decodable_frames_.Front()->GetState() != kStateComplete) {
    crit_sect_.Leave();
    return nullptr;
  }
  VCMEncodedFrame* encoded_frame = decodable_frames_.Front();
  crit_sect_.Leave();
  return encoded_frame;
}

VCMEncodedFrame* VCMJitterBuffer::ExtractAndSetDecode(uint32_t timestamp) {
  rtc::CritScope cs(&crit_sect_);
  if (!running_) {
    return NULL;
  }
  // Extract the frame with the desired timestamp.
  VCMFrameBuffer* frame = decodable_frames_.PopFrame(timestamp);
  bool continuous = true;
  if (!frame) {
    frame = incomplete_frames_.PopFrame(timestamp);
    if (frame)
      continuous = last_decoded_state_.ContinuousFrame(frame);
    else
      return NULL;
  }
  // Frame pulled out from jitter buffer, update the jitter estimate.
  const bool retransmitted = (frame->GetNackCount() > 0);
  if (retransmitted) {
    jitter_estimate_.FrameNacked();
  } else if (frame->size() > 0) {
    // Ignore retransmitted and empty frames.
    if (waiting_for_completion_.latest_packet_time >= 0) {
      UpdateJitterEstimate(waiting_for_completion_, true);
    }
    if (frame->GetState() == kStateComplete) {
      UpdateJitterEstimate(*frame, false);
    } else {
      // Wait for this one to get complete.
      waiting_for_completion_.frame_size = frame->size();
      waiting_for_completion_.latest_packet_time = frame->LatestPacketTimeMs();
      waiting_for_completion_.timestamp = frame->Timestamp();
    }
  }

  // The state must be changed to decoding before cleaning up zero sized
  // frames to avoid empty frames being cleaned up and then given to the
  // decoder. Propagates the missing_frame bit.
  frame->PrepareForDecode(continuous);

  // We have a frame - update the last decoded state and nack list.
  last_decoded_state_.SetState(frame);
  DropPacketsFromNackList(last_decoded_state_.sequence_num());

  if ((*frame).IsSessionComplete())
    UpdateAveragePacketsPerFrame(frame->NumPackets());

  return frame;
}

// Release frame when done with decoding. Should never be used to release
// frames from within the jitter buffer.
void VCMJitterBuffer::ReleaseFrame(VCMEncodedFrame* frame) {
  RTC_CHECK(frame != nullptr);
  rtc::CritScope cs(&crit_sect_);
  VCMFrameBuffer* frame_buffer = static_cast<VCMFrameBuffer*>(frame);
  RecycleFrameBuffer(frame_buffer);
}

// Gets frame to use for this timestamp. If no match, get empty frame.
VCMFrameBufferEnum VCMJitterBuffer::GetFrame(const VCMPacket& packet,
                                             VCMFrameBuffer** frame,
                                             FrameList** frame_list) {
  *frame = incomplete_frames_.PopFrame(packet.timestamp);
  if (*frame != NULL) {
    *frame_list = &incomplete_frames_;
    return kNoError;
  }
  *frame = decodable_frames_.PopFrame(packet.timestamp);
  if (*frame != NULL) {
    *frame_list = &decodable_frames_;
    return kNoError;
  }

  *frame_list = NULL;
  // No match, return empty frame.
  *frame = GetEmptyFrame();
  if (*frame == NULL) {
    // No free frame! Try to reclaim some...
    RTC_LOG(LS_WARNING) << "Unable to get empty frame; Recycling.";
    bool found_key_frame = RecycleFramesUntilKeyFrame();
    *frame = GetEmptyFrame();
    RTC_CHECK(*frame);
    if (!found_key_frame) {
      RecycleFrameBuffer(*frame);
      return kFlushIndicator;
    }
  }
  (*frame)->Reset();
  return kNoError;
}

int64_t VCMJitterBuffer::LastPacketTime(const VCMEncodedFrame* frame,
                                        bool* retransmitted) const {
  assert(retransmitted);
  rtc::CritScope cs(&crit_sect_);
  const VCMFrameBuffer* frame_buffer =
      static_cast<const VCMFrameBuffer*>(frame);
  *retransmitted = (frame_buffer->GetNackCount() > 0);
  return frame_buffer->LatestPacketTimeMs();
}

VCMFrameBufferEnum VCMJitterBuffer::InsertPacket(const VCMPacket& packet,
                                                 bool* retransmitted) {
  rtc::CritScope cs(&crit_sect_);

  ++num_packets_;
  // Does this packet belong to an old frame?
  if (last_decoded_state_.IsOldPacket(&packet)) {
    // Account only for media packets.
    if (packet.sizeBytes > 0) {
      num_consecutive_old_packets_++;
    }
    // Update last decoded sequence number if the packet arrived late and
    // belongs to a frame with a timestamp equal to the last decoded
    // timestamp.
    last_decoded_state_.UpdateOldPacket(&packet);
    DropPacketsFromNackList(last_decoded_state_.sequence_num());

    // Also see if this old packet made more incomplete frames continuous.
    FindAndInsertContinuousFramesWithState(last_decoded_state_);

    if (num_consecutive_old_packets_ > kMaxConsecutiveOldPackets) {
      RTC_LOG(LS_WARNING)
          << num_consecutive_old_packets_
          << " consecutive old packets received. Flushing the jitter buffer.";
      Flush();
      return kFlushIndicator;
    }
    return kOldPacket;
  }

  num_consecutive_old_packets_ = 0;

  VCMFrameBuffer* frame;
  FrameList* frame_list;
  const VCMFrameBufferEnum error = GetFrame(packet, &frame, &frame_list);
  if (error != kNoError)
    return error;

  int64_t now_ms = clock_->TimeInMilliseconds();
  // We are keeping track of the first and latest seq numbers, and
  // the number of wraps to be able to calculate how many packets we expect.
  if (first_packet_since_reset_) {
    // Now it's time to start estimating jitter
    // reset the delay estimate.
    inter_frame_delay_.Reset(now_ms);
  }

  // Empty packets may bias the jitter estimate (lacking size component),
  // therefore don't let empty packet trigger the following updates:
  if (packet.frameType != VideoFrameType::kEmptyFrame) {
    if (waiting_for_completion_.timestamp == packet.timestamp) {
      // This can get bad if we have a lot of duplicate packets,
      // we will then count some packet multiple times.
      waiting_for_completion_.frame_size += packet.sizeBytes;
      waiting_for_completion_.latest_packet_time = now_ms;
    } else if (waiting_for_completion_.latest_packet_time >= 0 &&
               waiting_for_completion_.latest_packet_time + 2000 <= now_ms) {
      // A packet should never be more than two seconds late
      UpdateJitterEstimate(waiting_for_completion_, true);
      waiting_for_completion_.latest_packet_time = -1;
      waiting_for_completion_.frame_size = 0;
      waiting_for_completion_.timestamp = 0;
    }
  }

  VCMFrameBufferStateEnum previous_state = frame->GetState();
  // Insert packet.
  FrameData frame_data;
  frame_data.rtt_ms = rtt_ms_;
  frame_data.rolling_average_packets_per_frame = average_packets_per_frame_;
  VCMFrameBufferEnum buffer_state =
      frame->InsertPacket(packet, now_ms, frame_data);

  if (buffer_state > 0) {
    if (first_packet_since_reset_) {
      latest_received_sequence_number_ = packet.seqNum;
      first_packet_since_reset_ = false;
    } else {
      if (IsPacketRetransmitted(packet)) {
        frame->IncrementNackCount();
      }
      if (!UpdateNackList(packet.seqNum) &&
          packet.frameType != VideoFrameType::kVideoFrameKey) {
        buffer_state = kFlushIndicator;
      }

      latest_received_sequence_number_ =
          LatestSequenceNumber(latest_received_sequence_number_, packet.seqNum);
    }
  }

  // Is the frame already in the decodable list?
  bool continuous = IsContinuous(*frame);
  switch (buffer_state) {
    case kGeneralError:
    case kTimeStampError:
    case kSizeError: {
      RecycleFrameBuffer(frame);
      break;
    }
    case kCompleteSession: {
      if (previous_state != kStateComplete) {
        if (continuous) {
          // Signal that we have a complete session.
          frame_event_->Set();
        }
      }

      *retransmitted = (frame->GetNackCount() > 0);
      if (continuous) {
        decodable_frames_.InsertFrame(frame);
        FindAndInsertContinuousFrames(*frame);
      } else {
        incomplete_frames_.InsertFrame(frame);
      }
      break;
    }
    case kIncomplete: {
      if (frame->GetState() == kStateEmpty &&
          last_decoded_state_.UpdateEmptyFrame(frame)) {
        RecycleFrameBuffer(frame);
        return kNoError;
      } else {
        incomplete_frames_.InsertFrame(frame);
      }
      break;
    }
    case kNoError:
    case kOutOfBoundsPacket:
    case kDuplicatePacket: {
      // Put back the frame where it came from.
      if (frame_list != NULL) {
        frame_list->InsertFrame(frame);
      } else {
        RecycleFrameBuffer(frame);
      }
      ++num_duplicated_packets_;
      break;
    }
    case kFlushIndicator:
      RecycleFrameBuffer(frame);
      return kFlushIndicator;
    default:
      assert(false);
  }
  return buffer_state;
}

bool VCMJitterBuffer::IsContinuousInState(
    const VCMFrameBuffer& frame,
    const VCMDecodingState& decoding_state) const {
  // Is this frame complete and continuous?
  return (frame.GetState() == kStateComplete) &&
         decoding_state.ContinuousFrame(&frame);
}

bool VCMJitterBuffer::IsContinuous(const VCMFrameBuffer& frame) const {
  if (IsContinuousInState(frame, last_decoded_state_)) {
    return true;
  }
  VCMDecodingState decoding_state;
  decoding_state.CopyFrom(last_decoded_state_);
  for (FrameList::const_iterator it = decodable_frames_.begin();
       it != decodable_frames_.end(); ++it) {
    VCMFrameBuffer* decodable_frame = it->second;
    if (IsNewerTimestamp(decodable_frame->Timestamp(), frame.Timestamp())) {
      break;
    }
    decoding_state.SetState(decodable_frame);
    if (IsContinuousInState(frame, decoding_state)) {
      return true;
    }
  }
  return false;
}

void VCMJitterBuffer::FindAndInsertContinuousFrames(
    const VCMFrameBuffer& new_frame) {
  VCMDecodingState decoding_state;
  decoding_state.CopyFrom(last_decoded_state_);
  decoding_state.SetState(&new_frame);
  FindAndInsertContinuousFramesWithState(decoding_state);
}

void VCMJitterBuffer::FindAndInsertContinuousFramesWithState(
    const VCMDecodingState& original_decoded_state) {
  // Copy original_decoded_state so we can move the state forward with each
  // decodable frame we find.
  VCMDecodingState decoding_state;
  decoding_state.CopyFrom(original_decoded_state);

  // When temporal layers are available, we search for a complete or decodable
  // frame until we hit one of the following:
  // 1. Continuous base or sync layer.
  // 2. The end of the list was reached.
  for (FrameList::iterator it = incomplete_frames_.begin();
       it != incomplete_frames_.end();) {
    VCMFrameBuffer* frame = it->second;
    if (IsNewerTimestamp(original_decoded_state.time_stamp(),
                         frame->Timestamp())) {
      ++it;
      continue;
    }
    if (IsContinuousInState(*frame, decoding_state)) {
      decodable_frames_.InsertFrame(frame);
      incomplete_frames_.erase(it++);
      decoding_state.SetState(frame);
    } else if (frame->TemporalId() <= 0) {
      break;
    } else {
      ++it;
    }
  }
}

uint32_t VCMJitterBuffer::EstimatedJitterMs() {
  rtc::CritScope cs(&crit_sect_);
  const double rtt_mult = 1.0f;
  return jitter_estimate_.GetJitterEstimate(rtt_mult);
}

void VCMJitterBuffer::UpdateRtt(int64_t rtt_ms) {
  rtc::CritScope cs(&crit_sect_);
  rtt_ms_ = rtt_ms;
  jitter_estimate_.UpdateRtt(rtt_ms);
}

void VCMJitterBuffer::SetNackSettings(size_t max_nack_list_size,
                                      int max_packet_age_to_nack,
                                      int max_incomplete_time_ms) {
  rtc::CritScope cs(&crit_sect_);
  assert(max_packet_age_to_nack >= 0);
  assert(max_incomplete_time_ms_ >= 0);
  max_nack_list_size_ = max_nack_list_size;
  max_packet_age_to_nack_ = max_packet_age_to_nack;
  max_incomplete_time_ms_ = max_incomplete_time_ms;
}

int VCMJitterBuffer::NonContinuousOrIncompleteDuration() {
  if (incomplete_frames_.empty()) {
    return 0;
  }
  uint32_t start_timestamp = incomplete_frames_.Front()->Timestamp();
  if (!decodable_frames_.empty()) {
    start_timestamp = decodable_frames_.Back()->Timestamp();
  }
  return incomplete_frames_.Back()->Timestamp() - start_timestamp;
}

uint16_t VCMJitterBuffer::EstimatedLowSequenceNumber(
    const VCMFrameBuffer& frame) const {
  assert(frame.GetLowSeqNum() >= 0);
  if (frame.HaveFirstPacket())
    return frame.GetLowSeqNum();

  // This estimate is not accurate if more than one packet with lower sequence
  // number is lost.
  return frame.GetLowSeqNum() - 1;
}

std::vector<uint16_t> VCMJitterBuffer::GetNackList(bool* request_key_frame) {
  rtc::CritScope cs(&crit_sect_);
  *request_key_frame = false;
  if (last_decoded_state_.in_initial_state()) {
    VCMFrameBuffer* next_frame = NextFrame();
    const bool first_frame_is_key =
        next_frame &&
        next_frame->FrameType() == VideoFrameType::kVideoFrameKey &&
        next_frame->HaveFirstPacket();
    if (!first_frame_is_key) {
      bool have_non_empty_frame =
          decodable_frames_.end() != find_if(decodable_frames_.begin(),
                                             decodable_frames_.end(),
                                             HasNonEmptyState);
      if (!have_non_empty_frame) {
        have_non_empty_frame =
            incomplete_frames_.end() != find_if(incomplete_frames_.begin(),
                                                incomplete_frames_.end(),
                                                HasNonEmptyState);
      }
      bool found_key_frame = RecycleFramesUntilKeyFrame();
      if (!found_key_frame) {
        *request_key_frame = have_non_empty_frame;
        return std::vector<uint16_t>();
      }
    }
  }
  if (TooLargeNackList()) {
    *request_key_frame = !HandleTooLargeNackList();
  }
  if (max_incomplete_time_ms_ > 0) {
    int non_continuous_incomplete_duration =
        NonContinuousOrIncompleteDuration();
    if (non_continuous_incomplete_duration > 90 * max_incomplete_time_ms_) {
      RTC_LOG_F(LS_WARNING) << "Too long non-decodable duration: "
                            << non_continuous_incomplete_duration << " > "
                            << 90 * max_incomplete_time_ms_;
      FrameList::reverse_iterator rit = find_if(
          incomplete_frames_.rbegin(), incomplete_frames_.rend(), IsKeyFrame);
      if (rit == incomplete_frames_.rend()) {
        // Request a key frame if we don't have one already.
        *request_key_frame = true;
        return std::vector<uint16_t>();
      } else {
        // Skip to the last key frame. If it's incomplete we will start
        // NACKing it.
        // Note that the estimated low sequence number is correct for VP8
        // streams because only the first packet of a key frame is marked.
        last_decoded_state_.Reset();
        DropPacketsFromNackList(EstimatedLowSequenceNumber(*rit->second));
      }
    }
  }
  std::vector<uint16_t> nack_list(missing_sequence_numbers_.begin(),
                                  missing_sequence_numbers_.end());
  return nack_list;
}

VCMFrameBuffer* VCMJitterBuffer::NextFrame() const {
  if (!decodable_frames_.empty())
    return decodable_frames_.Front();
  if (!incomplete_frames_.empty())
    return incomplete_frames_.Front();
  return NULL;
}

bool VCMJitterBuffer::UpdateNackList(uint16_t sequence_number) {
  // Make sure we don't add packets which are already too old to be decoded.
  if (!last_decoded_state_.in_initial_state()) {
    latest_received_sequence_number_ = LatestSequenceNumber(
        latest_received_sequence_number_, last_decoded_state_.sequence_num());
  }
  if (IsNewerSequenceNumber(sequence_number,
                            latest_received_sequence_number_)) {
    // Push any missing sequence numbers to the NACK list.
    for (uint16_t i = latest_received_sequence_number_ + 1;
         IsNewerSequenceNumber(sequence_number, i); ++i) {
      missing_sequence_numbers_.insert(missing_sequence_numbers_.end(), i);
    }
    if (TooLargeNackList() && !HandleTooLargeNackList()) {
      RTC_LOG(LS_WARNING) << "Requesting key frame due to too large NACK list.";
      return false;
    }
    if (MissingTooOldPacket(sequence_number) &&
        !HandleTooOldPackets(sequence_number)) {
      RTC_LOG(LS_WARNING)
          << "Requesting key frame due to missing too old packets";
      return false;
    }
  } else {
    missing_sequence_numbers_.erase(sequence_number);
  }
  return true;
}

bool VCMJitterBuffer::TooLargeNackList() const {
  return missing_sequence_numbers_.size() > max_nack_list_size_;
}

bool VCMJitterBuffer::HandleTooLargeNackList() {
  // Recycle frames until the NACK list is small enough. It is likely cheaper to
  // request a key frame than to retransmit this many missing packets.
  RTC_LOG_F(LS_WARNING) << "NACK list has grown too large: "
                        << missing_sequence_numbers_.size() << " > "
                        << max_nack_list_size_;
  bool key_frame_found = false;
  while (TooLargeNackList()) {
    key_frame_found = RecycleFramesUntilKeyFrame();
  }
  return key_frame_found;
}

bool VCMJitterBuffer::MissingTooOldPacket(
    uint16_t latest_sequence_number) const {
  if (missing_sequence_numbers_.empty()) {
    return false;
  }
  const uint16_t age_of_oldest_missing_packet =
      latest_sequence_number - *missing_sequence_numbers_.begin();
  // Recycle frames if the NACK list contains too old sequence numbers as
  // the packets may have already been dropped by the sender.
  return age_of_oldest_missing_packet > max_packet_age_to_nack_;
}

bool VCMJitterBuffer::HandleTooOldPackets(uint16_t latest_sequence_number) {
  bool key_frame_found = false;
  const uint16_t age_of_oldest_missing_packet =
      latest_sequence_number - *missing_sequence_numbers_.begin();
  RTC_LOG_F(LS_WARNING) << "NACK list contains too old sequence numbers: "
                        << age_of_oldest_missing_packet << " > "
                        << max_packet_age_to_nack_;
  while (MissingTooOldPacket(latest_sequence_number)) {
    key_frame_found = RecycleFramesUntilKeyFrame();
  }
  return key_frame_found;
}

void VCMJitterBuffer::DropPacketsFromNackList(
    uint16_t last_decoded_sequence_number) {
  // Erase all sequence numbers from the NACK list which we won't need any
  // longer.
  missing_sequence_numbers_.erase(
      missing_sequence_numbers_.begin(),
      missing_sequence_numbers_.upper_bound(last_decoded_sequence_number));
}

VCMFrameBuffer* VCMJitterBuffer::GetEmptyFrame() {
  if (free_frames_.empty()) {
    if (!TryToIncreaseJitterBufferSize()) {
      return NULL;
    }
  }
  VCMFrameBuffer* frame = free_frames_.front();
  free_frames_.pop_front();
  return frame;
}

bool VCMJitterBuffer::TryToIncreaseJitterBufferSize() {
  if (max_number_of_frames_ >= kMaxNumberOfFrames)
    return false;
  free_frames_.push_back(new VCMFrameBuffer());
  ++max_number_of_frames_;
  return true;
}

// Recycle oldest frames up to a key frame, used if jitter buffer is completely
// full.
bool VCMJitterBuffer::RecycleFramesUntilKeyFrame() {
  // First release incomplete frames, and only release decodable frames if there
  // are no incomplete ones.
  FrameList::iterator key_frame_it;
  bool key_frame_found = false;
  int dropped_frames = 0;
  dropped_frames += incomplete_frames_.RecycleFramesUntilKeyFrame(
      &key_frame_it, &free_frames_);
  key_frame_found = key_frame_it != incomplete_frames_.end();
  if (dropped_frames == 0) {
    dropped_frames += decodable_frames_.RecycleFramesUntilKeyFrame(
        &key_frame_it, &free_frames_);
    key_frame_found = key_frame_it != decodable_frames_.end();
  }
  if (key_frame_found) {
    RTC_LOG(LS_INFO) << "Found key frame while dropping frames.";
    // Reset last decoded state to make sure the next frame decoded is a key
    // frame, and start NACKing from here.
    last_decoded_state_.Reset();
    DropPacketsFromNackList(EstimatedLowSequenceNumber(*key_frame_it->second));
  } else if (decodable_frames_.empty()) {
    // All frames dropped. Reset the decoding state and clear missing sequence
    // numbers as we're starting fresh.
    last_decoded_state_.Reset();
    missing_sequence_numbers_.clear();
  }
  return key_frame_found;
}

void VCMJitterBuffer::UpdateAveragePacketsPerFrame(int current_number_packets) {
  if (frame_counter_ > kFastConvergeThreshold) {
    average_packets_per_frame_ =
        average_packets_per_frame_ * (1 - kNormalConvergeMultiplier) +
        current_number_packets * kNormalConvergeMultiplier;
  } else if (frame_counter_ > 0) {
    average_packets_per_frame_ =
        average_packets_per_frame_ * (1 - kFastConvergeMultiplier) +
        current_number_packets * kFastConvergeMultiplier;
    frame_counter_++;
  } else {
    average_packets_per_frame_ = current_number_packets;
    frame_counter_++;
  }
}

// Must be called under the critical section |crit_sect_|.
void VCMJitterBuffer::CleanUpOldOrEmptyFrames() {
  decodable_frames_.CleanUpOldOrEmptyFrames(&last_decoded_state_,
                                            &free_frames_);
  incomplete_frames_.CleanUpOldOrEmptyFrames(&last_decoded_state_,
                                             &free_frames_);
  if (!last_decoded_state_.in_initial_state()) {
    DropPacketsFromNackList(last_decoded_state_.sequence_num());
  }
}

// Must be called from within |crit_sect_|.
bool VCMJitterBuffer::IsPacketRetransmitted(const VCMPacket& packet) const {
  return missing_sequence_numbers_.find(packet.seqNum) !=
         missing_sequence_numbers_.end();
}

// Must be called under the critical section |crit_sect_|. Should never be
// called with retransmitted frames, they must be filtered out before this
// function is called.
void VCMJitterBuffer::UpdateJitterEstimate(const VCMJitterSample& sample,
                                           bool incomplete_frame) {
  if (sample.latest_packet_time == -1) {
    return;
  }
  UpdateJitterEstimate(sample.latest_packet_time, sample.timestamp,
                       sample.frame_size, incomplete_frame);
}

// Must be called under the critical section crit_sect_. Should never be
// called with retransmitted frames, they must be filtered out before this
// function is called.
void VCMJitterBuffer::UpdateJitterEstimate(const VCMFrameBuffer& frame,
                                           bool incomplete_frame) {
  if (frame.LatestPacketTimeMs() == -1) {
    return;
  }
  // No retransmitted frames should be a part of the jitter
  // estimate.
  UpdateJitterEstimate(frame.LatestPacketTimeMs(), frame.Timestamp(),
                       frame.size(), incomplete_frame);
}

// Must be called under the critical section |crit_sect_|. Should never be
// called with retransmitted frames, they must be filtered out before this
// function is called.
void VCMJitterBuffer::UpdateJitterEstimate(int64_t latest_packet_time_ms,
                                           uint32_t timestamp,
                                           unsigned int frame_size,
                                           bool incomplete_frame) {
  if (latest_packet_time_ms == -1) {
    return;
  }
  int64_t frame_delay;
  bool not_reordered = inter_frame_delay_.CalculateDelay(
      timestamp, &frame_delay, latest_packet_time_ms);
  // Filter out frames which have been reordered in time by the network
  if (not_reordered) {
    // Update the jitter estimate with the new samples
    jitter_estimate_.UpdateEstimate(frame_delay, frame_size, incomplete_frame);
  }
}

void VCMJitterBuffer::RecycleFrameBuffer(VCMFrameBuffer* frame) {
  frame->Reset();
  free_frames_.push_back(frame);
}

}  // namespace webrtc
