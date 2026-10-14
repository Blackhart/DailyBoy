#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup job_errors Job YAML contract errors
 *  @{
 *
 *  \note Message style (USER_ERROR_JOB_*):
 *  - Format: `<field_path>: <constraint>[. <fix_hint>].`
 *  - `field_path` uses contract paths with `[]` (e.g. `output.videos[]`).
 *  - String enums quote YAML values (`"fill"`, `"h264"`).
 *  - Templates: quoted non-empty / quoted enum / `unsupported value. Use …` /
 *    unquoted integer ranges / `missing required key. Set …`.
 *  - Keep `job:` only for generic fallbacks (JOB_10/11/18/25/40–44).
 *  - IDs are stable; do not renumber.
 */

/*!
 * \var USER_ERROR_JOB_1
 * \brief Message when \c layout.canvas width or height is odd.
 */
inline constexpr std::string_view USER_ERROR_JOB_1 =
    "layout.canvas: width and height must be even (e.g. 1920x1080).";

/*!
 * \var USER_ERROR_JOB_2
 * \brief Message when a \c metadata.substitutions key is not a YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_2 =
    "metadata.substitutions: names must be quoted strings (e.g. shot: "
    "\"sh010\"), not numbers or booleans.";

/*!
 * \var USER_ERROR_JOB_3
 * \brief Message when a substitution value is not a quoted string or frame
 *        map.
 */
inline constexpr std::string_view USER_ERROR_JOB_3 =
    "metadata.substitutions: values must be a quoted string (e.g. \"103\") "
    "or a frame map.";

/*!
 * \var USER_ERROR_JOB_4
 * \brief Message when \c metadata.substitutions is not a map.
 */
inline constexpr std::string_view USER_ERROR_JOB_4 =
    "metadata.substitutions: must be a map of name: value (e.g. shot: "
    "\"sh010\").";

/*!
 * \var USER_ERROR_JOB_5
 * \brief Message when \c metadata is present without \c substitutions.
 */
inline constexpr std::string_view USER_ERROR_JOB_5 =
    "metadata: missing required key. Set substitutions with at least one "
    "name (e.g. shot: \"sh010\"), or omit the metadata block.";

/*!
 * \var USER_ERROR_JOB_6
 * \brief Message when \c metadata.substitutions is an empty map.
 */
inline constexpr std::string_view USER_ERROR_JOB_6 =
    "metadata.substitutions: add at least one name (e.g. shot: \"sh010\"), "
    "or omit the metadata block.";

/*!
 * \var USER_ERROR_JOB_7
 * \brief Message when a frame-map substitution is used outside burn-in/slate.
 */
inline constexpr std::string_view USER_ERROR_JOB_7 =
    "metadata.substitutions: frame maps are only allowed in "
    "layout.burn_ins[].template and layout.slate.lines[].text.";

/*!
 * \var USER_ERROR_JOB_8
 * \brief Message when a file sequence pattern is empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_8 =
    "file sequence path: empty pattern. Set a path with padding (e.g. "
    "\"plate.%04d.png\").";

/*!
 * \var USER_ERROR_JOB_9
 * \brief Message when libfileseq rejects a sequence pattern.
 */
inline constexpr std::string_view USER_ERROR_JOB_9 =
    "file sequence path: invalid pattern. Use ShotGrid/libfileseq padding "
    "(#, @, %04d, $F4).";

/*!
 * \var USER_ERROR_JOB_10
 * \brief Message when a field is not the YAML type required by the contract.
 */
inline constexpr std::string_view USER_ERROR_JOB_10 =
    "job: wrong YAML type for this field. Use a map or list as required by "
    "the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_11
 * \brief Message when a nested required key is missing.
 */
inline constexpr std::string_view USER_ERROR_JOB_11 =
    "job: a required key is missing. Add the key named after this message.";

/*!
 * \var USER_ERROR_JOB_12
 * \brief Message when the job omits \c plans.
 */
inline constexpr std::string_view USER_ERROR_JOB_12 =
    "plans: missing required key. Add a plans list with exactly one plan.";

/*!
 * \var USER_ERROR_JOB_13
 * \brief Message when the job omits \c color.
 */
inline constexpr std::string_view USER_ERROR_JOB_13 =
    "color: missing required key. Add a color block with ocio_config.";

/*!
 * \var USER_ERROR_JOB_14
 * \brief Message when the job omits \c layout.
 */
inline constexpr std::string_view USER_ERROR_JOB_14 =
    "layout: missing required key. Add a layout block with canvas width and "
    "height.";

/*!
 * \var USER_ERROR_JOB_15
 * \brief Message when the job omits \c output.
 */
inline constexpr std::string_view USER_ERROR_JOB_15 =
    "output: missing required key. Add an output block with at least one "
    "enabled video or image sequence.";

/*!
 * \var USER_ERROR_JOB_16
 * \brief Message when the job omits \c dailyboy_version.
 */
inline constexpr std::string_view USER_ERROR_JOB_16 =
    "dailyboy_version: missing required key. Set dailyboy_version: 1.";

/*!
 * \var USER_ERROR_JOB_17
 * \brief Message when \c color omits \c ocio_config.
 */
inline constexpr std::string_view USER_ERROR_JOB_17 =
    "color.ocio_config: missing required key. Set a quoted config.ocio "
    "path.";

/*!
 * \var USER_ERROR_JOB_18
 * \brief Message when a YAML scalar cannot be converted to the expected type.
 */
inline constexpr std::string_view USER_ERROR_JOB_18 =
    "job: invalid value for this field. Check the type in the job YAML "
    "reference.";

/*!
 * \var USER_ERROR_JOB_19
 * \brief Message when a layout anchor is not one of the nine-point names.
 */
inline constexpr std::string_view USER_ERROR_JOB_19 =
    "layout.position.anchor: unsupported value. Use \"top_left\", "
    "\"top_center\", \"top_right\", \"center_left\", \"center_center\", "
    "\"center_right\", \"bottom_left\", \"bottom_center\", or "
    "\"bottom_right\".";

/*!
 * \var USER_ERROR_JOB_20
 * \brief Message when a text position mode is not layout, pixel, or percent.
 */
inline constexpr std::string_view USER_ERROR_JOB_20 =
    "layout.position.mode: unsupported value. Use \"layout\", \"pixel\", or "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_21
 * \brief Message when a burn-in box mode is not fill or outline.
 */
inline constexpr std::string_view USER_ERROR_JOB_21 =
    "layout.burn_ins[].box.mode: unsupported value. Use \"fill\" or "
    "\"outline\".";

/*!
 * \var USER_ERROR_JOB_22
 * \brief Message when \c layout.image.fit is not contain or cover.
 */
inline constexpr std::string_view USER_ERROR_JOB_22 =
    "layout.image.fit: unsupported value. Use \"contain\" or \"cover\".";

/*!
 * \var USER_ERROR_JOB_23
 * \brief Message when \c layout.image.filter is not bilinear or lanczos3.
 */
inline constexpr std::string_view USER_ERROR_JOB_23 =
    "layout.image.filter: unsupported value. Use \"bilinear\" or "
    "\"lanczos3\".";

/*!
 * \var USER_ERROR_JOB_24
 * \brief Message when H.264 sets both \c crf and \c bitrate_kbps.
 */
inline constexpr std::string_view USER_ERROR_JOB_24 =
    "output.videos[].codec_options: crf and bitrate_kbps are mutually "
    "exclusive. Keep only one.";

/*!
 * \var USER_ERROR_JOB_25
 * \brief Message when an enum field has a value not in the job contract.
 */
inline constexpr std::string_view USER_ERROR_JOB_25 =
    "job: unsupported value. Use a value allowed for this field in the job "
    "YAML reference.";

/*!
 * \var USER_ERROR_JOB_26
 * \brief Message when H.264 \c high422 is used without \c yuv422p.
 */
inline constexpr std::string_view USER_ERROR_JOB_26 =
    "output.videos[].codec_options: profile \"high422\" requires pix_fmt "
    "\"yuv422p\".";

/*!
 * \var USER_ERROR_JOB_27
 * \brief Message when H.264 \c yuv422p is used without \c high422.
 */
inline constexpr std::string_view USER_ERROR_JOB_27 =
    "output.videos[].codec_options: pix_fmt \"yuv422p\" requires profile "
    "\"high422\".";

/*!
 * \var USER_ERROR_JOB_28
 * \brief Message when H.264 \c level is not a YAML scalar.
 */
inline constexpr std::string_view USER_ERROR_JOB_28 =
    "output.videos[].codec_options.level: must be a scalar (e.g. 4.1 or "
    "\"4.1\").";

/*!
 * \var USER_ERROR_JOB_29
 * \brief Message when MJPEG \c qscale is outside 1..31.
 */
inline constexpr std::string_view USER_ERROR_JOB_29 =
    "output.videos[].codec_options.qscale: must be an unquoted integer in "
    "[1, 31] (e.g. 3).";

/*!
 * \var USER_ERROR_JOB_30
 * \brief Message when DNxHR sets \c bitrate_kbps.
 */
inline constexpr std::string_view USER_ERROR_JOB_30 =
    "output.videos[].codec_options.bitrate_kbps: only valid when profile is "
    "\"dnxhd\". Remove it for DNxHR.";

/*!
 * \var USER_ERROR_JOB_31
 * \brief Message when DNxHD omits \c bitrate_kbps.
 */
inline constexpr std::string_view USER_ERROR_JOB_31 =
    "output.videos[].codec_options.bitrate_kbps: missing required key when "
    "profile is \"dnxhd\".";

/*!
 * \var USER_ERROR_JOB_32
 * \brief Message when DNxHD \c bitrate_kbps is less than 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_32 =
    "output.videos[].codec_options.bitrate_kbps: must be an unquoted "
    "integer >= 1 (e.g. 8000).";

/*!
 * \var USER_ERROR_JOB_33
 * \brief Message when DNxHD/HR \c pix_fmt does not match the profile.
 */
inline constexpr std::string_view USER_ERROR_JOB_33 =
    "output.videos[].codec_options.pix_fmt: incompatible with this DNxHD/HR "
    "profile. Check the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_34
 * \brief Message when DNxHR sets \c interlaced: true.
 */
inline constexpr std::string_view USER_ERROR_JOB_34 =
    "output.videos[].codec_options.interlaced: only valid when profile is "
    "\"dnxhd\". Set interlaced: false or omit it.";

/*!
 * \var USER_ERROR_JOB_35
 * \brief Message when two deliverables share the same \c id.
 */
inline constexpr std::string_view USER_ERROR_JOB_35 =
    "output: duplicate id. Each video and image sequence id must be unique.";

/*!
 * \var USER_ERROR_JOB_36
 * \brief Message when every video and image sequence is disabled.
 */
inline constexpr std::string_view USER_ERROR_JOB_36 =
    "output: at least one video or image sequence must have enabled: true.";

/*!
 * \var USER_ERROR_JOB_37
 * \brief Message when \c plans is empty or has more than one item.
 */
inline constexpr std::string_view USER_ERROR_JOB_37 =
    "plans: must contain exactly one plan.";

/*!
 * \var USER_ERROR_JOB_38
 * \brief Message when \c dailyboy_version is not 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_38 =
    "dailyboy_version: must be 1. Set dailyboy_version: 1.";

/*!
 * \var USER_ERROR_JOB_39
 * \brief Message when \c output has neither \c videos nor \c image_sequences.
 */
inline constexpr std::string_view USER_ERROR_JOB_39 =
    "output: add videos and/or image_sequences, with at least one enabled: "
    "true.";

/*!
 * \var USER_ERROR_JOB_40
 * \brief Message when a key is forbidden by \c additionalProperties: false.
 */
inline constexpr std::string_view USER_ERROR_JOB_40 =
    "job: this key is not allowed. Remove it or check the job YAML "
    "reference.";

/*!
 * \var USER_ERROR_JOB_41
 * \brief Message when a field has the wrong JSON/YAML type in schema checks.
 */
inline constexpr std::string_view USER_ERROR_JOB_41 =
    "job: wrong type for this field. Check the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_42
 * \brief Message when a value fails a schema oneOf/anyOf contract.
 */
inline constexpr std::string_view USER_ERROR_JOB_42 =
    "job: value does not match the job contract (wrong type or keys). Check "
    "the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_43
 * \brief Message when a string field is empty (\c minLength: 1).
 */
inline constexpr std::string_view USER_ERROR_JOB_43 =
    "job: this field must not be empty.";

/*!
 * \var USER_ERROR_JOB_44
 * \brief Fallback when schema validation fails without a more specific case.
 */
inline constexpr std::string_view USER_ERROR_JOB_44 =
    "job: the YAML does not match the job contract. Check the field named "
    "after this message in the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_45
 * \brief Message when a frame-map key is not a YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_45 =
    "metadata.substitutions: frame map keys must be quoted strings (e.g. "
    "\"1001\" or \"1005-1010\"), not numbers or booleans.";

/*!
 * \var USER_ERROR_JOB_46
 * \brief Message when a frame-map value is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_46 =
    "metadata.substitutions: frame map values must be quoted strings (e.g. "
    "\"Start\").";

/*!
 * \var USER_ERROR_JOB_47
 * \brief Message when a frame-map key is neither a frame nor a frame range.
 */
inline constexpr std::string_view USER_ERROR_JOB_47 =
    "metadata.substitutions: frame map keys must be a frame (e.g. \"1001\") "
    "or a frame range (e.g. \"1005-1010\").";

/*!
 * \var USER_ERROR_JOB_48
 * \brief Message when \c color.ocio_config is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_48 =
    "color.ocio_config: must be a quoted non-empty string (e.g. "
    "\"/path/config.ocio\").";

/*!
 * \var USER_ERROR_JOB_49
 * \brief Message when \c color.working_colorspace is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_49 =
    "color.working_colorspace: must be a quoted non-empty string (e.g. "
    "\"ACES - ACEScg\").";

/*!
 * \var USER_ERROR_JOB_51
 * \brief Message when \c color.context is present but empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_51 =
    "color.context: add at least one key (e.g. \"SHOT\": \"sh010\"), or "
    "omit context.";

/*!
 * \var USER_ERROR_JOB_52
 * \brief Message when a \c color.context key is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_52 =
    "color.context: names must be quoted strings (e.g. \"SHOT\"), not "
    "numbers or booleans.";

/*!
 * \var USER_ERROR_JOB_53
 * \brief Message when a \c color.context value is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_53 =
    "color.context: values must be quoted strings (e.g. \"sh010\").";

/*!
 * \var USER_ERROR_JOB_54
 * \brief Message when \c layout omits \c canvas.
 */
inline constexpr std::string_view USER_ERROR_JOB_54 =
    "layout.canvas: missing required key. Add canvas with even width and "
    "height.";

/*!
 * \var USER_ERROR_JOB_55
 * \brief Message when \c layout.canvas omits \c width.
 */
inline constexpr std::string_view USER_ERROR_JOB_55 =
    "layout.canvas.width: missing required key. Set an even unquoted "
    "integer (e.g. 1920).";

/*!
 * \var USER_ERROR_JOB_56
 * \brief Message when \c layout.canvas.width is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_56 =
    "layout.canvas.width: must be an unquoted integer (e.g. 1920).";

/*!
 * \var USER_ERROR_JOB_57
 * \brief Message when \c layout.canvas omits \c height.
 */
inline constexpr std::string_view USER_ERROR_JOB_57 =
    "layout.canvas.height: missing required key. Set an even unquoted "
    "integer (e.g. 1080).";

/*!
 * \var USER_ERROR_JOB_58
 * \brief Message when \c layout.canvas.height is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_58 =
    "layout.canvas.height: must be an unquoted integer (e.g. 1080).";

/*!
 * \var USER_ERROR_JOB_59
 * \brief Message when \c layout.pixel_aspect is not a number.
 */
inline constexpr std::string_view USER_ERROR_JOB_59 =
    "layout.pixel_aspect: must be a number (e.g. 1.0).";

/*!
 * \var USER_ERROR_JOB_60
 * \brief Message when \c layout.canvas.width is less than 2.
 */
inline constexpr std::string_view USER_ERROR_JOB_60 =
    "layout.canvas.width: must be an unquoted integer >= 2 (e.g. 1920).";

/*!
 * \var USER_ERROR_JOB_61
 * \brief Message when \c layout.canvas.height is less than 2.
 */
inline constexpr std::string_view USER_ERROR_JOB_61 =
    "layout.canvas.height: must be an unquoted integer >= 2 (e.g. 1080).";

/*!
 * \var USER_ERROR_JOB_62
 * \brief Message when \c layout omits \c image.
 */
inline constexpr std::string_view USER_ERROR_JOB_62 =
    "layout.image: missing required key. Add image with fit \"contain\" or "
    "\"cover\".";

/*!
 * \var USER_ERROR_JOB_64
 * \brief Message when \c layout.image omits \c fit.
 */
inline constexpr std::string_view USER_ERROR_JOB_64 =
    "layout.image.fit: missing required key. Set \"contain\" or \"cover\".";

/*!
 * \var USER_ERROR_JOB_65
 * \brief Message when \c layout.image.fit is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_65 =
    "layout.image.fit: must be a quoted string (\"contain\" or \"cover\").";

/*!
 * \var USER_ERROR_JOB_66
 * \brief Message when \c layout.image.filter is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_66 =
    "layout.image.filter: must be a quoted string (\"bilinear\" or "
    "\"lanczos3\").";

/*!
 * \var USER_ERROR_JOB_67
 * \brief Message when \c layout.image.min_margin_px is not a map.
 */
inline constexpr std::string_view USER_ERROR_JOB_67 =
    "layout.image.min_margin_px: must be a map of top, right, bottom, and "
    "left.";

/*!
 * \var USER_ERROR_JOB_68
 * \brief Message when a \c min_margin_px side is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_68 =
    "layout.image.min_margin_px: each side must be an unquoted integer (e.g. "
    "top: 8).";

/*!
 * \var USER_ERROR_JOB_69
 * \brief Message when a \c min_margin_px side is less than 0.
 */
inline constexpr std::string_view USER_ERROR_JOB_69 =
    "layout.image.min_margin_px: each side must be an unquoted integer >= 0 "
    "(e.g. top: 8).";

/*!
 * \var USER_ERROR_JOB_70
 * \brief Message when \c layout.background is not a map.
 */
inline constexpr std::string_view USER_ERROR_JOB_70 =
    "layout.background: must be a map with keys r, g, and b only.";

/*!
 * \var USER_ERROR_JOB_71
 * \brief Message when a \c layout.background channel is not a number in [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_71 =
    "layout.background: r, g, and b must be numbers in [0, 1] (e.g. r: "
    "0.12).";

/*!
 * \var USER_ERROR_JOB_72
 * \brief Message when \c layout.burn_ins is present but empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_72 =
    "layout.burn_ins: add at least one burn-in, or omit burn_ins.";

/*!
 * \var USER_ERROR_JOB_73
 * \brief Message when a burn-in omits \c template.
 */
inline constexpr std::string_view USER_ERROR_JOB_73 =
    "layout.burn_ins[].template: missing required key. Set a quoted text "
    "template.";

/*!
 * \var USER_ERROR_JOB_74
 * \brief Message when a burn-in omits \c position.
 */
inline constexpr std::string_view USER_ERROR_JOB_74 =
    "layout.burn_ins[].position: missing required key. Add a position "
    "block.";

/*!
 * \var USER_ERROR_JOB_75
 * \brief Message when a burn-in omits \c font.
 */
inline constexpr std::string_view USER_ERROR_JOB_75 =
    "layout.burn_ins[].font: missing required key. Add a font block with "
    "path and size_px.";

/*!
 * \var USER_ERROR_JOB_76
 * \brief Message when a burn-in \c template is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_76 =
    "layout.burn_ins[].template: must be a quoted string (e.g. "
    "\"{frame}\").";

/*!
 * \var USER_ERROR_JOB_77
 * \brief Message when \c position.mode is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_77 =
    "layout.position.mode: must be a quoted string (\"layout\", \"pixel\", "
    "or \"percent\").";

/*!
 * \var USER_ERROR_JOB_78
 * \brief Message when \c position.anchor is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_78 =
    "layout.position.anchor: must be a quoted string (e.g. \"top_left\").";

/*!
 * \var USER_ERROR_JOB_79
 * \brief Message when \c position.anchor is set and \c mode is not layout.
 */
inline constexpr std::string_view USER_ERROR_JOB_79 =
    "layout.position.anchor: only valid when mode is \"layout\".";

/*!
 * \var USER_ERROR_JOB_80
 * \brief Message when \c position.x or \c y is set and \c mode is layout.
 */
inline constexpr std::string_view USER_ERROR_JOB_80 =
    "layout.position: x and y are only valid when mode is \"pixel\" or "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_81
 * \brief Message when \c position.x is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_81 =
    "layout.position.x: must be an unquoted integer.";

/*!
 * \var USER_ERROR_JOB_82
 * \brief Message when \c position.y is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_82 =
    "layout.position.y: must be an unquoted integer.";

/*!
 * \var USER_ERROR_JOB_83
 * \brief Message when pixel \c position.x is outside \c [0, canvas.width].
 */
inline constexpr std::string_view USER_ERROR_JOB_83 =
    "layout.position.x: must be an unquoted integer in [0, "
    "layout.canvas.width] when mode is \"pixel\".";

/*!
 * \var USER_ERROR_JOB_84
 * \brief Message when pixel \c position.y is outside \c [0, canvas.height].
 */
inline constexpr std::string_view USER_ERROR_JOB_84 =
    "layout.position.y: must be an unquoted integer in [0, "
    "layout.canvas.height] when mode is \"pixel\".";

/*!
 * \var USER_ERROR_JOB_85
 * \brief Message when percent \c position.x is outside \c [0, 100].
 */
inline constexpr std::string_view USER_ERROR_JOB_85 =
    "layout.position.x: must be an unquoted integer in [0, 100] when mode is "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_86
 * \brief Message when percent \c position.y is outside \c [0, 100].
 */
inline constexpr std::string_view USER_ERROR_JOB_86 =
    "layout.position.y: must be an unquoted integer in [0, 100] when mode is "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_87
 * \brief Message when a burn-in \c font.path is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_87 =
    "layout.font.path: must be a quoted non-empty string (e.g. "
    "\"dailyboy/tests/data/fonts/DejaVuSans.ttf\").";

/*!
 * \var USER_ERROR_JOB_88
 * \brief Message when a burn-in \c font.size_px is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_88 =
    "layout.font.size_px: must be an unquoted integer (e.g. 18).";

/*!
 * \var USER_ERROR_JOB_89
 * \brief Message when a burn-in \c font.size_px is below 4.
 */
inline constexpr std::string_view USER_ERROR_JOB_89 =
    "layout.font.size_px: must be an unquoted integer >= 4 (e.g. 18).";

/*!
 * \var USER_ERROR_JOB_90
 * \brief Message when a burn-in \c font.color is not a map of r, g, b.
 */
inline constexpr std::string_view USER_ERROR_JOB_90 =
    "layout.burn_ins[].font.color: must be a map with keys r, g, and b "
    "only.";

/*!
 * \var USER_ERROR_JOB_91
 * \brief Message when a burn-in \c font.color channel is outside [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_91 =
    "layout.burn_ins[].font.color: r, g, and b must be numbers in [0, 1] "
    "(e.g. r: 0.12).";

/*!
 * \var USER_ERROR_JOB_92
 * \brief Message when \c signal is missing or has an invalid field.
 */
inline constexpr std::string_view USER_ERROR_JOB_92 =
    "output.videos[].signal: missing required key. Set range, matrix, "
    "primaries, and transfer.";

/*!
 * \var USER_ERROR_JOB_93
 * \brief Message when \c format_options is set but the path extension is unknown.
 */
inline constexpr std::string_view USER_ERROR_JOB_93 =
    "output.image_sequences[].format_options: path_pattern extension must "
    "be png, jpg/jpeg, tif/tiff, exr, or heif/heic/avif.";

/*!
 * \var USER_ERROR_JOB_94
 * \brief Message when \c compression_level is invalid for the chosen EXR
 *        compression.
 */
inline constexpr std::string_view USER_ERROR_JOB_94 =
    "output.image_sequences[].format_options.compression_level: only valid "
    "for compression \"zip\", \"zips\", \"dwaa\", or \"dwab\".";

/*!
 * \var USER_ERROR_JOB_95
 * \brief Message when ProRes \c pix_fmt does not match the profile.
 */
inline constexpr std::string_view USER_ERROR_JOB_95 =
    "output.videos[].codec_options.pix_fmt: incompatible with this ProRes "
    "profile. Check the job YAML reference.";

/*!
 * \var USER_ERROR_JOB_96
 * \brief Message when ProRes \c vendor is not exactly four ASCII characters.
 */
inline constexpr std::string_view USER_ERROR_JOB_96 =
    "output.videos[].codec_options.vendor: must be a quoted string of "
    "exactly four ASCII characters (e.g. \"apl0\").";

/*!
 * \var USER_ERROR_JOB_97
 * \brief Message when ProRes \c alpha_bits is set (alpha is not supported).
 */
inline constexpr std::string_view USER_ERROR_JOB_97 =
    "output.videos[].codec_options.alpha_bits: not supported (source images "
    "have no alpha).";

/*!
 * \var USER_ERROR_JOB_98
 * \brief Message when ProRes \c bits_per_mb is outside 0..8192.
 */
inline constexpr std::string_view USER_ERROR_JOB_98 =
    "output.videos[].codec_options.bits_per_mb: must be an unquoted integer "
    "in [0, 8192] (e.g. 0).";

/*!
 * \var USER_ERROR_JOB_99
 * \brief Message when ProRes \c mbs_per_slice is outside 1..8.
 */
inline constexpr std::string_view USER_ERROR_JOB_99 =
    "output.videos[].codec_options.mbs_per_slice: must be an unquoted "
    "integer in [1, 8] (e.g. 8).";

/*!
 * \var USER_ERROR_JOB_100
 * \brief Message when \c plans[].sequence.handles.head or \c tail is negative.
 */
inline constexpr std::string_view USER_ERROR_JOB_100 =
    "plans[].sequence.handles: must be a map; head and tail must be "
    "unquoted integers >= 0.";

/*!
 * \var USER_ERROR_JOB_101
 * \brief Message when \c plans[].timecode.start is missing or wrong type.
 */
inline constexpr std::string_view USER_ERROR_JOB_101 =
    "plans[].timecode.start: missing required key. Set a string "
    "(HH:MM:SS:FF) or an unquoted integer >= 0.";

/*!
 * \var USER_ERROR_JOB_102
 * \brief Message when \c plans[].timecode.start SMPTE string is invalid.
 */
inline constexpr std::string_view USER_ERROR_JOB_102 =
    "plans[].timecode.start: invalid SMPTE timecode (expected HH:MM:SS:FF "
    "or HH:MM:SS;FF with drop_frame).";

/*!
 * \var USER_ERROR_JOB_103
 * \brief Message when drop-frame is set with a non-NTSC integer rate.
 */
inline constexpr std::string_view USER_ERROR_JOB_103 =
    "plans[].timecode.drop_frame: true requires resolved fps 30 or 60.";

/*!
 * \var USER_ERROR_JOB_104
 * \brief Message when enabled videos disagree on fps with timecode set.
 */
inline constexpr std::string_view USER_ERROR_JOB_104 =
    "plans[].timecode: all enabled output.videos[] must share the same fps.";

/*!
 * \var USER_ERROR_JOB_105
 * \brief Message when burn-in \c box.mode is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_105 =
    "layout.burn_ins[].box.mode: must be a quoted string (\"fill\" or "
    "\"outline\").";

/*!
 * \var USER_ERROR_JOB_106
 * \brief Message when burn-in \c box.color is not an r/g/b map.
 */
inline constexpr std::string_view USER_ERROR_JOB_106 =
    "layout.burn_ins[].box.color: must be a map with keys r, g, and b only.";

/*!
 * \var USER_ERROR_JOB_107
 * \brief Message when a burn-in box color channel is outside [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_107 =
    "layout.burn_ins[].box.color: r, g, and b must be numbers in [0, 1] "
    "(e.g. r: 0.12).";

/*!
 * \var USER_ERROR_JOB_108
 * \brief Message when burn-in \c box.opacity is outside [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_108 =
    "layout.burn_ins[].box.opacity: must be a number in [0, 1] (e.g. 0.45).";

/*!
 * \var USER_ERROR_JOB_109
 * \brief Message when burn-in \c box.margin is not an int >= 0 or side map.
 */
inline constexpr std::string_view USER_ERROR_JOB_109 =
    "layout.burn_ins[].box.margin: must be an unquoted integer >= 0 or a "
    "per-side map of top, right, bottom, left.";

/*!
 * \var USER_ERROR_JOB_110
 * \brief Message when \c layout.slate omits \c duration_frames.
 */
inline constexpr std::string_view USER_ERROR_JOB_110 =
    "layout.slate.duration_frames: missing required key. Set an unquoted "
    "integer >= 0.";

/*!
 * \var USER_ERROR_JOB_111
 * \brief Message when \c layout.slate.duration_frames is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_111 =
    "layout.slate.duration_frames: must be an unquoted integer.";

/*!
 * \var USER_ERROR_JOB_112
 * \brief Message when \c layout.slate.duration_frames is negative.
 */
inline constexpr std::string_view USER_ERROR_JOB_112 =
    "layout.slate.duration_frames: must be an unquoted integer >= 0.";

/*!
 * \var USER_ERROR_JOB_113
 * \brief Message when \c layout.slate.lines is missing or empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_113 =
    "layout.slate.lines: add at least one line, or omit the slate block.";

/*!
 * \var USER_ERROR_JOB_114
 * \brief Message when a slate line omits \c text.
 */
inline constexpr std::string_view USER_ERROR_JOB_114 =
    "layout.slate.lines[].text: missing required key. Set a quoted text "
    "string.";

/*!
 * \var USER_ERROR_JOB_115
 * \brief Message when a slate line omits \c position.
 */
inline constexpr std::string_view USER_ERROR_JOB_115 =
    "layout.slate.lines[].position: missing required key. Add a position "
    "block.";

/*!
 * \var USER_ERROR_JOB_116
 * \brief Message when a slate line omits \c font.
 */
inline constexpr std::string_view USER_ERROR_JOB_116 =
    "layout.slate.lines[].font: missing required key. Add a font block with "
    "path and size_px.";

/*!
 * \var USER_ERROR_JOB_117
 * \brief Message when a slate line \c text is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_117 =
    "layout.slate.lines[].text: must be a quoted string (e.g. \"Shot "
    "{shot}\").";

/*!
 * \var USER_ERROR_JOB_118
 * \brief Message when a plan omits \c id.
 */
inline constexpr std::string_view USER_ERROR_JOB_118 =
    "plans[].id: missing required key. Set a quoted non-empty id.";

/*!
 * \var USER_ERROR_JOB_119
 * \brief Message when a plan \c id is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_119 =
    "plans[].id: must be a quoted non-empty string (e.g. \"plate\").";

/*!
 * \var USER_ERROR_JOB_120
 * \brief Message when a plan omits \c input_colorspace.
 */
inline constexpr std::string_view USER_ERROR_JOB_120 =
    "plans[].input_colorspace: missing required key. Set a quoted OCIO "
    "colorspace name.";

/*!
 * \var USER_ERROR_JOB_121
 * \brief Message when \c input_colorspace is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_121 =
    "plans[].input_colorspace: must be a quoted non-empty string (e.g. "
    "\"ACEScg\").";

/*!
 * \var USER_ERROR_JOB_122
 * \brief Message when a plan omits \c sequence.
 */
inline constexpr std::string_view USER_ERROR_JOB_122 =
    "plans[].sequence: missing required key. Add sequence with path, "
    "frame_start, and frame_end.";

/*!
 * \var USER_ERROR_JOB_123
 * \brief Message when \c sequence.path is missing.
 */
inline constexpr std::string_view USER_ERROR_JOB_123 =
    "plans[].sequence.path: missing required key. Set a quoted file "
    "sequence pattern.";

/*!
 * \var USER_ERROR_JOB_124
 * \brief Message when \c sequence.path is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_124 =
    "plans[].sequence.path: must be a quoted non-empty string (e.g. "
    "\"plate.%04d.png\").";

/*!
 * \var USER_ERROR_JOB_125
 * \brief Message when \c sequence.frame_start is missing.
 */
inline constexpr std::string_view USER_ERROR_JOB_125 =
    "plans[].sequence.frame_start: missing required key. Set an unquoted "
    "integer.";

/*!
 * \var USER_ERROR_JOB_126
 * \brief Message when \c sequence.frame_start is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_126 =
    "plans[].sequence.frame_start: must be an unquoted integer.";

/*!
 * \var USER_ERROR_JOB_127
 * \brief Message when \c sequence.frame_end is missing.
 */
inline constexpr std::string_view USER_ERROR_JOB_127 =
    "plans[].sequence.frame_end: missing required key. Set an unquoted "
    "integer.";

/*!
 * \var USER_ERROR_JOB_128
 * \brief Message when \c sequence.frame_end is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_128 =
    "plans[].sequence.frame_end: must be an unquoted integer.";

/*!
 * \var USER_ERROR_JOB_129
 * \brief Message when \c audio is present without \c path.
 */
inline constexpr std::string_view USER_ERROR_JOB_129 =
    "plans[].audio.path: missing required key. Set a quoted audio file "
    "path.";

/*!
 * \var USER_ERROR_JOB_130
 * \brief Message when \c audio.path is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_130 =
    "plans[].audio.path: must be a quoted non-empty string (e.g. "
    "\"/audio/guide.wav\").";

/*!
 * \var USER_ERROR_JOB_131
 * \brief Message when \c dailyboy_version is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_131 =
    "dailyboy_version: must be the unquoted integer 1 (e.g. "
    "dailyboy_version: 1).";

/*!
 * \var USER_ERROR_JOB_132
 * \brief Message when \c layout.pixel_aspect is not strictly positive.
 */
inline constexpr std::string_view USER_ERROR_JOB_132 =
    "layout.pixel_aspect: must be a number > 0 (e.g. 1.0).";

/*!
 * \var USER_ERROR_JOB_133
 * \brief Message when \c color.context is not a map.
 */
inline constexpr std::string_view USER_ERROR_JOB_133 =
    "color.context: must be a map of quoted name: value (e.g. \"SHOT\": "
    "\"sh010\").";

/*!
 * \var USER_ERROR_JOB_134
 * \brief Message when a \c color.context key is empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_134 =
    "color.context: names must be non-empty quoted strings (e.g. \"SHOT\").";

/*!
 * \var USER_ERROR_JOB_135
 * \brief Message when a \c color.context value is empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_135 =
    "color.context: values must be non-empty quoted strings (e.g. "
    "\"sh010\").";

/*!
 * \var USER_ERROR_JOB_136
 * \brief Message when a text position omits \c mode.
 */
inline constexpr std::string_view USER_ERROR_JOB_136 =
    "layout.position.mode: missing required key. Set \"layout\", \"pixel\", "
    "or \"percent\".";

/*!
 * \var USER_ERROR_JOB_137
 * \brief Message when a text font omits \c path.
 */
inline constexpr std::string_view USER_ERROR_JOB_137 =
    "layout.font.path: missing required key. Set a quoted path to a .ttf or "
    ".otf file.";

/*!
 * \var USER_ERROR_JOB_138
 * \brief Message when a text font omits \c size_px.
 */
inline constexpr std::string_view USER_ERROR_JOB_138 =
    "layout.font.size_px: missing required key. Set an unquoted integer >= 4 "
    "(e.g. 18).";

/*!
 * \var USER_ERROR_JOB_139
 * \brief Message when a font file path does not exist on disk.
 */
inline constexpr std::string_view USER_ERROR_JOB_139 =
    "layout.font.path: file not found. Set a path to an existing .ttf or "
    ".otf file (after resolving substitutions).";

/*!
 * \var USER_ERROR_JOB_140
 * \brief Message when \c handles.head is not an integer >= 0.
 */
inline constexpr std::string_view USER_ERROR_JOB_140 =
    "plans[].sequence.handles.head: must be an unquoted integer >= 0 (e.g. "
    "8).";

/*!
 * \var USER_ERROR_JOB_141
 * \brief Message when \c handles.tail is not an integer >= 0.
 */
inline constexpr std::string_view USER_ERROR_JOB_141 =
    "plans[].sequence.handles.tail: must be an unquoted integer >= 0 (e.g. "
    "8).";

/*!
 * \var USER_ERROR_JOB_142
 * \brief Message when \c timecode.drop_frame is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_142 =
    "plans[].timecode.drop_frame: must be a boolean (true or false).";

/*!
 * \var USER_ERROR_JOB_143
 * \brief Message when an output video omits \c id.
 */
inline constexpr std::string_view USER_ERROR_JOB_143 =
    "output.videos[].id: missing required key. Set a quoted non-empty id.";

/*!
 * \var USER_ERROR_JOB_144
 * \brief Message when an output video \c id is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_144 =
    "output.videos[].id: must be a quoted non-empty string (e.g. \"review\").";

/*!
 * \var USER_ERROR_JOB_145
 * \brief Message when an output video omits \c enabled.
 */
inline constexpr std::string_view USER_ERROR_JOB_145 =
    "output.videos[].enabled: missing required key. Set true or false.";

/*!
 * \var USER_ERROR_JOB_146
 * \brief Message when an output video \c enabled is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_146 =
    "output.videos[].enabled: must be a boolean (true or false).";

/*!
 * \var USER_ERROR_JOB_147
 * \brief Message when an output video omits \c display_view.
 */
inline constexpr std::string_view USER_ERROR_JOB_147 =
    "output.videos[].display_view: missing required key. Add a display_view "
    "block with display and view.";

/*!
 * \var USER_ERROR_JOB_148
 * \brief Message when an output video omits \c display_view.display.
 */
inline constexpr std::string_view USER_ERROR_JOB_148 =
    "output.videos[].display_view.display: missing required key. Set a quoted "
    "OCIO display name.";

/*!
 * \var USER_ERROR_JOB_149
 * \brief Message when an output video \c display is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_149 =
    "output.videos[].display_view.display: must be a quoted non-empty string "
    "(e.g. \"Rec.1886 Rec.709 - Display\").";

/*!
 * \var USER_ERROR_JOB_150
 * \brief Message when an output video omits \c display_view.view.
 */
inline constexpr std::string_view USER_ERROR_JOB_150 =
    "output.videos[].display_view.view: missing required key. Set a quoted "
    "OCIO view name.";

/*!
 * \var USER_ERROR_JOB_151
 * \brief Message when an output video \c view is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_151 =
    "output.videos[].display_view.view: must be a quoted non-empty string "
    "(e.g. \"ACES 2.0 - SDR 100 nits (Rec.709)\").";

/*!
 * \var USER_ERROR_JOB_153
 * \brief Message when an output video signal omits \c range.
 */
inline constexpr std::string_view USER_ERROR_JOB_153 =
    "output.videos[].signal.range: missing required key. Set \"tv\" or "
    "\"pc\".";

/*!
 * \var USER_ERROR_JOB_154
 * \brief Message when an output video \c signal.range is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_154 =
    "output.videos[].signal.range: must be a quoted string (\"tv\" or "
    "\"pc\").";

/*!
 * \var USER_ERROR_JOB_155
 * \brief Message when an output video \c signal.range is not tv or pc.
 */
inline constexpr std::string_view USER_ERROR_JOB_155 =
    "output.videos[].signal.range: unsupported value. Use \"tv\" or \"pc\".";

/*!
 * \var USER_ERROR_JOB_156
 * \brief Message when an output video signal omits \c matrix.
 */
inline constexpr std::string_view USER_ERROR_JOB_156 =
    "output.videos[].signal.matrix: missing required key. Set \"bt709\".";

/*!
 * \var USER_ERROR_JOB_157
 * \brief Message when an output video \c signal.matrix is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_157 =
    "output.videos[].signal.matrix: must be a quoted string (\"bt709\").";

/*!
 * \var USER_ERROR_JOB_158
 * \brief Message when an output video \c signal.matrix is not bt709.
 */
inline constexpr std::string_view USER_ERROR_JOB_158 =
    "output.videos[].signal.matrix: unsupported value. Use \"bt709\".";

/*!
 * \var USER_ERROR_JOB_159
 * \brief Message when an output video signal omits \c primaries.
 */
inline constexpr std::string_view USER_ERROR_JOB_159 =
    "output.videos[].signal.primaries: missing required key. Set \"bt709\".";

/*!
 * \var USER_ERROR_JOB_160
 * \brief Message when an output video \c signal.primaries is not a quoted
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_160 =
    "output.videos[].signal.primaries: must be a quoted string (\"bt709\").";

/*!
 * \var USER_ERROR_JOB_161
 * \brief Message when an output video \c signal.primaries is not bt709.
 */
inline constexpr std::string_view USER_ERROR_JOB_161 =
    "output.videos[].signal.primaries: unsupported value. Use \"bt709\".";

/*!
 * \var USER_ERROR_JOB_162
 * \brief Message when an output video signal omits \c transfer.
 */
inline constexpr std::string_view USER_ERROR_JOB_162 =
    "output.videos[].signal.transfer: missing required key. Set \"bt709\".";

/*!
 * \var USER_ERROR_JOB_163
 * \brief Message when an output video \c signal.transfer is not a quoted
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_163 =
    "output.videos[].signal.transfer: must be a quoted string (\"bt709\").";

/*!
 * \var USER_ERROR_JOB_164
 * \brief Message when an output video \c signal.transfer is not bt709.
 */
inline constexpr std::string_view USER_ERROR_JOB_164 =
    "output.videos[].signal.transfer: unsupported value. Use \"bt709\".";

/*!
 * \var USER_ERROR_JOB_165
 * \brief Message when an output video omits \c path.
 */
inline constexpr std::string_view USER_ERROR_JOB_165 =
    "output.videos[].path: missing required key. Set a quoted .mov path.";

/*!
 * \var USER_ERROR_JOB_166
 * \brief Message when an output video \c path is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_166 =
    "output.videos[].path: must be a quoted non-empty string (e.g. "
    "\"/out/review.mov\").";

/*!
 * \var USER_ERROR_JOB_167
 * \brief Message when an output video \c path does not end with \c .mov.
 */
inline constexpr std::string_view USER_ERROR_JOB_167 =
    "output.videos[].path: must end with .mov (case-insensitive) after "
    "resolving tokens.";

/*!
 * \var USER_ERROR_JOB_168
 * \brief Message when an output video \c fps is not an integer \c >= 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_168 =
    "output.videos[].fps: must be an unquoted integer >= 1 (e.g. 24).";

/*!
 * \var USER_ERROR_JOB_169
 * \brief Message when an output video omits \c codec.
 */
inline constexpr std::string_view USER_ERROR_JOB_169 =
    "output.videos[].codec: missing required key. Set \"h264\", \"mjpeg\", "
    "\"dnxhd\", or \"prores\".";

/*!
 * \var USER_ERROR_JOB_170
 * \brief Message when an output video \c codec is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_170 =
    "output.videos[].codec: must be a quoted non-empty string (e.g. "
    "\"h264\").";

/*!
 * \var USER_ERROR_JOB_171
 * \brief Message when an output video \c codec is not a supported encoder.
 */
inline constexpr std::string_view USER_ERROR_JOB_171 =
    "output.videos[].codec: unsupported value. Use \"h264\", \"mjpeg\", "
    "\"dnxhd\", or \"prores\".";

/*!
 * \var USER_ERROR_JOB_172
 * \brief Message when H.264 \c preset is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_172 =
    "output.videos[].codec_options.preset: must be a quoted string (e.g. "
    "\"medium\").";

/*!
 * \var USER_ERROR_JOB_173
 * \brief Message when H.264 \c preset is not a supported x264 preset.
 */
inline constexpr std::string_view USER_ERROR_JOB_173 =
    "output.videos[].codec_options.preset: unsupported value. Use "
    "\"ultrafast\", \"superfast\", \"veryfast\", \"faster\", \"fast\", "
    "\"medium\", or \"slow\".";

/*!
 * \var USER_ERROR_JOB_174
 * \brief Message when H.264 \c crf is not a number in \c [0, 51].
 */
inline constexpr std::string_view USER_ERROR_JOB_174 =
    "output.videos[].codec_options.crf: must be a number in [0, 51] (e.g. "
    "23).";

/*!
 * \var USER_ERROR_JOB_175
 * \brief Message when H.264 \c bitrate_kbps is not an integer \c >= 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_175 =
    "output.videos[].codec_options.bitrate_kbps: must be an unquoted integer "
    ">= 1 (e.g. 8000).";

/*!
 * \var USER_ERROR_JOB_176
 * \brief Message when H.264 \c gop is not an integer \c >= 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_176 =
    "output.videos[].codec_options.gop: must be an unquoted integer >= 1 "
    "(e.g. 250).";

/*!
 * \var USER_ERROR_JOB_177
 * \brief Message when H.264 \c pix_fmt is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_177 =
    "output.videos[].codec_options.pix_fmt: must be a quoted string "
    "(\"yuv420p\" or \"yuv422p\").";

/*!
 * \var USER_ERROR_JOB_178
 * \brief Message when H.264 \c pix_fmt is not yuv420p or yuv422p.
 */
inline constexpr std::string_view USER_ERROR_JOB_178 =
    "output.videos[].codec_options.pix_fmt: unsupported value. Use "
    "\"yuv420p\" or \"yuv422p\".";

/*!
 * \var USER_ERROR_JOB_179
 * \brief Message when H.264 \c tune is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_179 =
    "output.videos[].codec_options.tune: must be a quoted string (e.g. "
    "\"film\").";

/*!
 * \var USER_ERROR_JOB_180
 * \brief Message when H.264 \c tune is not a supported x264 tune.
 */
inline constexpr std::string_view USER_ERROR_JOB_180 =
    "output.videos[].codec_options.tune: unsupported value. Use \"film\", "
    "\"animation\", \"grain\", \"stillimage\", \"fastdecode\", or "
    "\"zerolatency\".";

/*!
 * \var USER_ERROR_JOB_181
 * \brief Message when H.264 \c profile is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_181 =
    "output.videos[].codec_options.profile: must be a quoted string (e.g. "
    "\"high\").";

/*!
 * \var USER_ERROR_JOB_182
 * \brief Message when H.264 \c profile is not a supported profile.
 */
inline constexpr std::string_view USER_ERROR_JOB_182 =
    "output.videos[].codec_options.profile: unsupported value. Use "
    "\"baseline\", \"main\", \"high\", or \"high422\".";

/*!
 * \var USER_ERROR_JOB_184
 * \brief Message when H.264 \c level is not a supported level.
 */
inline constexpr std::string_view USER_ERROR_JOB_184 =
    "output.videos[].codec_options.level: unsupported value. Use \"3.0\", "
    "\"3.1\", \"4.0\", \"4.1\", \"4.2\", \"5.0\", \"5.1\", or \"5.2\".";

/*!
 * \var USER_ERROR_JOB_185
 * \brief Message when \c codec_options.faststart is not a boolean.
 *
 * Shared by H.264, MJPEG, DNxHD, and ProRes.
 */
inline constexpr std::string_view USER_ERROR_JOB_185 =
    "output.videos[].codec_options.faststart: must be a boolean (true or "
    "false).";

/*!
 * \var USER_ERROR_JOB_186
 * \brief Message when MJPEG \c pix_fmt is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_186 =
    "output.videos[].codec_options.pix_fmt: must be a quoted string "
    "(\"yuv420p\", \"yuv422p\", or \"yuv444p\").";

/*!
 * \var USER_ERROR_JOB_187
 * \brief Message when MJPEG \c pix_fmt is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_187 =
    "output.videos[].codec_options.pix_fmt: unsupported value. Use "
    "\"yuv420p\", \"yuv422p\", or \"yuv444p\".";

/*!
 * \var USER_ERROR_JOB_188
 * \brief Message when MJPEG \c huffman is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_188 =
    "output.videos[].codec_options.huffman: must be a quoted string "
    "(\"default\" or \"optimal\").";

/*!
 * \var USER_ERROR_JOB_189
 * \brief Message when MJPEG \c huffman is not default or optimal.
 */
inline constexpr std::string_view USER_ERROR_JOB_189 =
    "output.videos[].codec_options.huffman: unsupported value. Use "
    "\"default\" or \"optimal\".";

/*!
 * \var USER_ERROR_JOB_190
 * \brief Message when DNxHD \c profile is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_190 =
    "output.videos[].codec_options.profile: must be a quoted string (e.g. "
    "\"dnxhr_hq\").";

/*!
 * \var USER_ERROR_JOB_191
 * \brief Message when DNxHD \c profile is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_191 =
    "output.videos[].codec_options.profile: unsupported value. Use "
    "\"dnxhd\", \"dnxhr_lb\", \"dnxhr_sq\", \"dnxhr_hq\", \"dnxhr_hqx\", or "
    "\"dnxhr_444\".";

/*!
 * \var USER_ERROR_JOB_192
 * \brief Message when DNxHD \c pix_fmt is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_192 =
    "output.videos[].codec_options.pix_fmt: must be a quoted string "
    "(\"yuv422p\", \"yuv422p10\", or \"yuv444p10\").";

/*!
 * \var USER_ERROR_JOB_193
 * \brief Message when DNxHD \c pix_fmt is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_193 =
    "output.videos[].codec_options.pix_fmt: unsupported value. Use "
    "\"yuv422p\", \"yuv422p10\", or \"yuv444p10\".";

/*!
 * \var USER_ERROR_JOB_194
 * \brief Message when DNxHD \c interlaced is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_194 =
    "output.videos[].codec_options.interlaced: must be a boolean (true or "
    "false).";

/*!
 * \var USER_ERROR_JOB_195
 * \brief Message when DNxHD \c nitris_compat is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_195 =
    "output.videos[].codec_options.nitris_compat: must be a boolean (true or "
    "false).";

/*!
 * \var USER_ERROR_JOB_196
 * \brief Message when DNxHD uses \c signal.range pc.
 */
inline constexpr std::string_view USER_ERROR_JOB_196 =
    "output.videos[].signal.range: \"pc\" is not allowed with codec "
    "\"dnxhd\". Use \"tv\".";

/*!
 * \var USER_ERROR_JOB_197
 * \brief Message when ProRes \c profile is not a quoted string (or 4444).
 */
inline constexpr std::string_view USER_ERROR_JOB_197 =
    "output.videos[].codec_options.profile: must be a quoted string (e.g. "
    "\"hq\") or the unquoted integer 4444.";

/*!
 * \var USER_ERROR_JOB_198
 * \brief Message when ProRes \c profile is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_198 =
    "output.videos[].codec_options.profile: unsupported value. Use "
    "\"proxy\", \"lt\", \"standard\", \"hq\", \"4444\", or \"4444xq\".";

/*!
 * \var USER_ERROR_JOB_199
 * \brief Message when ProRes \c pix_fmt is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_199 =
    "output.videos[].codec_options.pix_fmt: must be a quoted string "
    "(\"yuv422p10\" or \"yuv444p10\").";

/*!
 * \var USER_ERROR_JOB_200
 * \brief Message when ProRes \c pix_fmt is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_200 =
    "output.videos[].codec_options.pix_fmt: unsupported value. Use "
    "\"yuv422p10\" or \"yuv444p10\".";

/*!
 * \var USER_ERROR_JOB_201
 * \brief Message when ProRes \c quant_mat is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_201 =
    "output.videos[].codec_options.quant_mat: must be a quoted string (e.g. "
    "\"auto\").";

/*!
 * \var USER_ERROR_JOB_202
 * \brief Message when ProRes \c quant_mat is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_202 =
    "output.videos[].codec_options.quant_mat: unsupported value. Use "
    "\"auto\", \"proxy\", \"lt\", \"standard\", \"hq\", or \"default\".";

/*!
 * \var USER_ERROR_JOB_203
 * \brief Message when ProRes \c vendor is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_203 =
    "output.videos[].codec_options.vendor: must be a quoted string of exactly "
    "four ASCII characters (e.g. \"apl0\").";

/*!
 * \var USER_ERROR_JOB_204
 * \brief Message when an image sequence omits \c id.
 */
inline constexpr std::string_view USER_ERROR_JOB_204 =
    "output.image_sequences[].id: missing required key. Set a quoted "
    "non-empty id.";

/*!
 * \var USER_ERROR_JOB_205
 * \brief Message when an image sequence \c id is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_205 =
    "output.image_sequences[].id: must be a quoted non-empty string (e.g. "
    "\"archive\").";

/*!
 * \var USER_ERROR_JOB_206
 * \brief Message when an image sequence omits \c enabled.
 */
inline constexpr std::string_view USER_ERROR_JOB_206 =
    "output.image_sequences[].enabled: missing required key. Set true or "
    "false.";

/*!
 * \var USER_ERROR_JOB_207
 * \brief Message when an image sequence \c enabled is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_207 =
    "output.image_sequences[].enabled: must be a boolean (true or false).";

/*!
 * \var USER_ERROR_JOB_208
 * \brief Message when an image sequence omits \c display_view.
 */
inline constexpr std::string_view USER_ERROR_JOB_208 =
    "output.image_sequences[].display_view: missing required key. Add a "
    "display_view block with display and view.";

/*!
 * \var USER_ERROR_JOB_209
 * \brief Message when an image sequence omits \c display_view.display.
 */
inline constexpr std::string_view USER_ERROR_JOB_209 =
    "output.image_sequences[].display_view.display: missing required key. Set "
    "a quoted OCIO display name.";

/*!
 * \var USER_ERROR_JOB_210
 * \brief Message when an image sequence \c display is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_210 =
    "output.image_sequences[].display_view.display: must be a quoted "
    "non-empty string (e.g. \"Rec.1886 Rec.709 - Display\").";

/*!
 * \var USER_ERROR_JOB_211
 * \brief Message when an image sequence omits \c display_view.view.
 */
inline constexpr std::string_view USER_ERROR_JOB_211 =
    "output.image_sequences[].display_view.view: missing required key. Set a "
    "quoted OCIO view name.";

/*!
 * \var USER_ERROR_JOB_212
 * \brief Message when an image sequence \c view is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_212 =
    "output.image_sequences[].display_view.view: must be a quoted non-empty "
    "string (e.g. \"ACES 2.0 - SDR 100 nits (Rec.709)\").";

/*!
 * \var USER_ERROR_JOB_213
 * \brief Message when an image sequence omits \c path_pattern.
 */
inline constexpr std::string_view USER_ERROR_JOB_213 =
    "output.image_sequences[].path_pattern: missing required key. Set a "
    "quoted path pattern.";

/*!
 * \var USER_ERROR_JOB_214
 * \brief Message when an image sequence \c path_pattern is not a quoted
 *        non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_214 =
    "output.image_sequences[].path_pattern: must be a quoted non-empty string "
    "(e.g. \"/out/plate.####.exr\").";

/*!
 * \var USER_ERROR_JOB_215
 * \brief Message when \c format_options is an empty map.
 */
inline constexpr std::string_view USER_ERROR_JOB_215 =
    "output.image_sequences[].format_options: must not be an empty map. Omit "
    "the key or set at least one option.";

/*!
 * \var USER_ERROR_JOB_216
 * \brief Message when PNG \c bit_depth is not 8 or 16.
 */
inline constexpr std::string_view USER_ERROR_JOB_216 =
    "output.image_sequences[].format_options.bit_depth: must be 8 or 16.";

/*!
 * \var USER_ERROR_JOB_217
 * \brief Message when PNG \c compression is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_217 =
    "output.image_sequences[].format_options.compression: must be a quoted "
    "string (e.g. \"default\").";

/*!
 * \var USER_ERROR_JOB_218
 * \brief Message when PNG \c compression is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_218 =
    "output.image_sequences[].format_options.compression: unsupported value. "
    "Use \"default\", \"filtered\", \"huffman\", \"rle\", \"fixed\", "
    "\"pngfast\", or \"none\".";

/*!
 * \var USER_ERROR_JOB_219
 * \brief Message when PNG \c compression_level is outside 0..6.
 */
inline constexpr std::string_view USER_ERROR_JOB_219 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer in [0, 6].";

/*!
 * \var USER_ERROR_JOB_220
 * \brief Message when PNG \c filter is outside 0..4.
 */
inline constexpr std::string_view USER_ERROR_JOB_220 =
    "output.image_sequences[].format_options.filter: must be an unquoted "
    "integer in [0, 4].";

/*!
 * \var USER_ERROR_JOB_221
 * \brief Message when JPEG \c bit_depth is not 8.
 */
inline constexpr std::string_view USER_ERROR_JOB_221 =
    "output.image_sequences[].format_options.bit_depth: must be 8.";

/*!
 * \var USER_ERROR_JOB_222
 * \brief Message when JPEG \c compression_level is outside 1..100.
 */
inline constexpr std::string_view USER_ERROR_JOB_222 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer in [1, 100].";

/*!
 * \var USER_ERROR_JOB_223
 * \brief Message when JPEG \c subsampling is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_223 =
    "output.image_sequences[].format_options.subsampling: must be a quoted "
    "string (e.g. \"4:2:0\").";

/*!
 * \var USER_ERROR_JOB_224
 * \brief Message when JPEG \c subsampling is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_224 =
    "output.image_sequences[].format_options.subsampling: unsupported value. "
    "Use \"4:4:4\", \"4:2:2\", \"4:2:0\", or \"4:1:1\".";

/*!
 * \var USER_ERROR_JOB_225
 * \brief Message when JPEG \c progressive is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_225 =
    "output.image_sequences[].format_options.progressive: must be a boolean "
    "(true or false).";

/*!
 * \var USER_ERROR_JOB_226
 * \brief Message when TIFF \c bit_depth is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_226 =
    "output.image_sequences[].format_options.bit_depth: unsupported value. "
    "Use 1, 2, 4, 6, 8, 10, 12, 14, 16, 24, 32, 64, \"h16\", or \"f32\".";

/*!
 * \var USER_ERROR_JOB_227
 * \brief Message when TIFF \c bigtiff is not a boolean.
 */
inline constexpr std::string_view USER_ERROR_JOB_227 =
    "output.image_sequences[].format_options.bigtiff: must be a boolean (true "
    "or false).";

/*!
 * \var USER_ERROR_JOB_228
 * \brief Message when TIFF \c compression is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_228 =
    "output.image_sequences[].format_options.compression: must be a quoted "
    "string (e.g. \"zip\").";

/*!
 * \var USER_ERROR_JOB_229
 * \brief Message when TIFF \c compression is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_229 =
    "output.image_sequences[].format_options.compression: unsupported value. "
    "Use \"none\", \"lzw\", \"zip\", \"ccittrle\", or \"packbits\".";

/*!
 * \var USER_ERROR_JOB_230
 * \brief Message when TIFF \c compression_level is outside 1..9.
 */
inline constexpr std::string_view USER_ERROR_JOB_230 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer in [1, 9].";

/*!
 * \var USER_ERROR_JOB_231
 * \brief Message when EXR \c bit_depth is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_231 =
    "output.image_sequences[].format_options.bit_depth: must be a quoted "
    "string (\"h16\" or \"f32\").";

/*!
 * \var USER_ERROR_JOB_232
 * \brief Message when EXR \c bit_depth is not h16 or f32.
 */
inline constexpr std::string_view USER_ERROR_JOB_232 =
    "output.image_sequences[].format_options.bit_depth: unsupported value. "
    "Use \"h16\" or \"f32\".";

/*!
 * \var USER_ERROR_JOB_233
 * \brief Message when EXR \c compression is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_233 =
    "output.image_sequences[].format_options.compression: must be a quoted "
    "string (e.g. \"zip\").";

/*!
 * \var USER_ERROR_JOB_234
 * \brief Message when EXR \c compression is not a supported value.
 */
inline constexpr std::string_view USER_ERROR_JOB_234 =
    "output.image_sequences[].format_options.compression: unsupported value. "
    "Use \"none\", \"rle\", \"zips\", \"zip\", \"piz\", \"pxr24\", \"b44\", "
    "\"b44a\", \"dwaa\", or \"dwab\".";

/*!
 * \var USER_ERROR_JOB_235
 * \brief Message when EXR \c compression_level is invalid for zip/zips.
 */
inline constexpr std::string_view USER_ERROR_JOB_235 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer in [1, 9] for zip/zips.";

/*!
 * \var USER_ERROR_JOB_236
 * \brief Message when EXR \c compression_level is negative for dwaa/dwab.
 */
inline constexpr std::string_view USER_ERROR_JOB_236 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer >= 0 for dwaa/dwab.";

/*!
 * \var USER_ERROR_JOB_237
 * \brief Message when HEIF \c bit_depth is not 8.
 */
inline constexpr std::string_view USER_ERROR_JOB_237 =
    "output.image_sequences[].format_options.bit_depth: must be 8.";

/*!
 * \var USER_ERROR_JOB_238
 * \brief Message when HEIF \c compression is not a quoted string.
 */
inline constexpr std::string_view USER_ERROR_JOB_238 =
    "output.image_sequences[].format_options.compression: must be a quoted "
    "string (\"heic\" or \"avif\").";

/*!
 * \var USER_ERROR_JOB_239
 * \brief Message when HEIF \c compression is not heic or avif.
 */
inline constexpr std::string_view USER_ERROR_JOB_239 =
    "output.image_sequences[].format_options.compression: unsupported value. "
    "Use \"heic\" or \"avif\" (\"hevc\" is accepted as an alias for "
    "\"heic\").";

/*!
 * \var USER_ERROR_JOB_240
 * \brief Message when HEIF \c compression_level is outside 0..100.
 */
inline constexpr std::string_view USER_ERROR_JOB_240 =
    "output.image_sequences[].format_options.compression_level: must be an "
    "unquoted integer in [0, 100].";

/*!
 * \var INTERNAL_ERROR_JOB_1
 * \brief Message when schema validation is called with an empty path.
 */
inline constexpr std::string_view INTERNAL_ERROR_JOB_1 =
    "Internal error: job path is empty.";

/*!
 * \var INTERNAL_ERROR_JOB_2
 * \brief Message when the installed job schema file cannot be opened.
 */
inline constexpr std::string_view INTERNAL_ERROR_JOB_2 =
    "Internal error: job schema file not found.";

/*!
 * \var INTERNAL_ERROR_JOB_3
 * \brief Message when the job schema file is not valid JSON.
 */
inline constexpr std::string_view INTERNAL_ERROR_JOB_3 =
    "Internal error: invalid JSON in the job schema file.";

/*!
 * \var INTERNAL_ERROR_JOB_4
 * \brief Message when the job schema itself is rejected by the validator.
 */
inline constexpr std::string_view INTERNAL_ERROR_JOB_4 =
    "Internal error: invalid job schema.";

/*! @} */

}  // namespace dailyboy
