# Reference — job configuration file (YAML)

## Table of contents

- [1. How a job file works](#section-1-role)
- [2. Root fields](#section-2-root)
- [3. `metadata`](#section-3-metadata)
  - [3.1 `substitutions`](#metadata-substitutions)
  - [3.2 String value (constant)](#substitutions-string)
  - [3.3 Frame map value (dictionary)](#substitutions-frame-map)
  - [3.4 Automatic tokens (slate / burn-in text)](#metadata-automatic-tokens)
- [4. `plans` (shot list)](#section-4-plans)
  - [4.1 `id` (shot identifier)](#plans-id)
  - [4.2 `input_colorspace` (source color)](#plans-input-colorspace)
  - [4.3 `sequence` (image sequence)](#plans-sequence)
    - [4.3.1 `handles` (optional head/tail frames)](#plans-sequence)
  - [4.4 `audio` (optional guide track)](#plans-audio)
  - [4.5 `timecode` (optional SMPTE clock)](#plans-timecode)
- [5. `color` (OpenColorIO)](#section-5-color)
- [6. `layout` (page layout)](#section-6-layout)
  - [6.1 `canvas`](#layout-canvas)
  - [6.2 `pixel_aspect`](#layout-pixel-aspect)
  - [6.3 `image` (source framing in canvas)](#layout-image)
  - [6.4 `background`](#layout-background)
  - [6.5 `burn_ins`](#layout-burn-ins)
  - [6.6 `slate`](#layout-slate)
- [7. `output`](#section-7-output)
  - [7.1 `output` object structure](#output-structure)
  - [7.2 `videos[]` entries](#output-videos)
  - [7.2.0 `signal`](#output-signal)
  - [7.2.1 `codec_options` for `h264`](#output-codec-options)
  - [7.2.2 `codec_options` for `mjpeg`](#output-codec-options-mjpeg)
  - [7.2.3 `codec_options` for `dnxhd`](#output-codec-options-dnxhd)
  - [7.2.4 `codec_options` for `prores`](#output-codec-options-prores)
  - [7.3 `image_sequences[]` entries](#output-image-sequences)
  - [7.3.1 `format_options` for `.png`](#output-format-options-png)
  - [7.3.2 `format_options` for `.jpg` / `.jpeg`](#output-format-options-jpeg)
  - [7.3.3 `format_options` for `.tif` / `.tiff`](#output-format-options-tiff)
  - [7.3.4 `format_options` for `.exr`](#output-format-options-exr)
  - [7.3.5 `format_options` for `.heif` / `.heic` / `.avif`](#output-format-options-heif)
- [8. Write your own job](#section-8-write-your-own)
- [9. See also](#section-9-see-also)

---

<a id="section-1-role"></a>

## 1. How a job file works

Imagine you have a stack of pictures (one picture per frame of a shot). DailyBoy’s job is to put each picture onto a **page**, maybe write words on it, maybe show a title card first, then save a **movie** and/or a new stack of pictures.

The job file answers four questions:

1. **Which frames are we using?** (`plans`) — specifies the input files, their frame ranges, and their original color space.
2. **What does the page look like?** (`layout`) — defines the page’s width and height, how the input image is placed, background/UI colors (display RGB), which burn-ins to display, and whether to show a slate
3. **How does color management work?** (`color`) — specifies the OpenColorIO configuration DailyBoy should use for color processing, as well as the color spaces applied when saving each output file.
4. **What do we export, and where?** (`output`) — defines what files (movies, image sequences, or both) DailyBoy should write, and where to save them.

---

<a id="section-2-root"></a>

## 2. Root fields

These are the main root fields of the job file. Each one defines a necessary part of the configuration for DailyBoy to perform its tasks.

| Field              | Required | Type                                  | Description |
| ------------------ | -------- | ------------------------------------- | ----------- |
| `dailyboy_version` | yes      | integer (`1`)                         | Version of the job file format. Must always be `1`. DailyBoy refuses any other value. |
| `metadata`         | no       | object                                | Optional substitutions ("name tags") you define (e.g. `shot`, `project`, folder paths, colorspace). Skip the whole block if unused. If present, it must include at least one key under `substitutions`. Used for string template expansion in paths, text overlays, etc. (see §3). |
| `plans`            | yes      | array of objects (at least 1)         | List of input shots/plans to process. For now, only one shot is allowed per job (start with a dash `-`). |
| `color`            | yes      | object                                | OpenColorIO color management: `config.ocio`, working space, optional context. DisplayView lives on each output. |
| `layout`           | yes      | object                                | Page layout definition: canvas size, image placement, background color, overlays/burn-ins, and slate (title card) configuration. |
| `output`           | yes      | object                                | Defines what outputs DailyBoy writes (movies, image sequences, or both). At least one output must be enabled (`enabled: true`). |

---

<a id="section-3-metadata"></a>

## 3. `metadata`

The `metadata` section provides named substitutions you can use throughout your job file. Use `{name}` syntax in paths or text fields to reference these values.

You may omit the entire `metadata` block if you do not need any substitutions. If present, the block must provide a `substitutions` object with at least one entry.

| Sub-field       | Required (when `metadata` is present) | Type               | Description |
| --------------- | -------------------------------------- | ------------------ | ----------- |
| `substitutions` | yes                                    | object (min 1 key) | The dictionary of names and their values. |

Define simple names without spaces (such as `shot`, `dailies_root`, `project`) as keys in `substitutions`. DailyBoy replaces `{name}` in any string field with the corresponding value.

If a placeholder such as `{oops}` does not exist in `substitutions`, DailyBoy issues a warning and leaves the placeholder as-is (or blank, depending on context). If your key matches a built-in name from §3.4, the built-in value takes precedence.

<a id="metadata-substitutions"></a>

### 3.1 `substitutions`

| Entry part | Type                             | Description |
| ---------- | -------------------------------- | ----------- |
| **Key**    | string                           | Name of the substitution. In the job file, referenced as `{key}`. |
| **Value**  | quoted string or object (frame map) | Quoted text to substitute, or a mapping for frame-specific values. |

A **key** is the name within curly braces used to reference a substitution, for example `{shot}`.

A **value** is:

- a **string**: provides the same text for the whole job (e.g., a shot name, folder, or colorspace);
- a **frame map**: an object specifying per-frame text, typically for overlays like burn-ins and slates.

<a id="substitutions-string"></a>

### 3.2 String value (constant)

A string value is used for constant substitutions. The value is the same for all frames.

```yaml
metadata:
  substitutions:
    project: "MY_SHOW"
    dailies_root: "/mnt/artist/dailies/sh010"
    working_colorspace: "ACEScg"
```

For example, `{project}` will always be replaced with `MY_SHOW`, and `{dailies_root}` will always be replaced with `/mnt/artist/dailies/sh010`. This allows you to write paths like `path: "{dailies_root}/review.mov"` to avoid repeating the full path.

<a id="substitutions-frame-map"></a>

### 3.3 Frame map value (dictionary)

A frame map value is used when the text should vary according to the frame number, such as changing overlay text or slate content depending on the frame.

| Frame map part | Type   | Description |
| -------------- | ------ | ----------- |
| **Map key**    | string | Quoted frame number (e.g. `"1001"`) or frame range (e.g. `"1005-1010"`). |
| **Map value**  | string | Quoted substitution text for those frames. |

If a frame does not match any key or range, nothing is substituted for that frame.

Example:

```yaml
metadata:
  substitutions:
    note_frame:
      "1001": "Start"
      "1005-1010": "Action"
      "1020": "End"
```

For a burn-in field with `template: "{note_frame}"`, the value will be:
- `"Start"` for frame 1001
- `"Action"` for frames 1005 to 1010
- `"End"` for frame 1020
- empty string for frames not listed

<a id="metadata-automatic-tokens"></a>

### 3.4 Automatic tokens (slate / burn-in text)

DailyBoy provides certain automatic tokens that can be used in templates for slate and burn-ins. These do not need to be defined in `metadata`. If a user defines a substitution with the same name, DailyBoy’s value takes precedence.

| Token           | Meaning |
| --------------- | ------- |
| `{frame}`       | The current frame number being processed. |
| `{frame_start}` | The starting frame of the shot as set in `plans[].sequence.frame_start`. |
| `{frame_end}`   | The ending frame of the shot as set in `plans[].sequence.frame_end`. |
| `{source_file}` | The resolved file path for the given image frame. |
| `{plan_id}`     | The value of the `id` field for that plan (see §4.1). |
| `{timecode}`    | SMPTE clock for this frame when `plans[].timecode` is set (see §4.5). Absent timecode → unknown-token warning. |

---

<a id="section-4-plans"></a>

## 4. `plans` (shot list)

The `plans` section defines which shot(s) will be processed in this job. Currently, one plan (shot) per job is supported, specified as a single item in a YAML list.

Each plan is required to specify:
- a unique identifier (`id`) — quoted non-empty string
- the colorspace of its images (`input_colorspace`) — quoted non-empty string
- the image sequence location and range (`sequence`), with `sequence.path` as a quoted non-empty string

Optional `audio.path` (when the `audio` block is present) must also be a quoted non-empty string. You may also add handles and timecode for more advanced cases.

<a id="plans-id"></a>

### 4.1 `id`

| Field | Required | Type                              | Description                                 |
| ----- | -------- | --------------------------------- | ------------------------------------------- |
| `id`  | yes      | quoted string (non-empty)         | Unique, non-empty identifier for this shot. |

The `id` is required, must be a **quoted** non-empty YAML string (e.g. `"plate"`), and is used for `{plan_id}` substitution tokens. Unquoted or empty values fail with `plans[].id: must be a quoted non-empty string (e.g. "plate").`.

<a id="plans-input-colorspace"></a>

### 4.2 `input_colorspace`

| Field              | Required | Type                                              | Description                                                       |
| ------------------ | -------- | ------------------------------------------------- | ----------------------------------------------------------------- |
| `input_colorspace` | yes      | quoted string (OCIO colorspace name, non-empty)   | Plate image colorspace as defined in your OCIO config.            |

Must be a **quoted** non-empty YAML string that exactly matches a colorspace in your OCIO configuration (e.g. `"ACEScg"`, `"Linear Rec.709 (sRGB)"`, `"AlexaV3LogC"`). Invalid values fail with `plans[].input_colorspace: must be a quoted non-empty string.`.

<a id="plans-sequence"></a>

### 4.3 `sequence`

| Sub-field     | Required | Type                      | Description                                                 |
| ------------- | -------- | ------------------------- | ----------------------------------------------------------- |
| `path`        | yes      | quoted string (non-empty) | File pattern for the image sequence.                        |
| `frame_start` | yes      | integer                   | First hero frame (inclusive, used for overlays/tokens).     |
| `frame_end`   | yes      | integer                   | Last hero frame (inclusive).                                |
| `handles`     | no       | object                    | Optional extra source frames before/after hero range.       |

**Sequence Paths:**
- `path` must be a **quoted** non-empty YAML string (e.g. `"plate.%04d.png"`). Unquoted or empty values fail with `plans[].sequence.path: must be a quoted non-empty string (e.g. "plate.%04d.png").`.
- Patterns (`%04d`, `####`, `@@@@`, `$F4`, or explicit ranges like `file.1001-1050.exr`) are supported.
- You can embed substitutions like `{dailies_root}` in the path.
- `frame_start` and `frame_end` are the main (hero) range. Slate/burn-in tokens `{frame_start}` and `{frame_end}` use these values.

**Handles:** (advanced)
Optional `handles` allow including additional plates before the first and/or after the last hero frame.

#### 4.3.1 Handles structure

| `handles` sub-field | Required | Type          | Default | Description                                                                            |
| ------------------- | -------- | ------------- | ------- | -------------------------------------------------------------------------------------- |
| `head`              | no       | integer ≥ 0   | `0`     | Frames before `frame_start` to include (`frame_start - head` ... `frame_start - 1`).   |
| `tail`              | no       | integer ≥ 0   | `0`     | Frames after `frame_end` to include (`frame_end + 1` ... `frame_end + tail`).          |

- If `handles` is omitted, or a side is omitted, that side defaults to `0` (hero range only for that side).
- If handles are set, **the processed range becomes** `frame_start - handles.head` through `frame_end + handles.tail`.
- If any frame in this full range is missing, that is a hard error.
- `head` / `tail` must be unquoted integers ≥ 0. Invalid values fail with `plans[].sequence.handles.head: must be an unquoted integer >= 0 (e.g. 8).` or the matching `…handles.tail…` message. A non-object `handles` value fails with `plans[].sequence.handles: head and tail must be integers >= 0.`.

<a id="plans-audio"></a>

### 4.4 `audio`

| Sub-field | Required | Type                      | Description                                                                                                            |
| --------- | -------- | ------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| `path`    | yes      | quoted string (non-empty) | Path to a guide audio track (WAV PCM, AAC in `.wav` / `.aac` / `.m4a`, or audio-only `.mov`). Metadata tokens allowed.  |

**Audio is optional.**  
When present, `path` must be a **quoted** non-empty YAML string. Unquoted or empty values fail with `plans[].audio.path: must be a quoted non-empty string.`.  
If present for any plan, _all_ generated MOVs for this job include a shared AAC-LC stereo 48 kHz guide track at **192 kbps**.  
Audio behavior per plan:
1. **Slate duration**: MOVs always begin with silence for `layout.slate.duration_frames`, regardless of audio.
2. **Guide track matching plate range**: For each plan, the guide audio is loaded, resampled to 48 kHz stereo, and trimmed/padded to match the *processed plate range* (`frame_end - frame_start + 1 + handles.head + handles.tail`), then appended to the shared output audio stream.
3. **Missing/omitted audio**: If `audio.path` is omitted on a plan, silence fills its entire range.

<a id="plans-timecode"></a>

### 4.5 `timecode`

The `timecode` block is optional. It specifies a SMPTE timecode clock for overlays (burn-ins, such as `{timecode}`) and for embedding QuickTime MOV timecode metadata. If you do not need timecode, omit the entire `timecode` section from the plan.

| Field        | Required (if `timecode` is present) | Type | Default | Description |
| ------------ | ----------------------------------- | ---- | ------- | ----------- |
| `start`      | yes | string `HH:MM:SS:FF` **or** integer ≥ 0 | — | The timecode at the hero `sequence.frame_start`. Strings are SMPTE format, e.g. `01:00:00:00`. Integers specify the frame count from `00:00:00:00` at the resolved frame rate. If `drop_frame` is `true`, use a semicolon before the frame number (`;FF`) as per SMPTE practice. |
| `drop_frame` | no  | boolean | `false` | Drop-frame counting. Omit for `false`. Must be a YAML boolean (`true` / `false`); other types fail with `plans[].timecode.drop_frame: must be a boolean (true or false).`. **`true` is valid only when the resolved fps is `30` or `60`**; otherwise DailyBoy rejects the job with `plans[].timecode.drop_frame: true requires resolved fps 30 or 60.`. |

**Frame Rate Resolution (How the timecode "fps" is chosen):**
- There is *no* explicit `rate` field in the job.
- The timecode's frame rate is taken as the common `fps` set on all **enabled** `output.videos[]` entries (the default is `24` if omitted).
- If `timecode` is set and any enabled video outputs disagree on `fps`, DailyBoy will flag a job error.
- If there are no enabled video outputs, and only image sequences are enabled, the timecode frame rate is `24`.

**Timecode anchoring and offsets:**
- The `start` value is the timecode assigned to the hero `frame_start`.
- Handles (if present) extend before and/or after the hero range, and timecodes before/after `start` are counted using normal SMPTE rules. For example, a head handle will yield negative or earlier timecode values prior to the `frame_start`.
- The slate burn-in (if used) shows the `start` timecode on every slate frame.
- For QuickTime MOV files, the embedded timecode is for the *first media frame* as encoded (which includes slate frames and head handles if present).

#### Example: plans section with all relevant fields

```yaml
plans:
  - id: "sh010_bg"
    input_colorspace: "ACEScg"
    timecode:
      start: "01:00:00:00"
      # drop_frame: false
      # You can also use: start: 86400   # (frame count at 24 fps)
    sequence:
      path: "/shots/sh010/renders/v003/bg.%04d.png"
      frame_start: 1001
      frame_end: 1048
      handles:
        head: 8
        tail: 8
    audio:
      path: "{dailies_root}/audio/sh010_guide.wav"
```

**Explanation:**

- Declares a plan called `sh010_bg`.
- Input plate images are expected for frames 993 to 1056, due to 8 head and 8 tail handles (`frame_start` 1001, `frame_end` 1048).
- All frames are interpreted as ACEScg color space for correct color management.
- If a guide audio track is provided, it is conformed to the entire plate range (handles included), and silence covers the slate duration.
- Timecode at hero frame 1001 is set to `01:00:00:00`. The head handle frame 993 gets the timecode `00:59:59:16` at 24 fps, progressing forward by one for every frame.
- The hero range—used for overlays and burn-ins—remains 1001–1048; the handles merely extend the processed image/audio beyond this range.

This ensures your timecode overlays and MOV files are always consistent and predictable, even when using handles or slates.

---

<a id="section-5-color"></a>

## 5. `color` (OpenColorIO)

The `color` section defines the OpenColorIO (OCIO) configuration, working space, and optional context.

| Field                | Required | Type                   | Description |
| -------------------- | -------- | ---------------------- | ----------- |
| `ocio_config`        | yes      | quoted string (non-empty) | Path to the `config.ocio` file. Can use environment variables (`${VAR}`). |
| `working_colorspace` | no       | quoted string (non-empty) | Linear working space for plate resize. Default at render: `plans[0].input_colorspace`. Supports `{token}` string substitutions from `metadata.substitutions` (not frame maps). |
| `context`            | no       | object (quoted string keys and values, min 1 pair) | OCIO context overrides (optional). Omit the key, or provide at least one pair. Context values support `{token}` string substitutions. |

At render time (empty after parse = omitted):

1. **working** = `color.working_colorspace` if set, else `plans[0].input_colorspace`
2. Each deliverable applies OCIO **DisplayView** from its `display_view.display` / `display_view.view` after resize
3. Layout UI colors (background, burn-ins, slate) are authored as display RGB (no `overlay_colorspace`)

Example:

```yaml
color:
  ocio_config: "ocio://studio-config-v4.0.0_aces-v2.0_ocio-v2.5"
  working_colorspace: "ACEScg"
  context:
    "SHOT": "sh010"
```

Explanation:

This example defines a `color` block for OpenColorIO color management:

- `ocio_config`: Points to the OCIO configuration.
- `working_colorspace`: Linear space used for plate resize (here, `"ACEScg"`).
- `context`: Overrides the OCIO context variable `SHOT` to `sh010` for this job.

This means: “Use the provided OCIO configuration, resize in ACEScg, then apply each deliverable’s DisplayView for review.”

---

<a id="section-6-layout"></a>

## 6. `layout` (page layout)

The `layout` section specifies the output page dimensions, image framing and placement, background color, burn-in text overlays, and slate (title card) configuration.

Required subfields: `canvas` and `image`. Optional: `pixel_aspect`, `background`, `burn_ins`, `slate`.

<a id="layout-canvas"></a>

### 6.1 `canvas`

Defines the output image size.

| Sub-field | Required | Type          | Description |
| --------- | -------- | ------------- | ----------- |
| `width`   | yes      | even integer (≥ 2) | Width of the output image, in pixels. Must be even so video codecs can encode the canvas as-is. |
| `height`  | yes      | even integer (≥ 2) | Height of the output image, in pixels. Must be even so video codecs can encode the canvas as-is. |

Common sizes: `1920x1080` (HD), `2048x1080` (2K DCI), `2560x1350` (QHD+), `2840x2160` (UHD), `4096x1716` (4K DCI), `4096x2160` (full 4K), `1024x768` (XGA), `1280x720` (HD 720p), `2560x1440` (1440p/QHD).

<a id="layout-pixel-aspect"></a>

### 6.2 `pixel_aspect`

| Field          | Required | Type                        | Description |
| -------------- | -------- | --------------------------- | ----------- |
| `pixel_aspect` | no       | number (> 0, default `1.0`) | Pixel aspect ratio. Must be strictly greater than 0 (`exclusiveMinimum: 0`). Default when omitted is square pixels (`1.0`). |

You can leave out the `pixel_aspect` field unless your output images use non-square pixels (for example, if you are matching legacy formats or anamorphic material). For most modern workflows, the default value `1.0` (square pixels) is correct and you do not need to include this field. Zero or negative values fail with `layout.pixel_aspect: must be a number > 0 (e.g. 1.0).`.

<a id="layout-image"></a>

### 6.3 `image` (source framing in canvas)

Describes how the source image is placed on the canvas.

| Sub-field       | Required | Type                                                 | Description |
| --------------- | -------- | ---------------------------------------------------- | ----------- |
| `fit`           | yes      | quoted string (`"contain"` or `"cover"`) | How the input image is framed on the canvas: <br> - `contain` fits the entire image within the canvas without cropping, adding bars if the aspect does not match. <br> - `cover` fills the canvas completely, cropping the image as needed to avoid bars. |
| `filter`        | no       | quoted string (`"bilinear"`, `"lanczos3"`) | Resize filter. `bilinear` is fast and softer; `lanczos3` is slower and sharper. Default `"lanczos3"`. |
| `min_margin_px` | no       | object                                               | Ensures there is enough space for burn-ins around the image by guaranteeing a minimum margin (in pixels) between the input image and the edges of the canvas, so overlays do not overlap the input image.

Margins can be specified as:

| Key      | Required | Type          | Default |
| -------- | -------- | ------------- | ------- |
| `top`    | no       | integer (≥ 0) | `0`     |
| `bottom` | no       | integer (≥ 0) | `0`     |
| `left`   | no       | integer (≥ 0) | `0`     |
| `right`  | no       | integer (≥ 0) | `0`     |

<a id="layout-background"></a>

### 6.4 `background` (optional)

Sets the color for the background behind the image. This is the color of the canvas edges (the pillarbox or letterbox area) that appear if the input image and canvas aspect ratios do not match and bars are needed.

| Sub-field     | Required | Type                                   | Default | Description                |
| ------------- | -------- | -------------------------------------- | ------- | -------------------------- |
| `r`, `g`, `b` | no       | number (0–1)                           | `0`     | Red, green, blue components. |

Components are display-referred RGB (0–1) after DisplayView. Any missing field defaults to 0. If `background` is present it must be a map whose keys are only `r`, `g`, and `b`.

<a id="layout-burn-ins"></a>

### 6.5 `burn_ins` (optional)

A list of burn-in overlays (text lines) to draw on every frame, except the slate. Omit the key if there are none. If the key is present, the list must contain at least one burn-in (`burn_ins: []` is a user error).

Each entry:

| Field      | Required | Type                    | Description                |
| ---------- | -------- | ----------------------- | -------------------------- |
| `template` | yes      | quoted string           | Text template, can include substitutions and tokens. |
| `position` | yes      | object                  | Position on the image.     |
| `font`     | yes      | object                  | Font file and size.        |
| `box`      | no       | object                  | Box styling (optional).    |

**Position:**

| Sub-field | Required | Type | Description |
| --------- | -------- | ---- | ----------- |
| `mode` | yes | quoted string (`"layout"`, `"pixel"`, or `"percent"`) | How `anchor` or `x` / `y` are interpreted. |
| `anchor` | when `mode` is `"layout"` | quoted string | Nine-point name. Invalid when `mode` is `"pixel"` or `"percent"`. |
| `x`, `y` | when `mode` is `"pixel"` or `"percent"` | integer | Invalid when `mode` is `"layout"`. Pixel: `x` in `[0, canvas.width]`, `y` in `[0, canvas.height]`. Percent: both in `[0, 100]`. |

**Position Modes:**

- `"layout"`: nine-point grid on the canvas (`anchor` = `{vertical}_{horizontal}`: `"top_left"`, `"top_center"`, `"top_right"`, `"center_left"`, `"center_center"`, `"center_right"`, `"bottom_left"`, `"bottom_center"`, `"bottom_right"`). Corners and edge midpoints sit 10 px in from the touched edge(s); `"center_center"` is the canvas center with no inset. The anchor applies to the full box (glyph plus `box.margin`).
- `"pixel"`: specify integer `x` / `y` in image pixels from top-left of the full box (`x` in `[0, canvas.width]`, `y` in `[0, canvas.height]`).
- `"percent"`: specify integer `x` / `y` as percent (`[0, 100]`) of canvas width/height, top-left of the full box.

**Font:**

| Sub-field | Required | Type               | Description             |
| --------- | -------- | ------------------ | ----------------------- |
| `path`    | yes      | quoted string (non-empty) | Path to an existing `.ttf` or `.otf` file. After substitutions resolve, the file must exist on disk. Existence is **not** checked while the path still contains `{` or `$` substitution tokens (those resolve later). Missing resolved files fail with `layout: font.path: file not found. Set a path to an existing .ttf or .otf file (after resolving substitutions).`. |
| `size_px` | yes      | integer (≥ 4)      | Font size in pixels.    |
| `color`   | no       | object (r, g, b in `[0, 1]`) | Text color in display RGB. Omitted block or channel defaults to `1` (white). If present, keys must be only `r`, `g`, and `b`. |

**Box:**

| Sub-field | Required | Type            | Default   | Description                  |
| --------- | -------- | --------------- | --------- | ---------------------------- |
| `mode`    | no       | string          | `fill`    | Fill or outline              |
| `color`   | no       | object (r,g,b)  | black     | Color (display RGB)|
| `opacity` | no       | number (0–1)    | `1`       | Alpha transparency           |
| `margin`  | no       | int or object   | `0`       | Padding around text          |

<a id="layout-slate"></a>

### 6.6 `slate` (optional)

Defines a title card prepended to each enabled movie and image sequence. **Omit the whole `slate` block to skip the slate** — do not use `lines: []`. When `slate` is present, `lines` must contain **at least one** entry (`minItems: 1`); an empty list fails with `layout.slate.lines: add at least one line, or omit the slate block.`. Sequence files use frame numbers immediately before the first processed plate (`plans[].sequence.frame_start - plans[].sequence.handles.head`, or `frame_start` when handles are omitted); plate files keep source numbers.

| Field             | Required | Type                          | Description                  |
| ----------------- | -------- | ----------------------------- | ---------------------------- |
| `duration_frames` | yes      | integer (≥ 0)                 | Number of identical head frames to write. `0` writes none. |
| `lines`           | yes      | array of objects (min 1 item) | Text lines for the slate. At least one line when the block is present. |

Each `lines[]` entry:

| Sub-field  | Required | Type   | Description                                                      |
| ---------- | -------- | ------ | ---------------------------------------------------------------- |
| `text`     | yes      | quoted string | Text for the line, substitutions and tokens allowed.      |
| `position` | yes      | object | Positioning; same as for burn-ins.                               |
| `font`     | yes      | object | Font declaration; same as for burn-ins (including `font.path` existence rules). |

<a id="layout-example"></a>

### Example — `layout`

```yaml
layout:
  canvas:
    width: 1920
    height: 1080
  pixel_aspect: 1.0
  image:
    fit: "contain"
    filter: "lanczos3"
    min_margin_px:
      top: 48
      right: 32
      bottom: 72
      left: 32
  background:
    r: 0
    g: 0
    b: 0
  burn_ins:
    - template: "{shot}  {frame}  {source_file}"
      position:
        mode: "layout"
        anchor: "bottom_left"
      font:
        path: "dailyboy/tests/data/fonts/DejaVuSans.ttf"
        size_px: 22
      box:
        mode: fill
        color:
          r: 0
          g: 0
          b: 0
        opacity: 0.45
        margin: 6
  slate:
    duration_frames: 24
    lines:
      - text: "PROJECT — {project}"
        position:
          mode: "layout"
          anchor: "top_left"
        font:
          path: "dailyboy/tests/data/fonts/DejaVuSans.ttf"
          size_px: 32
```

---

<a id="section-7-output"></a>

## 7. `output`

The `output` section specifies the exported files. You may output movie files (`videos`), image sequences (`image_sequences`), or both. At least one entry in either list must have `enabled: true`.

A movie is a single file (e.g. `.mov`). An image sequence writes one file per rendered frame (e.g. `.png` or `.exr`).

<a id="output-structure"></a>

### 7.1 `output` object structure

| Key               | Required | Type                     | Description                                    |
| ----------------- | -------- | ------------------------ | ---------------------------------------------- |
| `videos`          | no       | array of objects         | Movie output definitions.                      |
| `image_sequences` | no       | array of objects         | Image sequence output definitions (per frame). |

- `videos`: List of movie outputs. You can set `enabled: false` for entries you want to keep in the config but not process on a run.
- `image_sequences`: List of image sequence outputs (per-frame files). The file type comes from the extension in `path_pattern`.

<a id="output-videos"></a>

### 7.2 `videos[]` entries

Each entry specifies one movie output.

| Field           | Required | Values                        | Default if omitted | Description                    |
| --------------- | -------- | ----------------------------- | ------------------ | ------------------------------ |
| `id`            | yes      | string (non-empty)            | —                  | Unique identifier for this movie|
| `enabled`       | yes      | boolean                       | —                  | Whether to write this output    |
| `display_view`  | yes      | object (`display`, `view`)    | —                  | OCIO DisplayView for review     |
| `signal`        | yes      | object — see [§7.2.0](#output-signal) | —                  | Encode color tags + YCbCr matrix|
| `path`          | yes      | string (non-empty)            | —                  | Output file path (QuickTime `.mov`; muxer is always `mov`) |
| `fps`           | no       | integer ≥ 1                   | `24`               | Frames per second. When `plans[].timecode` is set, every enabled video must share the same `fps` (see §4.5). |
| `codec`         | yes      | `h264`, `mjpeg`, `dnxhd`, `prores` | —                  | Output codec                    |
| `codec_options` | no       | object                        | codec defaults     | Codec parameters (optional)     |

See the documentation for details regarding available `codec_options` per codec.

<a id="output-signal"></a>

#### 7.2.0 `signal`

Required on every `videos[]` entry. Tags written into the bitstream / container and the RGB→YCbCr matrix used by swscale. Only BT.709 is supported today.

| Field        | Required | Values     | Description |
| ------------ | -------- | ---------- | ----------- |
| `range`      | yes      | `tv`, `pc` | Limited (`tv`) or full (`pc`) luma/chroma range |
| `matrix`     | yes      | `bt709`    | YCbCr matrix (AVCOL_SPC) and swscale RGB→YUV matrix |
| `primaries`  | yes      | `bt709`    | Color primaries metadata |
| `transfer`   | yes      | `bt709`    | Transfer characteristic metadata |

`display_view` selects the OCIO DisplayView that produces the review RGB sent to the encoder; `signal` describes how that RGB is tagged and converted to YCbCr. They are independent fields.

<a id="output-codec-options"></a>

#### 7.2.1 `codec_options` for `h264`

Optional. If not specified, defaults are used. Only set either `crf` or `bitrate_kbps`, not both.

| Field          | Required | Values                                                                 | Default                  | Description                                 |
| -------------- | -------- | ---------------------------------------------------------------------- | ------------------------ | ------------------------------------------- |
| `preset`       | no       | `ultrafast`, `superfast`, `veryfast`, `faster`, `fast`, `medium`, `slow` | `medium`                | Encoding speed / efficiency tradeoff.       |
| `crf`          | no       | number 0–51                                                            | `23` (if bitrate omitted)| Constant Rate Factor (quality).             |
| `bitrate_kbps` | no       | integer ≥ 1                                                            | unused                   | Target bitrate (alternative to crf).        |
| `gop`          | no       | integer ≥ 1                                                            | encoder default (~250)   | Keyframe interval.                          |
| `pix_fmt`      | no       | `yuv420p`, `yuv422p`                                                   | `yuv420p`                | Pixel format.                               |
| `tune`         | no       | `film`, `animation`, `grain`, `stillimage`, `fastdecode`, `zerolatency` | not set                  | Optional encoder hint.                      |
| `profile`      | no       | `baseline`, `main`, `high`, `high422`                                  | auto based on pix_fmt    | H.264 feature set.                          |
| `level`        | no       | `3.0`, `3.1`, `4.0`, `4.1`, `4.2`, `5.0`, `5.1`, `5.2`               | auto                     | Decoder limits (resolution, bitrate).       |
| `faststart`    | no       | `true`, `false`                                                        | `true`                   | Store moov atom at file head.               |

<a id="output-codec-options-mjpeg"></a>

#### 7.2.2 `codec_options` for `mjpeg`

Optional. If not specified, defaults are used.

| Field         | Required | Values                          | Default    | Description                                  |
| ------------- | -------- | ------------------------------- | ---------- | -------------------------------------------- |
| `qscale`      | no       | integer 1–31                    | `3`        | JPEG quality (lower is better quality/larger)|
| `pix_fmt`     | no       | `yuv420p`, `yuv422p`, `yuv444p` | `yuv422p`  | Pixel format                                 |
| `huffman`     | no       | `default`, `optimal`            | `optimal`  | JPEG table type                              |
| `faststart`   | no       | `true`, `false`                 | `true`     | Store moov atom at file head                 |

<a id="output-codec-options-dnxhd"></a>

#### 7.2.3 `codec_options` for `dnxhd`

Optional. If not specified, defaults are used.

- For DNxHR, profile controls quality; for DNxHD, you must specify `bitrate_kbps` and match Avid’s profile requirements.

| Field           | Required                    | Values                                                              | Default      | Description                                |
| --------------- | -------------------------- | ------------------------------------------------------------------- | ------------ | ------------------------------------------ |
| `profile`       | no                         | `dnxhd`, `dnxhr_lb`, `dnxhr_sq`, `dnxhr_hq`, `dnxhr_hqx`, `dnxhr_444` | `dnxhr_hq`  | Codec profile                              |
| `pix_fmt`       | no                         | `yuv422p`, `yuv422p10`, `yuv444p10`                                | by profile   | Pixel format                               |
| `bitrate_kbps`  | yes if `profile: dnxhd`    | integer ≥ 1                                                         | N/A          | Bitrate (DNxHD only)                       |
| `interlaced`    | no                         | `true`, `false`                                                     | `false`      | Interlacing (DNxHD only)                   |
| `nitris_compat` | no                         | `true`, `false`                                                     | `false`      | Nitris compatibility (padding)             |
| `faststart`     | no                         | `true`, `false`                                                     | `true`       | Store moov atom at file head               |

<a id="output-codec-options-prores"></a>

#### 7.2.4 `codec_options` for `prores`

Optional. If not specified, defaults are used. Encoding uses FFmpeg `prores_ks`. Source images have no alpha plane; `yuva444p10` / `alpha_bits` are not accepted.

- Profiles `proxy` / `lt` / `standard` / `hq` require `pix_fmt: yuv422p10`.
- Profiles `4444` / `4444xq` use `yuv444p10`.

| Field           | Required | Values                                                          | Default      | Description                                      |
| --------------- | -------- | --------------------------------------------------------------- | ------------ | ------------------------------------------------ |
| `profile`       | no       | `proxy`, `lt`, `standard`, `hq`, `4444`, `4444xq`               | `hq`         | ProRes profile (YAML `4444` may be unquoted) |
| `pix_fmt`       | no       | `yuv422p10`, `yuv444p10`                                        | by profile   | Pixel format (10-bit); float ImageBuf → RGB48 → YUV |
| `quant_mat`     | no       | `auto`, `proxy`, `lt`, `standard`, `hq`, `default`              | `auto`       | Quantization matrix                              |
| `bits_per_mb`   | no       | integer 0–8192                                                  | `0`          | Bits per macroblock (`0` = encoder default)      |
| `mbs_per_slice` | no       | integer 1–8                                                     | `8`          | Macroblocks per slice                            |
| `vendor`        | no       | exactly 4 ASCII characters                                      | `apl0`       | ProRes vendor ID in the bitstream                |
| `faststart`     | no       | `true`, `false`                                                 | `true`       | Store moov atom at file head                     |

<a id="output-image-sequences"></a>

### 7.3 `image_sequences[]` entries

Defines per-frame image outputs. Required fields:

| Field             | Required | Type                                                                      | Description                                    |
| ----------------- | -------- | ------------------------------------------------------------------------- | ---------------------------------------------- |
| `id`              | yes      | string (non-empty)                                                        | Unique identifier for the output.              |
| `enabled`         | yes      | boolean                                                                   | Whether to write this output                   |
| `display_view`    | yes      | object (`display`, `view`)                                                | OCIO DisplayView for this sequence             |
| `path_pattern`    | yes      | string ([fileseq](https://github.com/shotgunsoftware/fileseq) syntax)     | Output file pattern; extension picks file type |
| `format_options`  | no       | object                                                                    | Writer settings; keys depend on the extension  |

The file extension of `path_pattern` selects the OpenImageIO writer and which `format_options` keys are valid. Supported extensions: `.png`, `.jpg` / `.jpeg`, `.tif` / `.tiff`, `.exr`, `.heif` / `.heic` / `.avif`.

<a id="output-format-options-png"></a>

#### 7.3.1 `format_options` for `.png`

| Field | Required | Values | Default | Description |
| ----- | -------- | ------ | ------- | ----------- |
| `bit_depth` | no | `8`, `16` | `8` | Pixel bit depth |
| `compression` | no | `default`, `filtered`, `huffman`, `rle`, `fixed`, `pngfast`, `none` | `default` | zlib strategy |
| `compression_level` | no | 0–6 | `6` | zlib level (`png:compressionLevel`) |
| `filter` | no | 0–4 | `0` | PNG filter (0=none … 4=paeth) |

<a id="output-format-options-jpeg"></a>

#### 7.3.2 `format_options` for `.jpg` / `.jpeg`

| Field | Required | Values | Default | Description |
| ----- | -------- | ------ | ------- | ----------- |
| `bit_depth` | no | `8` | `8` | Pixel bit depth |
| `compression_level` | no | 1–100 | `98` | JPEG quality (`CompressionQuality`) |
| `subsampling` | no | `4:4:4`, `4:2:2`, `4:2:0`, `4:1:1` | libjpeg default | Chroma subsampling |
| `progressive` | no | `true`, `false` | `false` | Progressive JPEG |

<a id="output-format-options-tiff"></a>

#### 7.3.3 `format_options` for `.tif` / `.tiff`

| Field | Required | Values | Default | Description |
| ----- | -------- | ------ | ------- | ----------- |
| `bit_depth` | no | `1`,`2`,`4`,`6`,`8`,`10`,`12`,`14`,`16`,`24`,`32`,`64`,`h16`,`f32` | `8` | Sample depth (`h16` = half) |
| `bigtiff` | no | `true`, `false` | `false` | Force BigTIFF |
| `compression` | no | `none`, `lzw`, `zip`, `ccittrle`, `packbits` | `zip` | TIFF compression |
| `compression_level` | no | 1–9 | `6` | zip quality |

<a id="output-format-options-exr"></a>

#### 7.3.4 `format_options` for `.exr`

| Field | Required | Values | Default | Description |
| ----- | -------- | ------ | ------- | ----------- |
| `bit_depth` | no | `h16`, `f32` | `h16` | HALF or FLOAT |
| `compression` | no | `none`, `rle`, `zips`, `zip`, `piz`, `pxr24`, `b44`, `b44a`, `dwaa`, `dwab` | `zip` | OpenEXR compression |
| `compression_level` | no | zip/zips: 1–9; dwaa/dwab: ≥0 | `4` (zip/zips) | Forbidden for other compressions |

<a id="output-format-options-heif"></a>

#### 7.3.5 `format_options` for `.heif` / `.heic` / `.avif`

| Field | Required | Values | Default | Description |
| ----- | -------- | ------ | ------- | ----------- |
| `bit_depth` | no | `8` | `8` | OIIO writes uint8 only |
| `compression` | no | `heic`, `avif` (`hevc` alias → `heic`) | `heic` (or `avif` if `.avif`) | Codec |
| `compression_level` | no | 0–100 | `75` | Quality |

Examples:

<a id="output-example"></a>

### Example — `output`

```yaml
output:
  videos:
    - id: review_h264
      enabled: true
      display_view:
        display: "Rec.1886 Rec.709 - Display"
        view: "ACES 2.0 - SDR 100 nits (Rec.709)"
      signal:
        range: tv
        matrix: bt709
        primaries: bt709
        transfer: bt709
      path: /out/sh010_review.mov
      fps: 24
      codec: h264
      codec_options:
        preset: medium
        crf: 20
        gop: 12
        pix_fmt: yuv420p
        tune: film
        profile: high
        level: 4.1
        faststart: true
  image_sequences:
    - id: archive_acescg
      enabled: true
      display_view:
        display: "passthrough"
        view: "passthrough"
      path_pattern: /out/sh010_archive.%04d.png
      format_options:
        bit_depth: 8
        compression: default
        compression_level: 6
        filter: 0
```

---

<a id="section-8-write-your-own"></a>

## 8. Write your own job

To create a new job file, copy the following skeleton to a new `.yaml` file. Replace all placeholder values such as `CHANGE_ME` and update paths to your needs. Maintain valid indentation.

Checklist before running:

1. Specify the location and frame numbers of source images.
2. Set the OCIO colorspace name and the `config.ocio` path.
3. Set an appropriate page size (`1920x1080` is commonly used).
4. Choose your framing: `contain` (show all image) or `cover` (fill/crop page).
5. Decide what output files are needed (movie, image sequence, or both), and specify their destinations.

```yaml
dailyboy_version: 1

# Optional. Remove if you do not need substitutions.
metadata:
  substitutions:
    shot: "sh010"
    dailies_root: "/path/to/output/folder"

plans:
  - id: "my_shot"
    input_colorspace: "ACEScg"   # CHANGE_ME: OCIO input colorspace
    timecode:
      start: "01:00:00:00"       # optional; omit block if unused
    sequence:
      path: "/path/to/frames/plate.%04d.png"   # CHANGE_ME
      frame_start: 1001                         # CHANGE_ME
      frame_end: 1048                           # CHANGE_ME
    # audio:                               # optional guide track for MOVs
    #   path: "{dailies_root}/audio/guide.wav"

color:
  ocio_config: "${OCIO_CONFIGS}/aces_1.2/config.ocio"   # CHANGE_ME
  working_colorspace: "ACEScg"
  
layout:
  canvas:
    width: 1920    # CHANGE_ME if different
    height: 1080
  image:
    fit: "contain"   # or "cover"
  background:
    r: 0
    g: 0
    b: 0
  # burn_ins: omit this key if none; if present, at least one item
  # slate: omit this block to skip the slate; if present, lines needs ≥ 1 item

output:
  videos:
    - id: review_h264
      enabled: true
      display_view:
        display: "Rec.1886 Rec.709 - Display"
        view: "ACES 2.0 - SDR 100 nits (Rec.709)"
      signal:
        range: tv
        matrix: bt709
        primaries: bt709
        transfer: bt709
      path: "{dailies_root}/review.mov"   # CHANGE_ME
      fps: 24
      codec: h264
      # codec_options:   # optional; omit for defaults
```

- To add overlays (burn-ins), populate `burn_ins` with template(s) from the example above.
- To add a slate page, add `layout.slate` with `duration_frames` and at least one `lines[]` entry (see §6.6). To skip the slate, omit the whole `slate` block — never use `lines: []`.
- To output image sequences (in addition to or instead of a movie), add `image_sequences` similarly to the example in §7.3, ensuring that at least one output is `enabled: true`.

A more complete example is available per year: [cy2026](../../examples/job.mvp.example.cy2026.yaml), [cy2025](../../examples/job.mvp.example.cy2025.yaml), [cy2024](../../examples/job.mvp.example.cy2024.yaml).

---

<a id="section-9-see-also"></a>

## 9. See also

- [README](../../README.md#usage): how to run and embed `makeDaily` (C++ / Python).
- [README](../../README.md#build-install-and-test-with-cmake): build, tests, and color pipeline overview.
