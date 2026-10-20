// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

// Procedural sound DSP for the play mode. Plain C++ with no Unreal types, so the same code runs inside
// UPlayAudio's synth component, the play.audio.dump console command, and the standalone render tool in
// Tools/render_wavs.main.

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace PlayAudioDsp {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;

// Linear gains of each voice before the master soft clip. Peaks of the rendered voices stay under 0.9.
constexpr float kMenuGain = 0.55f;
constexpr float kFlightGain = 0.30f;
constexpr float kRotorGain = 0.40f;
constexpr float kClickGain = 0.35f;
constexpr float kConfirmGain = 0.35f;

struct Stereo {
  float L = 0.f;
  float R = 0.f;
};

// xorshift32 white noise in [-1, 1).
class Noise {
 public:
  explicit Noise(uint32_t seed = 0x9E3779B9u) : state_(seed ? seed : 1u) {}
  float Next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return static_cast<float>(state_) * (2.0f / 4294967296.0f) - 1.0f;
  }
  // Uniform in [0, 1).
  float Unit() { return 0.5f * (Next() + 1.0f); }

 private:
  uint32_t state_;
};

class OnePoleLowpass {
 public:
  void SetCutoff(float hz, float sample_rate) {
    a_ = 1.0f - std::exp(-kTwoPi * hz / sample_rate);
  }
  float Process(float x) {
    y_ += a_ * (x - y_);
    return y_;
  }

 private:
  float a_ = 0.1f;
  float y_ = 0.f;
};

// Chamberlin state-variable filter; stable for cutoff well below sample_rate / 6.
class StateVariableFilter {
 public:
  void Set(float hz, float q, float sample_rate) {
    f_ = 2.0f * std::sin(kPi * std::min(hz, sample_rate / 6.0f) / sample_rate);
    damp_ = 1.0f / q;
  }
  void Process(float x) {
    low_ += f_ * band_;
    const float high = x - low_ - damp_ * band_;
    band_ += f_ * high;
  }
  float Low() const { return low_; }
  float Band() const { return band_; }

 private:
  float f_ = 0.1f, damp_ = 1.0f, low_ = 0.f, band_ = 0.f;
};

// Smooth gain ramp: a raised-cosine fade between 0 and 1 over a fixed time. Reversing mid-fade continues
// from the current level, so toggling never steps the gain.
class Fader {
 public:
  Fader(float seconds, float sample_rate) : step_(1.0f / (seconds * sample_rate)) {}
  void SetOn(bool on) { target_ = on ? 1.0f : 0.0f; }
  bool IsSilent() const { return pos_ <= 0.0f && target_ <= 0.0f; }
  float Next() {
    if (pos_ < target_) pos_ = std::min(target_, pos_ + step_);
    else if (pos_ > target_) pos_ = std::max(target_, pos_ - step_);
    return 0.5f - 0.5f * std::cos(kPi * pos_);
  }

 private:
  float step_;
  float pos_ = 0.f;
  float target_ = 0.f;
};

// Slow sine LFO returning [-1, 1].
class Lfo {
 public:
  Lfo(float hz, float sample_rate, float phase = 0.f)
      : inc_(hz / sample_rate), phase_(phase) {}
  float Next() {
    const float v = std::sin(kTwoPi * phase_);
    phase_ += inc_;
    if (phase_ >= 1.0f) phase_ -= 1.0f;
    return v;
  }

 private:
  float inc_, phase_;
};

// Menu bed: detuned low drones, a slow filtered noise swell and a distant sparse radio blip.
class MenuAmbient {
 public:
  explicit MenuAmbient(float sr)
      : sr_(sr),
        fade_(2.5f, sr),
        swell_lfo_(0.043f, sr, 0.2f),
        cutoff_lfo_(0.071f, sr, 0.6f),
        drone_lfo_a_(0.037f, sr, 0.1f),
        drone_lfo_b_(0.052f, sr, 0.7f),
        noise_(0x1234567u),
        blip_noise_(0x7654321u),
        delay_(static_cast<size_t>(0.37f * sr), 0.f) {
    for (float& p : phase_) p = 0.f;
    hiss_.SetCutoff(2600.0f, sr);
    hiss2_.SetCutoff(1800.0f, sr);
    blip_countdown_ = static_cast<int>(3.0f * sr);
  }

