## Supported imaging/capture camera customizations

### Camera-level parameters
Other than the parameters common to all sensors, the following apply to all capture types in a camera.
| Parameter                       | Value-range                       | Description & notes                                          |
| ------------------------------- | --------------------------------- | ------------------------------------------------------------ |
| capture-interval                | > 0                               | Interval in seconds in between subsequent captures           |
| annotation-settings                    | json                             | (Optional) See Annotation Settings section below. Off by default.         |

### Annotation settings
You can enable 2D and 3D bounding box annotations on the images by including `annotation-settings` object as part of your camera settings.

For example, the example below will produce bounding boxes whenever the objects `Cone_5` and `OrangeBall` are in the camera's view.

```json
      "annotation-settings": {
        "object-ids": [
          "Cone_5",
          "OrangeBall"
        ],
        "enabled": true,
        "bbox2D-settings":
        {
          "alignment": "axis" // axis or oriented
        },
        "bbox3D-settings":
        {
          "alignment": "oriented" // axis or oriented
        }
      }
```

The `alignment` of each kind of bounding box will determine whether they should be aligned to the axis of the coordinate system they exist on (world-axis for 3D bboxes, image-axis for 2D bboxes) or whether they should have an orientation (object-oriented for 3D bboxes, or [not yet supported:] tightest possible box for 2D bbox).

#### Annotations output
If annotations output is enabled, as part of the *image message* there will be an `annotations` list. For example:
```json
{
  "data": ...,
  "time_stamp": ...,
  ...,
  "annotations": [
    "object_id": "Cone_5",
    "bbox2d": {
      "center": { "x": 40, "y": 30},
      "size":  { "x": 40, "y": 30},
    }
    "bbox3d": {
      "center": { "x": 40, "y": 30, "z": 20 },
      "size":  { "x": 40, "y": 30, "z": 20 },
      "quaternion":  { "w": 1, "x": 40, "y": 30, "z": 20 },
    },
    "bbox3d_in_image_space": [
    {
      "x": 822,
      "y": 259
    },
    ... // 8 2D points in total
    ],
    "bbox3d_in_projection_space": [
      {
        "x": 0,
        "y": 0,
        "z": 0
      },
      ... // 8 3D points in total
    ],
    ... // any more objects in view
  ]
}
```
If annotations are disabled, this list will be empty.

`bbox3d_in_image_space`, is a 3D bounding box projected onto the image plane, so you can draw it on the image. It's made up of 8 2D points representing the corners of the 3D bbox.
`bbox3d_in_projection_space` is a 3D bounding box in NDC space - a projection space representing the camera frustum, in between image-space and world-space. Useful for some autonomy scenarios. It's made up of 8 3D points represeting the corners of the 3D bbox.

### Capture-level parameters
A single camera can output multiple capture types. The following parameters describe a specific capture in a camera.

