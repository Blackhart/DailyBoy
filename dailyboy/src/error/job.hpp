#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup job_errors Job YAML contract errors
 *  @{
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
    "metadata.substitutions: values must be a quoted string (e.g. \"103\") or "
    "a frame map.";

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
    "metadata: missing required key 'substitutions'. Add at least one name "
    "(e.g. shot: \"sh010\"), or remove the metadata block.";

/*!
 * \var USER_ERROR_JOB_6
 * \brief Message when \c metadata.substitutions is an empty map.
 */
inline constexpr std::string_view USER_ERROR_JOB_6 =
    "metadata.substitutions: add at least one name (e.g. shot: \"sh010\"), or "
    "remove the metadata block.";

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
    "sequence: empty file sequence pattern. Set a path with padding (e.g. "
    "plate.%04d.png).";

/*!
 * \var USER_ERROR_JOB_9
 * \brief Message when libfileseq rejects a sequence pattern.
 */
inline constexpr std::string_view USER_ERROR_JOB_9 =
    "sequence: invalid file sequence pattern. Use ShotGrid/libfileseq padding "
    "(#, @, %04d, $F4).";

/*!
 * \var USER_ERROR_JOB_10
 * \brief Message when a field is not the YAML type required by the contract.
 */
inline constexpr std::string_view USER_ERROR_JOB_10 =
    "job: this field has the wrong YAML type. Use a map or list as required "
    "by the job YAML reference.";

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
    "color.ocio_config: missing required key. Set it to a config.ocio path.";

/*!
 * \var USER_ERROR_JOB_18
 * \brief Message when a YAML scalar cannot be converted to the expected type.
 */
inline constexpr std::string_view USER_ERROR_JOB_18 =
    "job: this field has an invalid value. Check the type in the job YAML "
    "reference.";

/*!
 * \var USER_ERROR_JOB_19
 * \brief Message when a layout anchor is not one of the nine-point names.
 */
inline constexpr std::string_view USER_ERROR_JOB_19 =
    "layout: unsupported position anchor. Use top_left, top_center, "
    "top_right, center_left, center_center, center_right, bottom_left, "
    "bottom_center, or bottom_right.";

/*!
 * \var USER_ERROR_JOB_20
 * \brief Message when a text position mode is not layout, pixel, or percent.
 */
inline constexpr std::string_view USER_ERROR_JOB_20 =
    "layout: unsupported position mode. Use layout, pixel, or percent.";

/*!
 * \var USER_ERROR_JOB_21
 * \brief Message when a burn-in box mode is not fill or outline.
 */
inline constexpr std::string_view USER_ERROR_JOB_21 =
    "layout: unsupported burn-in box mode. Use fill or outline.";

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
    "output: crf and bitrate_kbps are mutually exclusive. Keep only one.";

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
    "output: profile high422 requires pix_fmt yuv422p.";

/*!
 * \var USER_ERROR_JOB_27
 * \brief Message when H.264 \c yuv422p is used without \c high422.
 */
inline constexpr std::string_view USER_ERROR_JOB_27 =
    "output: pix_fmt yuv422p requires profile high422.";

/*!
 * \var USER_ERROR_JOB_28
 * \brief Message when H.264 \c level is not a YAML scalar.
 */
inline constexpr std::string_view USER_ERROR_JOB_28 =
    "output: level must be a scalar (e.g. 4.1).";

/*!
 * \var USER_ERROR_JOB_29
 * \brief Message when MJPEG \c qscale is outside 1..31.
 */
inline constexpr std::string_view USER_ERROR_JOB_29 =
    "output: qscale must be an integer in [1, 31].";

/*!
 * \var USER_ERROR_JOB_30
 * \brief Message when DNxHR sets \c bitrate_kbps.
 */
inline constexpr std::string_view USER_ERROR_JOB_30 =
    "output: bitrate_kbps is only valid when profile is dnxhd. Remove it for "
    "DNxHR.";

/*!
 * \var USER_ERROR_JOB_31
 * \brief Message when DNxHD omits \c bitrate_kbps.
 */
inline constexpr std::string_view USER_ERROR_JOB_31 =
    "output: bitrate_kbps is required when profile is dnxhd.";

/*!
 * \var USER_ERROR_JOB_32
 * \brief Message when DNxHD \c bitrate_kbps is less than 1.
 */
inline constexpr std::string_view USER_ERROR_JOB_32 =
    "output: bitrate_kbps must be an integer >= 1.";

/*!
 * \var USER_ERROR_JOB_33
 * \brief Message when DNxHD/HR \c pix_fmt does not match the profile.
 */
inline constexpr std::string_view USER_ERROR_JOB_33 =
    "output: pix_fmt is incompatible with this DNxHD/HR profile. Check the "
    "job YAML reference.";

/*!
 * \var USER_ERROR_JOB_34
 * \brief Message when DNxHR sets \c interlaced: true.
 */
inline constexpr std::string_view USER_ERROR_JOB_34 =
    "output: interlaced is only valid when profile is dnxhd. Set interlaced: "
    "false or omit it.";

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
    "job: this key is not allowed. Remove it or check the job YAML reference.";

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
    "metadata.substitutions: frame map keys must be a frame (e.g. \"1001\") or "
    "a frame range (e.g. \"1005-1010\").";

/*!
 * \var USER_ERROR_JOB_48
 * \brief Message when \c color.ocio_config is not a quoted non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_48 =
    "color.ocio_config: must be a quoted string (e.g. \"/path/config.ocio\"), "
    "not empty.";

/*!
 * \var USER_ERROR_JOB_49
 * \brief Message when \c color.working_colorspace is not a quoted non-empty
 *        string.
 */
inline constexpr std::string_view USER_ERROR_JOB_49 =
    "color.working_colorspace: must be a quoted string (e.g. "
    "\"ACES - ACEScg\"), not empty.";

/*!
 * \var USER_ERROR_JOB_50
 * \brief Message when a \c display_view.display or \c view is not a quoted
 *        non-empty string.
 */
inline constexpr std::string_view USER_ERROR_JOB_50 =
    "display_view: display and view must be quoted non-empty strings.";

/*!
 * \var USER_ERROR_JOB_51
 * \brief Message when \c color.context is present but empty.
 */
inline constexpr std::string_view USER_ERROR_JOB_51 =
    "color.context: add at least one key (e.g. \"SHOT\": \"sh010\"), or omit "
    "context.";

/*!
 * \var USER_ERROR_JOB_52
 * \brief Message when a \c color.context key is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_52 =
    "color.context: names must be quoted strings (e.g. \"SHOT\"), not numbers "
    "or booleans.";

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
    "layout: missing required key 'canvas'. Add canvas with even width and "
    "height.";

/*!
 * \var USER_ERROR_JOB_55
 * \brief Message when \c layout.canvas omits \c width.
 */
inline constexpr std::string_view USER_ERROR_JOB_55 =
    "layout.canvas.width: missing required key. Set an even integer (e.g. "
    "1920).";

/*!
 * \var USER_ERROR_JOB_56
 * \brief Message when \c layout.canvas.width is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_56 =
    "layout.canvas.width: must be an integer (e.g. 1920).";

/*!
 * \var USER_ERROR_JOB_57
 * \brief Message when \c layout.canvas omits \c height.
 */
inline constexpr std::string_view USER_ERROR_JOB_57 =
    "layout.canvas.height: missing required key. Set an even integer (e.g. "
    "1080).";

/*!
 * \var USER_ERROR_JOB_58
 * \brief Message when \c layout.canvas.height is not an integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_58 =
    "layout.canvas.height: must be an integer (e.g. 1080).";

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
    "layout.canvas.width: must be at least 2 (e.g. 1920).";

/*!
 * \var USER_ERROR_JOB_61
 * \brief Message when \c layout.canvas.height is less than 2.
 */
inline constexpr std::string_view USER_ERROR_JOB_61 =
    "layout.canvas.height: must be at least 2 (e.g. 1080).";

/*!
 * \var USER_ERROR_JOB_62
 * \brief Message when \c layout omits \c image.
 */
inline constexpr std::string_view USER_ERROR_JOB_62 =
    "layout: missing required key 'image'. Add image with fit contain or "
    "cover.";

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
    "layout.image.fit: must be a quoted string (e.g. \"contain\" or "
    "\"cover\").";

/*!
 * \var USER_ERROR_JOB_66
 * \brief Message when \c layout.image.filter is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_66 =
    "layout.image.filter: must be a quoted string (e.g. \"bilinear\" or "
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
    "layout.image.min_margin_px: each side must be an integer (e.g. top: 8).";

/*!
 * \var USER_ERROR_JOB_69
 * \brief Message when a \c min_margin_px side is less than 0.
 */
inline constexpr std::string_view USER_ERROR_JOB_69 =
    "layout.image.min_margin_px: each side must be at least 0 (e.g. top: 8).";

/*!
 * \var USER_ERROR_JOB_70
 * \brief Message when \c layout.background is not a map.
 */
inline constexpr std::string_view USER_ERROR_JOB_70 =
    "layout.background: must be a map of r, g, and b.";

/*!
 * \var USER_ERROR_JOB_71
 * \brief Message when a \c layout.background channel is not a number in [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_71 =
    "layout.background: each channel must be a number between 0 and 1 "
    "(e.g. r: 0.12).";

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
    "layout.burn_ins[].position: missing required key. Add a position block.";

/*!
 * \var USER_ERROR_JOB_75
 * \brief Message when a burn-in omits \c font.
 */
inline constexpr std::string_view USER_ERROR_JOB_75 =
    "layout.burn_ins[].font: missing required key. Add a font block with path "
    "and size_px.";

/*!
 * \var USER_ERROR_JOB_76
 * \brief Message when a burn-in \c template is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_76 =
    "layout.burn_ins[].template: must be a quoted string (e.g. \"{frame}\").";

/*!
 * \var USER_ERROR_JOB_77
 * \brief Message when \c position.mode is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_77 =
    "layout.burn_ins[].position.mode: must be a quoted string (e.g. "
    "\"layout\", \"pixel\", or \"percent\").";

/*!
 * \var USER_ERROR_JOB_78
 * \brief Message when \c position.anchor is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_78 =
    "layout.burn_ins[].position.anchor: must be a quoted string (e.g. "
    "\"top_left\").";

/*!
 * \var USER_ERROR_JOB_79
 * \brief Message when \c position.anchor is set and \c mode is not layout.
 */
inline constexpr std::string_view USER_ERROR_JOB_79 =
    "layout.burn_ins[].position: anchor is only valid when mode is "
    "\"layout\".";

/*!
 * \var USER_ERROR_JOB_80
 * \brief Message when \c position.x or \c y is set and \c mode is layout.
 */
inline constexpr std::string_view USER_ERROR_JOB_80 =
    "layout.burn_ins[].position: x and y are only valid when mode is "
    "\"pixel\" or \"percent\".";

/*!
 * \var USER_ERROR_JOB_81
 * \brief Message when \c position.x is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_81 =
    "layout.burn_ins[].position.x: must be an integer.";

/*!
 * \var USER_ERROR_JOB_82
 * \brief Message when \c position.y is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_82 =
    "layout.burn_ins[].position.y: must be an integer.";

/*!
 * \var USER_ERROR_JOB_83
 * \brief Message when pixel \c position.x is outside \c [0, canvas.width].
 */
inline constexpr std::string_view USER_ERROR_JOB_83 =
    "layout.burn_ins[].position.x: must be between 0 and layout.canvas.width "
    "when mode is \"pixel\".";

/*!
 * \var USER_ERROR_JOB_84
 * \brief Message when pixel \c position.y is outside \c [0, canvas.height].
 */
inline constexpr std::string_view USER_ERROR_JOB_84 =
    "layout.burn_ins[].position.y: must be between 0 and layout.canvas.height "
    "when mode is \"pixel\".";

/*!
 * \var USER_ERROR_JOB_85
 * \brief Message when percent \c position.x is outside \c [0, 100].
 */
inline constexpr std::string_view USER_ERROR_JOB_85 =
    "layout.burn_ins[].position.x: must be between 0 and 100 when mode is "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_86
 * \brief Message when percent \c position.y is outside \c [0, 100].
 */
inline constexpr std::string_view USER_ERROR_JOB_86 =
    "layout.burn_ins[].position.y: must be between 0 and 100 when mode is "
    "\"percent\".";

/*!
 * \var USER_ERROR_JOB_87
 * \brief Message when a burn-in \c font.path is not a quoted YAML string.
 */
inline constexpr std::string_view USER_ERROR_JOB_87 =
    "layout.burn_ins[].font.path: must be a quoted string (e.g. "
    "\"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf\").";

/*!
 * \var USER_ERROR_JOB_88
 * \brief Message when a burn-in \c font.size_px is not an unquoted integer.
 */
inline constexpr std::string_view USER_ERROR_JOB_88 =
    "layout.burn_ins[].font.size_px: must be an integer (e.g. 18).";

/*!
 * \var USER_ERROR_JOB_89
 * \brief Message when a burn-in \c font.size_px is below 4.
 */
inline constexpr std::string_view USER_ERROR_JOB_89 =
    "layout.burn_ins[].font.size_px: must be at least 4 (e.g. 18).";

/*!
 * \var USER_ERROR_JOB_90
 * \brief Message when a burn-in \c font.color is not a map of r, g, b.
 */
inline constexpr std::string_view USER_ERROR_JOB_90 =
    "layout.burn_ins[].font.color: must be a map of r, g, and b.";

/*!
 * \var USER_ERROR_JOB_91
 * \brief Message when a burn-in \c font.color channel is outside [0, 1].
 */
inline constexpr std::string_view USER_ERROR_JOB_91 =
    "layout.burn_ins[].font.color: each channel must be a number between 0 "
    "and 1 (e.g. r: 0.12).";

/*!
 * \var USER_ERROR_JOB_92
 * \brief Message when \c signal is missing or has an invalid field.
 */
inline constexpr std::string_view USER_ERROR_JOB_92 =
    "output.videos[].signal: requires range, matrix, primaries, and transfer.";

/*!
 * \var USER_ERROR_JOB_93
 * \brief Message when \c format_options is set but the path extension is unknown.
 */
inline constexpr std::string_view USER_ERROR_JOB_93 =
    "output.image_sequences[].format_options: path_pattern extension must be "
    "png, jpg/jpeg, tif/tiff, exr, or heif/heic/avif.";

/*!
 * \var USER_ERROR_JOB_94
 * \brief Message when \c compression_level is invalid for the chosen EXR
 *        compression.
 */
inline constexpr std::string_view USER_ERROR_JOB_94 =
    "output.image_sequences[].format_options: compression_level is only valid "
    "for zip, zips, dwaa, or dwab.";

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