  void SetOn(bool on) { fade_.SetOn(on); }

  Stereo Next() {
    const float fade = fade_.Next();
    // Drones: a low open cluster; each pair is detuned by about 0.4 Hz so it beats slowly.
    static const float kFreqL[4] = {55.0f, 82.6f, 110.3f, 164.9f};
    static const float kFreqR[4] = {55.4f, 82.2f, 109.9f, 165.5f};
    static const float kAmp[4] = {0.45f, 0.22f, 0.14f, 0.06f};
    const float ma = 0.65f + 0.35f * drone_lfo_a_.Next();
    const float mb = 0.65f + 0.35f * drone_lfo_b_.Next();
    float l = 0.f, r = 0.f;
    for (int i = 0; i < 4; ++i) {
      phase_[i] += kFreqL[i] / sr_;
      phase_[i] -= std::floor(phase_[i]);
      phase_[4 + i] += kFreqR[i] / sr_;
      phase_[4 + i] -= std::floor(phase_[4 + i]);
      const float mod = (i & 1) ? mb : ma;
      l += kAmp[i] * mod * Tri(phase_[i]);
      r += kAmp[i] * mod * Tri(phase_[4 + i]);
    }
    l *= 0.5f;
    r *= 0.5f;

    // Noise swell: low-passed noise whose level and cutoff drift over about 20 s.
    const float swell = 0.5f + 0.5f * swell_lfo_.Next();
    const float cutoff = 180.0f + 520.0f * (0.5f + 0.5f * cutoff_lfo_.Next());
    svf_a_.Set(cutoff, 1.2f, sr_);
    svf_b_.Set(cutoff * 1.07f, 1.2f, sr_);
    svf_a_.Process(noise_.Next());
    svf_b_.Process(noise_.Next());
    const float swell_gain = 0.55f * swell * swell;
    l += swell_gain * svf_a_.Low();
    r += swell_gain * svf_b_.Low();

    // Radio blip: a short bandpassed two-tone chirp every 7 to 16 s, fed through a feedback delay so it
    // reads as far away.
    float blip = 0.f;
    if (blip_countdown_ > 0) {
      --blip_countdown_;
      if (blip_countdown_ == 0) {
        blip_pos_ = 0;
        blip_freq_ = 1500.0f + 900.0f * blip_noise_.Unit();
        blip_countdown_ = static_cast<int>((7.0f + 9.0f * blip_noise_.Unit()) * sr_);
      }
    }
    if (blip_pos_ >= 0) {
      const float t = static_cast<float>(blip_pos_) / sr_;
      const float len = 0.16f;
      if (t < len) {
        const float seg = (t < 0.07f) ? 1.0f : (t < 0.09f ? 0.0f : 0.8f);
        const float freq = blip_freq_ * ((t < 0.09f) ? 1.0f : 1.25f);
        const float env = std::sin(kPi * std::min(1.0f, t / len)) * seg;
        blip_phase_ += freq / sr_;
        blip_phase_ -= std::floor(blip_phase_);
        blip = 0.5f * env * std::sin(kTwoPi * blip_phase_);
        ++blip_pos_;
      } else {
        blip_pos_ = -1;
      }
    }
    const float delayed = delay_[delay_pos_];
    delay_[delay_pos_] = blip + 0.45f * delayed;
    delay_pos_ = (delay_pos_ + 1) % delay_.size();
    const float far = hiss_.Process(0.12f * (blip + delayed));
    l += far;
    r += 0.7f * far + 0.3f * hiss2_.Process(delayed * 0.1f);

    return {l * fade * kMenuGain, r * fade * kMenuGain};
  }