| Parameter                       | Value-range                       | Description & notes                                          |
| ------------------------------- | --------------------------------- | ------------------------------------------------------------ |
| image-type                      | index 0~8                         | Index for camera type: 0 = RGB Scene, 1 = Depth Planar, 2 = Depth Perspective, 3 = Segmentation, 4 = Depth Visualization, 5 = DisparityNormalized, 6 = SurfaceNormals, 7 = Infrared (thermal display image), 8 = Temperature (thermal, radiometric) |
| width                           | > 0                               | Width of the camera image                                    |
| height                          | > 0                               | Height of the camera image                                   |
| fov-degrees                     | [5, 170]                          | Field of View of the camera                                  |
| capture-enabled                 | True/False                        | Enable capturing and sending the full raw images to the client through pub/sub topics and/or req/resp APIs. |
| streaming-enabled               | True/False                        | Enable streaming a real-time video feed of this camera capture. See the [Camera Streaming](camera_streaming.md) page for more details. |
| pixels-as-float                 | True/False                        | Use float for pixel data                                     |
| compress                        | True/False                        | Apply PNG lossless compression to the captured image data sent to the client with `capture-enabled` set to True. *Note: Compressing/decompressing adds processing time that may slow down the image transfer rate.* |
| auto-exposure-method            | 0: histogram, 1: basic, 3: manual | **Auto Exposure Histogram** constructs a 64 bin histogram enabling finer control over auto exposure with advanced settings.  **Auto Exposure Basic** is a faster method that computes single values by down sampling. **Manual** enables the use of **Camera** settings within the Post Process Volume to control exposure rather than the Exposure settings. |
| auto-exposure-speed             |                                   | The speed at which the adaptation occurs from a dark environment to a bright environment. |
| auto-exposure-max-brightness    | x > 1.0 && x >= min-brightness    | The maximum brightness for auto exposure that limits the upper brightness the eye can adapt within. **NOTE**: If Min Brightness = Max Brightness, auto exposure is disabled |
| auto-exposure-min-brightness    | 0.0 < x < max-brightness          | The minimum brightness for auto exposure that limits the lower brightness the eye can adapt within. **NOTE:** If Min Brightness = Max Brightness, auto exposure is disabled |
| auto-exposure-low-percent       | [0, 100]                          | The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color. The value defines the lower percentage of the histogram that is used to find the average scene luminance. **NOTE:** Values in the range 70-80 give the best results. |
| auto-exposoure-high-percent     | [0, 100]                          | The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color. The value defines the upper percentage of the histogram that is used to find the average scene luminance. ***NOTE:*** Values in the range 70-80 give the best results. |
| auto-exposure-histogram-log-min |                                   | Defines the lower bounds for the brightness range of the generated histogram when using the  HDR (Eye Adaptation) visualization mode;log-min value for histogram of scene colors; E.g.:0.8 ==> 80% of the screen pixels are darker than the luminance value A |
| auto-exposure-histogram-log-max |                                   | log-max value for histogram of scene colors; E.g.:0.95 ==>  95% of the screen pixels are darker than the luminance value B. Current luminance value (C) = avg(A, B) |
| motion-blur-amount              | [0, 1]                            | Blurs objects based on it's motion determined using velocity maps. A value of 0.25 - 0.5 looks reasonable. Avoid 1.0 |
| target-gamma                    |                                   | Target gamma to be used by the render target (UTextureRenderTarget2D) |
| max-depth-meters              | > 0.0                             | Maximum depth for Depth Visualization. Objects beyond the maximum depth will be true white, while close objects will be completely black. |
| lens                          | json                              | (Optional) Lens model of the capture, see Lens models below. Pinhole by default. |
| thermal                       | json                              | (Optional, image type 7) Sensor model of the thermal camera, see [Thermal camera](thermal_camera.md). |

| Image Type | Description |
| Scene | The regular camera image. |
| Depth Planar | Depth measured from the camera plane, in meters. |
| Depth Perspective | Depth measured in a line from the camera point, in meters. |
| Segmentation | The ground truth segmentation of the scene, i.e. each object is a different color.|
| Depth Visualization | Image that helps visualize depth. |
| DisparityNormalized | Uses the ground truth depth to simulate a stereo disparity image with a single camera. The baseline distance is assumed to be 0.25m. Values are normalized to a 0-1 scale. |
| SurfaceNormals | Surfaces are colored according to their normals. RGB corresponds to XYZ, and values are normalized to a 0-1 scale. |
| Infrared | Long-wave infrared (thermal) camera: an 8-bit display image (white-hot, black-hot or ironbow) after the camera's optics, noise and automatic gain control. See [Thermal camera](thermal_camera.md). |
| Temperature | The same thermal render as radiometric `16UC1`: apparent temperature in centikelvin (divide by 100 for kelvin). Types 7 and 8 come from one render and share size, field of view and sensor model. |


### Lens models

Unreal renders pinhole images. Without `lens`, or with `"model": "pinhole"`, the capture is an exact pinhole with
`fov-degrees` horizontal field of view and no distortion. `scene_camera_info` then reports `plumb_bob` with zero
coefficients, `K = [f 0 cx; 0 f cy; 0 0 1]` with `f = (width/2) / tan(fov/2)`, and the principal point at the image centre,
`cx = (width-1)/2` and `cy = (height-1)/2` (OpenCV/ROS pixel-centre convention).

