// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.
//
// Wall-clock stage times of a scene capture, for finding where the camera latency goes. Off unless
// PROJECTAIRSIM_CAMERA_TIMING_FILE names a file; then one tab-separated row per published capture is appended to it.

#pragma once

#include <chrono>

#include "CoreMinimal.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/Archive.h"

// All times are CLOCK_REALTIME nanoseconds (comparable with Python's time.time_ns() on the same machine), 0 = not set.
struct FCaptureTimes {
  int64 CaptureIndex = 0;
  int64 SimStamp = 0;         // the image's own sim time stamp
  int64 SimNowAtCapture = 0;  // SimClock now when the game thread issued the capture
  int64 TickNs = 0;           // game thread: start of the tick that issued the capture
  int64 CaptureBeginNs = 0;   // game thread: before CaptureScene()
  int64 CaptureEndNs = 0;     // game thread: after CaptureScene()
  int64 RenderedNs = 0;       // render thread: OnRendered runs (the capture's own render commands are done being recorded)
  int64 ReadyNs = 0;          // render thread: the GPU copy was seen finished
  int64 PackBeginNs = 0;      // worker thread: image packing starts
  int64 PackEndNs = 0;        // worker thread: PublishImages is about to run
  int32 InFlightAtCapture = 0;
  int32 TicksSinceCapture = 0;
  float TickDtMs = 0.f;       // time since the previous tick of this camera
  float GpuFrameMs = 0.f;     // RHI's last measured GPU frame time when the capture was issued
};

namespace CameraTiming {

inline int64 WallNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

inline FArchive* Sink() {
  static FArchive* Ar = []() -> FArchive* {
    const FString Path = FPlatformMisc::GetEnvironmentVariable(TEXT("PROJECTAIRSIM_CAMERA_TIMING_FILE"));
    if (Path.IsEmpty()) return nullptr;
    FArchive* Writer = IFileManager::Get().CreateFileWriter(*Path, FILEWRITE_AllowRead | FILEWRITE_Append);
    if (Writer) {
      const char* Header =
          "idx\tsim_stamp\tsim_now_capture\tsim_now_publish\ttick\tcap_begin\tcap_end\trendered\tready\tpack_begin\t"
          "pack_end\tpublish_end\tinflight\tticks_since\ttick_dt_ms\tgpu_frame_ms\n";
      Writer->Serialize((void*)Header, FCStringAnsi::Strlen(Header));
      Writer->Flush();
    }
    return Writer;
  }();
  return Ar;
}

inline bool Enabled() { return Sink() != nullptr; }

inline void LogRow(const FCaptureTimes& T, int64 PublishEndNs, int64 SimNowAtPublish) {
  FArchive* Ar = Sink();
  if (!Ar) return;
  static FCriticalSection Lock;
  char Row[512];
  const int32 Len = FCStringAnsi::Snprintf(
      Row, sizeof(Row),
      "%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\t%d\t%.2f\t%.2f\n",
      (long long)T.CaptureIndex, (long long)T.SimStamp, (long long)T.SimNowAtCapture, (long long)SimNowAtPublish,
      (long long)T.TickNs, (long long)T.CaptureBeginNs, (long long)T.CaptureEndNs, (long long)T.RenderedNs,
      (long long)T.ReadyNs, (long long)T.PackBeginNs, (long long)T.PackEndNs, (long long)PublishEndNs,
      T.InFlightAtCapture, T.TicksSinceCapture, T.TickDtMs, T.GpuFrameMs);
  FScopeLock Guard(&Lock);
  Ar->Serialize(Row, Len);
  Ar->Flush();
}

}  // namespace CameraTiming