 private:
  static float Tri(float phase) { return 4.0f * std::fabs(phase - 0.5f) - 1.0f; }

  float sr_;
  Fader fade_;
  Lfo swell_lfo_, cutoff_lfo_, drone_lfo_a_, drone_lfo_b_;
  Noise noise_, blip_noise_;
  StateVariableFilter svf_a_, svf_b_;
  OnePoleLowpass hiss_, hiss2_;
  float phase_[8];
  std::vector<float> delay_;
  size_t delay_pos_ = 0;
  int blip_countdown_ = 0;
  int blip_pos_ = -1;
  float blip_freq_ = 1800.0f;
  float blip_phase_ = 0.f;
};

// Flight bed: a quiet wind of low-passed noise with slow gusts.
class FlightAmbient {
 public:
  explicit FlightAmbient(float sr)
      : sr_(sr),
        fade_(1.5f, sr),
        gust_a_(0.11f, sr, 0.3f),
        gust_b_(0.27f, sr, 0.8f),
        noise_(0xABCDEFu) {}

  void SetOn(bool on) { fade_.SetOn(on); }

  Stereo Next() {
    const float fade = fade_.Next();
    const float gust = 0.5f + 0.5f * (0.7f * gust_a_.Next() + 0.3f * gust_b_.Next());
    const float cutoff = 250.0f + 650.0f * gust;
    svf_l_.Set(cutoff, 0.9f, sr_);
    svf_r_.Set(cutoff * 0.93f, 0.9f, sr_);
    svf_l_.Process(noise_.Next());
    svf_r_.Process(noise_.Next());
    const float level = (0.35f + 0.65f * gust) * 0.9f;
    return {svf_l_.Low() * level * fade * kFlightGain,
            svf_r_.Low() * level * fade * kFlightGain};
  }

 private:
  float sr_;
  Fader fade_;
  Lfo gust_a_, gust_b_;
  Noise noise_;
  StateVariableFilter svf_l_, svf_r_;
};

// Short dry tick: a decaying 1.8 kHz sine plus a noise transient.
class UiClick {
 public:
  explicit UiClick(float sr) : sr_(sr), noise_(0x5151u) {}
  void Trigger() { pos_ = 0; phase_ = 0.f; }
  bool Active() const { return pos_ >= 0; }

  float Next() {
    if (pos_ < 0) return 0.f;
    const float t = static_cast<float>(pos_) / sr_;
    if (t > 0.05f) {
      pos_ = -1;
      return 0.f;
    }
    ++pos_;
    const float attack = std::min(1.0f, t / 0.0005f);
    phase_ += 1800.0f / sr_;
    phase_ -= std::floor(phase_);
    const float tone = std::sin(kTwoPi * phase_) * std::exp(-t * 220.0f);
    const float tick = noise_.Next() * std::exp(-t * 900.0f) * 0.5f;
    return attack * (0.8f * tone + tick) * kClickGain;
  }

 private:
  float sr_;
  Noise noise_;
  int pos_ = -1;
  float phase_ = 0.f;
};

// Two soft sine blips, a fourth apart, rising.
class UiConfirm {
 public:
  explicit UiConfirm(float sr) : sr_(sr) {}
  void Trigger() { pos_ = 0; phase_[0] = phase_[1] = 0.f; }
  bool Active() const { return pos_ >= 0; }

  float Next() {
    if (pos_ < 0) return 0.f;
    const float t = static_cast<float>(pos_) / sr_;
    const float second = 0.085f;
    if (t > second + 0.22f) {
      pos_ = -1;
      return 0.f;
    }
    ++pos_;
    float out = 0.f;
    out += Blip(0, 660.0f, t);
    if (t >= second) out += Blip(1, 880.0f, t - second);
    return out * kConfirmGain;
  }