`"model": "kannala-brandt"` gives a fisheye: a ray at angle theta from the optical axis lands at radius
`f * theta_d` from the principal point, with `theta_d = theta (1 + k1 theta^2 + k2 theta^4 + k3 theta^6 + k4 theta^8)`.
This is OpenCV's `cv::fisheye` model, called `equidistant` in ROS. `f` is chosen so that the left and right image edges
are `fov-degrees`/2 off-axis, and the principal point is the image centre. The camera renders a wider pinhole image and
remaps every output pixel along its ray, averaging four bilinear samples for the scene image and taking the nearest
pixel for depth, segmentation and the other types. The remap is built once per capture. `scene_camera_info` reports
`distortion_model: "equidistant"`, `D = [k1, k2, k3, k4, 0]` and the `K` above, so a client can rectify keypoints with
`cv::fisheye::undistortPoints(K, D)`.

| Parameter              | Value-range        | Description & notes |
| ---------------------- | ------------------ | ------------------- |
| model                  | pinhole, kannala-brandt | Lens model. |
| distortion             | [k1, k2, k3, k4]   | Kannala-Brandt coefficients; [0, 0, 0, 0] (default) is an ideal equidistant fisheye. |
| max-incidence-degrees  | (0, 89), default 80 | Rays further off-axis render black, like the outside of an image circle. It also bounds the rendered pinhole image. |
| source-sampling        | > 0, default 1     | Rendered pinhole pixels per output pixel at the image centre. The rendered image is capped at 4096 px per side, which lowers this for very wide lenses. For a pinhole only 1 or 2, and 2 supersamples (see below). |

The rendered image is larger than the output, and that costs render time. At 1280x720: 90 deg is 2.0x the output pixels, 110 deg 3.2x, 120 deg 4.6x, and from about 140 deg the
4096 px cap applies (centre sampling 0.79). Annotations (bounding boxes) are computed for the rendered pinhole image
and are not remapped. A runtime `fov-degrees` change is ignored for a lens capture (the remap is built once).

```json
"capture-settings": [{
  "image-type": 0, "width": 1280, "height": 720, "fov-degrees": 120,
  "capture-enabled": true, "compress": false,
  "lens": {"model": "kannala-brandt", "distortion": [-0.013, 0.021, -0.017, 0.0049]}
}]
```

### Supersampling

Unreal's screen percentage does not apply to scene captures, so a capture cannot be supersampled by
`r.ScreenPercentage`. `"lens": {"source-sampling": 2}` on a pinhole capture renders the image at twice the width and
height (same field of view, so `scene_camera_info` is unchanged) and makes each output pixel the mean of its 2x2 source
pixels. Other values throw: the remap's four taps are an exact box only at 2, and 1.5 came out softer than no
supersampling. It renders 4x the pixels, and temporal AA runs on the large image. Fine detail of about one output
pixel (distant window grids, glass glints) aliases less. In CitySample at 1280x720 on an M3 Max it cut the uncapped
camera rate from 39 to 17 fps and left the turning stability unchanged. A runtime `fov-degrees` change is ignored, as
for a lens capture.

## Sample config

```json
{
"sensors": [
    {
      "id": "Down",
      "type": "camera",
      "enabled": true,
      "parent-link": "Frame",
      "capture-interval": 0.05,
      "capture-settings": [
        {
          "image-type": 0,
          "width": 640,
          "height": 480,
          "fov-degrees": 90,
          "capture-enabled": true,
          "streaming-enabled": false,
          "pixels-as-float": false,
          "compress": false,
          "target-gamma": 2.5,
          "auto-exposure-speed": 0.25,
          "auto-exposure-bias": 0.0,
          "auto-exposure-max-brightness": 1.00,
          "auto-exposure-min-brightness": 0.7,
          "auto-exposure-low-percent": 0.7,
          "auto-exposure-high-percent": 0.9,
          "auto-exposure-histogram-log-min": -8,
          "auto-exposure-histogram-log-max": 4,
          "motion-blur-amount": 0.3
        },
    }
}
```

---

Copyright (C) Microsoft Corporation.  
Copyright (C) 2025 IAMAI CONSULTING CORP.

MIT License. All rights reserved.
