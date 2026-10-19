// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "ImagePackingAsyncTask.h"

#include <map>

#include "UnrealCamera.h"
#include "ThermalSensorModel.h"
#include "UnrealLogger.h"
#include "core_sim/message/image_message.hpp"
#include "UnrealLogger.h"

namespace projectairsim = microsoft::projectairsim;

void FImagePackingAsyncTask::DoWork() {
  std::map<projectairsim::ImageType, projectairsim::ImageMessage> ImageMessages;

  for (auto& [ImageRequest, RenderResult] : CaptureResults) {
    if (ImageRequest.ThermalSensor) {
      // One radiometric render becomes the display image and/or the
      // temperature image through the camera's sensor model.
      const int32 Count = RenderResult.Width * RenderResult.Height;
      std::vector<float> TemperatureK(Count);
      for (int32 I = 0; I < Count; ++I) {
        TemperatureK[I] = RenderResult.UnrealImageFloat32[I].R;
      }
      std::vector<uint8_t> Display, CentiKelvin;
      ImageRequest.ThermalSensor->Process(
          TemperatureK.data(), RenderResult.Width, RenderResult.Height,
          ImageRequest.ThermalFrame,
          ImageRequest.bThermalDisplay ? &Display : nullptr,
          ImageRequest.bThermalTemperature ? &CentiKelvin : nullptr);

      auto Add = [&](projectairsim::ImageType Type, const char* Encoding,
                     std::vector<uint8_t>&& Data) {
        ImageMessages.emplace(
            Type,
            projectairsim::ImageMessage(
                CapturedCameraTransform.timestamp_, RenderResult.Height,
                RenderResult.Width, Encoding, false, 1, std::move(Data),
                CapturedCameraTransform.translation_.x(),
                CapturedCameraTransform.translation_.y(),
                CapturedCameraTransform.translation_.z(),
                CapturedCameraTransform.rotation_.w(),
                CapturedCameraTransform.rotation_.x(),
                CapturedCameraTransform.rotation_.y(),
                CapturedCameraTransform.rotation_.z(), Annotations));
      };
      if (ImageRequest.bThermalDisplay) {
        if (ImageRequest.bCompress) {
          TArray<FColor> Pixels;
          Pixels.SetNumUninitialized(Count);
          for (int32 I = 0; I < Count; ++I) {
            Pixels[I] = FColor(Display[I * 3 + 2], Display[I * 3 + 1],
                               Display[I * 3], 255);
          }
          TArray<uint8> Compressed;
          UnrealCameraRenderRequest::CompressUsingImageWrapper(
              Pixels, RenderResult.Width, RenderResult.Height, Compressed);
          Add(projectairsim::ImageType::kInfrared, "PNG",
              std::vector<uint8_t>(Compressed.GetData(),
                                   Compressed.GetData() + Compressed.Num()));
        } else {
          Add(projectairsim::ImageType::kInfrared, "BGR", std::move(Display));
        }
      }
      if (ImageRequest.bThermalTemperature) {
        // uint16 centikelvin, little endian: 0.01 K steps up to 655 K.
        Add(projectairsim::ImageType::kTemperature, "16UC1",
            std::move(CentiKelvin));
      }
      continue;
    }

    if (ImageRequest.LensRemap) {
      const FLensRemap& Remap = *ImageRequest.LensRemap;
      if (RenderResult.UnrealImage.Num() > 0) {
        TArray<FColor> Output;
        if (ImageRequest.ImageType == projectairsim::ImageType::kScene) {
          Remap.Apply(RenderResult.UnrealImage, Output);
        } else {
          Remap.ApplyNearest(RenderResult.UnrealImage, Output);
        }
        RenderResult.UnrealImage = MoveTemp(Output);
      }
      if (RenderResult.UnrealImageFloat.Num() > 0) {
        TArray<FFloat16Color> Output;
        Remap.ApplyNearest(RenderResult.UnrealImageFloat, Output);
        RenderResult.UnrealImageFloat = MoveTemp(Output);
      }
      RenderResult.Width = Remap.Width;
      RenderResult.Height = Remap.Height;
    }

    // --- Make image response from the image request and render result ---
    ImageResponse ImgResponse;
    ImgResponse.CameraPosition = CapturedCameraTransform.translation_;
    ImgResponse.CameraOrientation = CapturedCameraTransform.rotation_;
    ImgResponse.TimeStamp = CapturedCameraTransform.timestamp_;

    bool bIsDepthImage =
        (ImageRequest.ImageType == projectairsim::ImageType::kDepthPerspective ||
         ImageRequest.ImageType == projectairsim::ImageType::kDepthPlanar);

    // Handle Depth image requests here.
    // 1. Currently, we do not support Compression for depth images
    // 2. We also do not support sending back Floats since our ImageResponse
    // Message is limited to uint8 for now. Instead, we convert Float16 from
    // Unreal to uint16 and then pack it in two uint8s that have to be unpacked
    // properly on client side. NOTE: This case also handles PixelsAsFloat
    // implicitly i.e. it ignores it and always sends back uint16 for depth
    if (bIsDepthImage && !ImageRequest.bCompress) {
      static constexpr float MAX_DEPTH = UINT16_MAX;  // TODO move to config?

      ImgResponse.ImageDataUInt8.resize(
          RenderResult.Width * RenderResult.Height * 2 * sizeof(uint8));

      uint8* DstPtr = ImgResponse.ImageDataUInt8.data();
      for (const auto& SrcPixel : RenderResult.UnrealImageFloat) {
        float DepthMilli = SrcPixel.R.GetFloat() * 1000;  // m -> mm
        DepthMilli = DepthMilli > MAX_DEPTH ? MAX_DEPTH : DepthMilli;
        auto DepthMilliUInt16 = static_cast<uint16_t>(DepthMilli);
        auto DepthMilliUInt8Ptr = reinterpret_cast<uint8_t*>(&DepthMilliUInt16);
        *DstPtr++ = *DepthMilliUInt8Ptr++;  // 1st byte of uint16 depth
        *DstPtr++ = *DepthMilliUInt8Ptr;    // 2nd byte of uint16 depth
      }
    }
    // Normal RGB images without compression or PixelsAsFloat requested
    else if (!bIsDepthImage && !ImageRequest.bPixelsAsFloat &&
             !ImageRequest.bCompress) {
      // Copy pixels to std::vector response:
      //   RenderResult.UnrealImage (TArray<FColor> BGRA aligned struct) ->
      //     ImgResponse.ImageDataUInt8 (std::vector<uint8_t> BGR)

      ImgResponse.ImageDataUInt8.resize(
          RenderResult.Width * RenderResult.Height * 3 * sizeof(uint8));

      uint8* DstPtr = ImgResponse.ImageDataUInt8.data();
      for (const auto& SrcPixel : RenderResult.UnrealImage) {
        *DstPtr++ = SrcPixel.B;
        *DstPtr++ = SrcPixel.G;
        *DstPtr++ = SrcPixel.R;
      }
    }
    // Normal RGB images with compression requested but not PixelsAsFloat
    else if (!bIsDepthImage && !ImageRequest.bPixelsAsFloat &&
             ImageRequest.bCompress) {
      // Do compression and copy to std::vector response:
      //   RenderResult.UnrealImage (TArray<FColor> BGRA aligned struct) ->
      //     CompressedArray (TArray<uint8> RGBA PNG) ->
      //     ImgResponse.ImageDataUInt8 (std::vector<uint8_t> RGBA PNG)

      // TODO Move PNG compression to inside sim?
      TArray<uint8> CompressedArray;
      UnrealCameraRenderRequest::CompressUsingImageWrapper(
          RenderResult.UnrealImage, RenderResult.Width, RenderResult.Height,
          CompressedArray);

      ImgResponse.ImageDataUInt8 = std::vector<uint8_t>(
          CompressedArray.GetData(),
          CompressedArray.GetData() + CompressedArray.Num());

    }
    // TODO: Since this does not respect PixelsAsFloat anyway, should we delete
    // the PixelsAsFloat option completely? Normal RGB images with FloatsAsPixel
    // requested but not compression
    else if (!bIsDepthImage && ImageRequest.bPixelsAsFloat &&
             !ImageRequest.bCompress) {
      // TODO Send image as exponent/mantissa/sign to be converted back to
      // float at client instead of converting to BGR8 here?
      // TODO Support compression with pixels as float?

      // Convert pixels from float16 to uint8 std::vector response:
      //   RenderResult.UnrealImageFloat (TArray<FFloat16Color> RGBA struct)->
      //     ImgResponse.ImageDataUInt8 (std::vector<uint8_t> BGR)
      ImgResponse.ImageDataUInt8.resize(
          RenderResult.Width * RenderResult.Height * 3 * sizeof(uint8));

      uint8* DstPtr = ImgResponse.ImageDataUInt8.data();
      for (const auto& SrcPixel : RenderResult.UnrealImageFloat) {
        *DstPtr++ = static_cast<uint8_t>(SrcPixel.B.GetFloat() * 255);
        *DstPtr++ = static_cast<uint8_t>(SrcPixel.G.GetFloat() * 255);
        *DstPtr++ = static_cast<uint8_t>(SrcPixel.R.GetFloat() * 255);
      }
    }
    // Unsupported combos right now
    // 1. PixelsAsFloat and Compression together
    // 2. DepthImages and Compression
    else {
      UnrealLogger::Log(projectairsim::LogLevel::kWarning,
                        TEXT("[FImagePackingAsyncTask] Unsupport combination "
                             "of camera options."));
    }

    ImgResponse.CameraName = ImageRequest.CameraName;
    ImgResponse.bPixelsAsFloat = ImageRequest.bPixelsAsFloat;
    ImgResponse.bCompress = ImageRequest.bCompress;
    ImgResponse.Width = RenderResult.Width;
    ImgResponse.Height = RenderResult.Height;
    ImgResponse.ImageType = ImageRequest.ImageType;

    // --- Make image message from the image response ---
    std::string ImgEncoding;
    if (!bIsDepthImage) {
      if (ImgResponse.bCompress) {
        ImgEncoding = "PNG";
      } else {
        ImgEncoding = "BGR";
      }
    } else {  // bIsDepthImage
      // 16-bit unsigned, 1 channel for depth in mm
      ImgEncoding = "16UC1";
    }

    ImageMessages.emplace(
        ImgResponse.ImageType,
        projectairsim::ImageMessage(
            ImgResponse.TimeStamp, ImgResponse.Height, ImgResponse.Width,
            ImgEncoding, false, 1, std::move(ImgResponse.ImageDataUInt8),
            ImgResponse.CameraPosition.x(), ImgResponse.CameraPosition.y(),
            ImgResponse.CameraPosition.z(), ImgResponse.CameraOrientation.w(),
            ImgResponse.CameraOrientation.x(),
            ImgResponse.CameraOrientation.y(),
            ImgResponse.CameraOrientation.z(),
            Annotations));
  }

  // Publish the whole pack of image messages
  Camera.PublishImages(std::move(ImageMessages));
}