 private:
  float Blip(int i, float freq, float t) {
    phase_[i] += freq / sr_;
    phase_[i] -= std::floor(phase_[i]);
    const float attack = std::min(1.0f, t / 0.004f);
    const float s = std::sin(kTwoPi * phase_[i]) + 0.15f * std::sin(2.0f * kTwoPi * phase_[i]);
    return 0.8f * attack * std::exp(-t * 18.0f) * s;
  }

  float sr_;
  int pos_ = -1;
  float phase_[2] = {0.f, 0.f};
};

// Quad buzz: four slightly detuned rotors, each a harmonic stack at the blade-pass rate, plus a little
// noise that opens with throttle. Pitch and level follow the smoothed throttle. Throttle below zero
// fades the voice out.
class RotorLoop {
 public:
  explicit RotorLoop(float sr) : sr_(sr), fade_(0.25f, sr), noise_(0x600Du) {
    for (float& p : phase_) p = 0.f;
  }

  void SetThrottle(float throttle01) {
    on_ = throttle01 >= 0.0f;
    if (on_) target_ = std::min(throttle01, 1.0f);
    fade_.SetOn(on_);
  }

  float Next() {
    const float fade = fade_.Next();
    if (fade <= 0.0f && !on_) return 0.f;
    // 30 ms smoothing keeps pitch changes glide-like and removes zipper steps.
    throttle_ += (target_ - throttle_) * (1.0f - std::exp(-1.0f / (0.03f * sr_)));
    const float base = 95.0f + 235.0f * throttle_;
    static const float kDetune[4] = {1.000f, 1.013f, 0.989f, 1.026f};
    float sum = 0.f;
    for (int r = 0; r < 4; ++r) {
      const float f0 = base * kDetune[r];
      phase_[r] += f0 / sr_;
      phase_[r] -= std::floor(phase_[r]);
      const int harmonics = std::min(12, static_cast<int>(0.45f * sr_ / f0));
      float v = 0.f;
      for (int h = 1; h <= harmonics; ++h) {
        // Blade-pass harmonics fall off steeply; the second is boosted for the buzz character.
        const float amp = (h == 2 ? 1.0f : 0.7f) / (static_cast<float>(h) * std::sqrt(static_cast<float>(h)));
        v += amp * std::sin(kTwoPi * phase_[r] * static_cast<float>(h));
      }
      sum += v;
    }
    sum *= 0.25f;

    noise_lp_.SetCutoff(1200.0f + 3500.0f * throttle_, sr_);
    const float hiss = noise_lp_.Process(noise_.Next()) * (0.10f + 0.25f * throttle_);
    const float level = 0.18f + 0.62f * throttle_;
    return (sum * level + hiss * level) * fade * kRotorGain;
  }

 private:
  float sr_;
  Fader fade_;
  Noise noise_;
  OnePoleLowpass noise_lp_;
  float phase_[4];
  float throttle_ = 0.f;
  float target_ = 0.f;
  bool on_ = false;
};

// Cross-thread control block. The game thread writes; the audio thread reads. Triggers are counters, so
// two clicks between audio callbacks are both heard.
struct Controls {
  std::atomic<bool> menu_on{false};
  std::atomic<bool> flight_on{false};
  std::atomic<float> rotor_throttle{-1.0f};
  std::atomic<uint32_t> click_count{0};
  std::atomic<uint32_t> confirm_count{0};
};

inline float SoftClip(float x) {
  // tanh is linear to within 3 percent below 0.25 and bounded by 1, so the sum never clips.
  return std::tanh(x);
}

// All voices behind one master soft clip.
class Mixer {
 public:
  Mixer(float sample_rate, const Controls& controls)
      : controls_(controls),
        menu_(sample_rate),
        flight_(sample_rate),
        click_(sample_rate),
        confirm_(sample_rate),
        rotor_(sample_rate) {}

  // Renders `frames` interleaved stereo frames.
  void Render(float* out, int frames) {
    menu_.SetOn(controls_.menu_on.load(std::memory_order_relaxed));
    flight_.SetOn(controls_.flight_on.load(std::memory_order_relaxed));
    rotor_.SetThrottle(controls_.rotor_throttle.load(std::memory_order_relaxed));
    const uint32_t clicks = controls_.click_count.load(std::memory_order_relaxed);
    if (clicks != seen_clicks_) { seen_clicks_ = clicks; click_.Trigger(); }
    const uint32_t confirms = controls_.confirm_count.load(std::memory_order_relaxed);
    if (confirms != seen_confirms_) { seen_confirms_ = confirms; confirm_.Trigger(); }

    for (int i = 0; i < frames; ++i) {
      const Stereo m = menu_.Next();
      const Stereo f = flight_.Next();
      const float ui = click_.Next() + confirm_.Next();
      const float rotor = rotor_.Next();
      out[2 * i] = SoftClip(m.L + f.L + ui + rotor);
      out[2 * i + 1] = SoftClip(m.R + f.R + ui + rotor);
    }
  }

 private:
  const Controls& controls_;
  MenuAmbient menu_;
  FlightAmbient flight_;
  UiClick click_;
  UiConfirm confirm_;
  RotorLoop rotor_;
  uint32_t seen_clicks_ = 0;
  uint32_t seen_confirms_ = 0;
};

// The sounds `RenderSound` can produce, named for the dump file suffix.
enum class Sound { Menu, Flight, Click, Confirm, RotorLow, RotorHigh };

inline const char* SoundSuffix(Sound s) {
  switch (s) {
    case Sound::Menu: return "_menu.wav";
    case Sound::Flight: return "_flight.wav";
    case Sound::Click: return "_click.wav";
    case Sound::Confirm: return "_confirm.wav";
    case Sound::RotorLow: return "_rotor_low.wav";
    case Sound::RotorHigh: return "_rotor_high.wav";
  }
  throw std::logic_error("unknown Sound");
}

// Renders one sound through the same Mixer the game uses. One-shots get a fixed 0.6 s window; the loops
// get `seconds`. Returns interleaved stereo floats.
inline std::vector<float> RenderSound(Sound sound, float seconds, float sample_rate) {
  Controls controls;
  Mixer mixer(sample_rate, controls);
  switch (sound) {
    case Sound::Menu: controls.menu_on = true; break;
    case Sound::Flight: controls.flight_on = true; break;
    case Sound::Click: controls.click_count = 1; seconds = 0.6f; break;
    case Sound::Confirm: controls.confirm_count = 1; seconds = 0.6f; break;
    case Sound::RotorLow: controls.rotor_throttle = 0.2f; break;
    case Sound::RotorHigh: controls.rotor_throttle = 0.9f; break;
  }
  const int frames = static_cast<int>(seconds * sample_rate);
  std::vector<float> out(static_cast<size_t>(frames) * 2);
  // Rendered in the 512-frame blocks the audio thread uses.
  for (int done = 0; done < frames; done += 512) {
    mixer.Render(out.data() + 2 * done, std::min(512, frames - done));
  }
  return out;
}

// Writes interleaved stereo float samples as 16-bit PCM WAV. Throws if the file cannot be written.
inline void WriteWav(const std::string& path, const std::vector<float>& stereo, int sample_rate) {
  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open WAV for writing: " + path);
  const uint32_t data_bytes = static_cast<uint32_t>(stereo.size() * 2);
  auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
  auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
  f.write("RIFF", 4); u32(36 + data_bytes); f.write("WAVE", 4);
  f.write("fmt ", 4); u32(16); u16(1); u16(2); u32(static_cast<uint32_t>(sample_rate));
  u32(static_cast<uint32_t>(sample_rate) * 4); u16(4); u16(16);
  f.write("data", 4); u32(data_bytes);
  for (float s : stereo) {
    const float c = std::max(-1.0f, std::min(1.0f, s));
    u16(static_cast<uint16_t>(static_cast<int16_t>(std::lrintf(c * 32767.0f))));
  }
  if (!f) throw std::runtime_error("failed writing WAV: " + path);
}

}  // namespace PlayAudioDsp
