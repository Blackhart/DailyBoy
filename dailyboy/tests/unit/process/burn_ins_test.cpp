#include "process/burn_ins.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "error/process.hpp"
#include "image/frame.hpp"
#include "image/sequence.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "job/primitives.hpp"
#include "job/text.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/output.hpp"
#include "process/tokens.hpp"
#include "status.hpp"
#include "support/test_fixtures.hpp"

namespace {

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}

constexpr const char* kDejaVu =
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

OIIO::ImageBuf make_rgb(int width, int height, const float* fill) {
  OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(fill, 3)));
  return buf;
}

std::array<float, 3> pixel_rgb(const OIIO::ImageBuf& buf, int x, int y) {
  float px[3] = {0.0f, 0.0f, 0.0f};
  EXPECT_TRUE(
      buf.get_pixels(OIIO::ROI(x, x + 1, y, y + 1), OIIO::TypeDesc::FLOAT, px));
  return {px[0], px[1], px[2]};
}

void expect_rgb(const OIIO::ImageBuf& buf, int x, int y, float r, float g,
                float b) {
  const std::array<float, 3> px = pixel_rgb(buf, x, y);
  EXPECT_NEAR(px[0], r, 1e-5) << "at (" << x << "," << y << ")";
  EXPECT_NEAR(px[1], g, 1e-5) << "at (" << x << "," << y << ")";
  EXPECT_NEAR(px[2], b, 1e-5) << "at (" << x << "," << y << ")";
}

bool is_background(const std::array<float, 3>& px) {
  return px[0] < 0.05f && px[1] > 0.95f && px[2] < 0.05f;
}

dailyboy::JobLayoutBurnIn make_burn_in(
    const std::string& text, dailyboy::TextPosition position,
    const dailyboy::RGBColor& font_color,
    std::optional<dailyboy::JobLayoutBurnInBox> box) {
  dailyboy::JobLayoutBurnIn burn_in;
  burn_in.set_template_text(text);
  burn_in.set_position(std::move(position));
  dailyboy::TextFont font;
  font.set_path(kDejaVu);
  font.set_size_px(16);
  font.set_color(font_color);
  burn_in.set_font(std::move(font));
  burn_in.set_box(std::move(box));
  return burn_in;
}

dailyboy::TextPosition layout_position(
    dailyboy::TextPositionModeLayout::Anchor anchor) {
  dailyboy::TextPositionModeLayout layout;
  layout.set_anchor(anchor);
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Layout);
  position.set_value(layout);
  return position;
}

dailyboy::TextPosition pixel_position(int x, int y) {
  dailyboy::TextPositionModePixel pixel;
  pixel.set_x(x);
  pixel.set_y(y);
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Pixel);
  position.set_value(pixel);
  return position;
}

dailyboy::TextPosition percent_position(double x, double y) {
  dailyboy::TextPositionModePercent percent;
  percent.set_x(x);
  percent.set_y(y);
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Percent);
  position.set_value(percent);
  return position;
}

dailyboy::JobLayoutBurnInBox fill_box(double r, double g, double b,
                                      double opacity, int margin) {
  dailyboy::JobLayoutBurnInBox box;
  box.set_mode(dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill);
  box.set_color(dailyboy::RGBColor(r, g, b));
  box.set_opacity(opacity);
  dailyboy::Margin pad;
  pad.set_top(margin);
  pad.set_right(margin);
  pad.set_bottom(margin);
  pad.set_left(margin);
  box.set_margin(std::move(pad));
  return box;
}

dailyboy::JobLayoutBurnInBox outline_box(double r, double g, double b,
                                         double opacity, int margin) {
  dailyboy::JobLayoutBurnInBox box = fill_box(r, g, b, opacity, margin);
  box.set_mode(
      dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline);
  return box;
}

dailyboy::Job make_job(std::vector<dailyboy::JobLayoutBurnIn> burn_ins) {
  dailyboy::Job job;
  dailyboy::JobLayoutBurnIns list;
  list.set_burn_ins(std::move(burn_ins));
  dailyboy::JobLayout layout;
  layout.set_burn_ins(std::move(list));
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::OverlayTokenContext make_tokens() {
  dailyboy::OverlayTokenContext tokens;
  tokens.frame = 1001;
  tokens.frame_start = 1001;
  tokens.frame_end = 1002;
  tokens.source_file = "plate.1001.exr";
  tokens.plan_id = "beauty";
  return tokens;
}

dailyboy::Frame green_canvas(int width = 64, int height = 64) {
  const float fill[3] = {0.0f, 1.0f, 0.0f};
  return dailyboy::Frame(make_rgb(width, height, fill));
}

dailyboy::ColorPipeline must_prepare(const dailyboy::Job& job) {
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  EXPECT_TRUE(prepared.ok()) << prepared.status().message();
  return std::move(prepared).value();
}

}  // namespace

/*!
 * \brief Leaves the canvas unchanged when \c burn_ins is empty.
 */
TEST(BurnIns, DrawHud_EmptyList_IsNoOp) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 0, 0, 0.0f, 1.0f, 0.0f);
  expect_rgb(frame.buf(), 32, 32, 0.0f, 1.0f, 0.0f);
  expect_rgb(frame.buf(), 63, 63, 0.0f, 1.0f, 0.0f);
}

/*!
 * \brief Places a fill box at the bottom-left inset, not at the canvas center.
 */
TEST(BurnIns, DrawHud_BottomLeftAnchor_TouchesCornerNotCenter) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_burn_in(
      "H",
      layout_position(dailyboy::TextPositionModeLayout::Anchor::BottomLeft),
      dailyboy::RGBColor(1.0, 1.0, 1.0), fill_box(1.0, 0.0, 0.0, 1.0, 4))});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 10, 63 - 10, 1.0f, 0.0f, 0.0f);
  EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 32, 32)));
}

/*!
 * \brief Centers a fill box on each canvas edge for the four mid-side anchors.
 */
TEST(BurnIns, DrawHud_MidSideAnchors_SitOnEdgeCenters) {
  // Prepare

  // Test
  auto check = [](dailyboy::TextPositionModeLayout::Anchor anchor, int x,
                  int y) {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_burn_in(
        "H", layout_position(anchor), dailyboy::RGBColor(1.0, 1.0, 1.0),
        fill_box(1.0, 0.0, 0.0, 1.0, 4))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    expect_rgb(frame.buf(), x, y, 1.0f, 0.0f, 0.0f);
  };
  check(dailyboy::TextPositionModeLayout::Anchor::TopCenter, 32, 10);
  check(dailyboy::TextPositionModeLayout::Anchor::BottomCenter, 32, 63 - 10);
  check(dailyboy::TextPositionModeLayout::Anchor::CenterLeft, 10, 32);
  check(dailyboy::TextPositionModeLayout::Anchor::CenterRight, 63 - 10, 32);

  // Assert
}

/*!
 * \brief Places a fill box over the canvas center with \c center_center.
 */
TEST(BurnIns, DrawHud_CenterCenter_CoversCanvasCenter) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_burn_in(
      "H",
      layout_position(dailyboy::TextPositionModeLayout::Anchor::CenterCenter),
      dailyboy::RGBColor(1.0, 1.0, 1.0), fill_box(1.0, 0.0, 0.0, 1.0, 8))});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 32, 32, 1.0f, 0.0f, 0.0f);
}

/*!
 * \brief Honors pixel and percent top-left placement of the full box.
 */
TEST(BurnIns, DrawHud_PixelAndPercent_PlaceBoxTopLeft) {
  // Prepare

  // Test
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_burn_in(
        "H", pixel_position(20, 12), dailyboy::RGBColor(1.0, 1.0, 1.0),
        fill_box(1.0, 0.0, 0.0, 1.0, 2))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    expect_rgb(frame.buf(), 20, 12, 1.0f, 0.0f, 0.0f);
    EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 0, 0)));
  }
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_burn_in(
        "H", percent_position(25.0, 50.0), dailyboy::RGBColor(1.0, 1.0, 1.0),
        fill_box(1.0, 0.0, 0.0, 1.0, 2))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    expect_rgb(frame.buf(), 16, 32, 1.0f, 0.0f, 0.0f);
  }

  // Assert
}

/*!
 * \brief Skips the rectangle when \c box is omitted; still draws glyphs.
 */
TEST(BurnIns, DrawHud_OmittedBox_DrawsTextOnly) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job =
      make_job({make_burn_in("H", pixel_position(8, 8),
                             dailyboy::RGBColor(1.0, 1.0, 1.0), std::nullopt)});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 0, 0)));
  bool found_glyph = false;
  for (int y = 0; y < frame.height() && !found_glyph; ++y) {
    for (int x = 0; x < frame.width() && !found_glyph; ++x) {
      const std::array<float, 3> px = pixel_rgb(frame.buf(), x, y);
      if (!is_background(px)) {
        found_glyph = true;
      }
    }
  }
  EXPECT_TRUE(found_glyph);
}

/*!
 * \brief Skips the rectangle at opacity 0 and still draws the glyphs.
 */
TEST(BurnIns, DrawHud_ZeroOpacityBox_DrawsTextOnly) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_burn_in("H", pixel_position(8, 8),
                                             dailyboy::RGBColor(1.0, 1.0, 1.0),
                                             fill_box(1.0, 0.0, 0.0, 0.0, 4))});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 8, 8)));
  bool found_glyph = false;
  for (int y = 0; y < frame.height() && !found_glyph; ++y) {
    for (int x = 0; x < frame.width() && !found_glyph; ++x) {
      const std::array<float, 3> px = pixel_rgb(frame.buf(), x, y);
      if (!is_background(px)) {
        found_glyph = true;
      }
    }
  }
  EXPECT_TRUE(found_glyph);
}

/*!
 * \brief Paints default white glyphs and a custom font color.
 */
TEST(BurnIns, DrawHud_FontColor_DefaultWhiteAndCustom) {
  // Prepare

  // Test
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::TextFont font;
    EXPECT_DOUBLE_EQ(font.color().r(), 1.0);
    EXPECT_DOUBLE_EQ(font.color().g(), 1.0);
    EXPECT_DOUBLE_EQ(font.color().b(), 1.0);
    dailyboy::Job job = make_job(
        {make_burn_in("H", pixel_position(8, 8), font.color(), std::nullopt)});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
  }
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job(
        {make_burn_in("H", pixel_position(8, 8),
                      dailyboy::RGBColor(0.0, 0.0, 1.0), std::nullopt)});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    bool found_blue = false;
    for (int y = 0; y < frame.height() && !found_blue; ++y) {
      for (int x = 0; x < frame.width() && !found_blue; ++x) {
        const std::array<float, 3> px = pixel_rgb(frame.buf(), x, y);
        if (px[2] > 0.5f && px[0] < 0.25f) {
          found_blue = true;
        }
      }
    }
    EXPECT_TRUE(found_blue);
  }

  // Assert
}

/*!
 * \brief Lerps a semi-transparent fill onto the box ROI and leaves far pixels.
 */
TEST(BurnIns, DrawHud_FillOpacity_BlendsRoiOnly) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_burn_in(
      "H", pixel_position(20, 12), dailyboy::RGBColor(1.0, 1.0, 1.0),
      fill_box(1.0, 0.0, 0.0, 0.45, 2))});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 20, 12, 0.45f, 0.55f, 0.0f);
  expect_rgb(frame.buf(), 0, 0, 0.0f, 1.0f, 0.0f);
}

/*!
 * \brief Paints outline edges without filling the box interior (margin ring).
 */
TEST(BurnIns, DrawHud_OutlineBox_TouchesBorderNotInterior) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_burn_in(
      "H", pixel_position(10, 10), dailyboy::RGBColor(1.0, 1.0, 1.0),
      outline_box(1.0, 0.0, 0.0, 1.0, 8))});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 10, 10, 1.0f, 0.0f, 0.0f);
  EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 14, 14)));
}

/*!
 * \brief Returns a user error when the font file is missing.
 */
TEST(BurnIns, DrawHud_MissingFont_ReturnsOverlayUserError3) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::JobLayoutBurnIn burn_in =
      make_burn_in("H", pixel_position(0, 0), dailyboy::RGBColor(1.0, 1.0, 1.0),
                   std::nullopt);
  dailyboy::TextFont font;
  font.set_path("/no/such/font.ttf");
  font.set_size_px(16);
  burn_in.set_font(std::move(font));
  dailyboy::Job job = make_job({std::move(burn_in)});

  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status = dailyboy::draw_hud(frame, job, make_tokens());

  // Assert
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser);
  EXPECT_NE(status.message().find(std::string(dailyboy::USER_ERROR_OVERLAY_3)),
            std::string::npos);
}

/*!
 * \brief Loads a plate frame and leaves pixels unchanged (identity color).
 */
TEST(BurnIns, LoadPlate_FirstPlateFrame_LoadsRgb) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "load_plate_plate";
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  dailyboy::JobSequence sequence;
  sequence.set_path(dailyboy::test::plate_pattern_path(dir).string());
  sequence.set_frame_start(dailyboy::test::kPlateFrameStart);
  sequence.set_frame_end(dailyboy::test::kPlateFrameEnd);
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(sequence);
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  const dailyboy::Sequence::Iterator it = seq.value().begin();

  // Test
  dailyboy::StatusOr<dailyboy::Frame> frame = dailyboy::load_plate(it);

  // Assert
  ASSERT_TRUE(frame.ok()) << frame.status().message();
  EXPECT_EQ(frame.value().width(), dailyboy::test::kPlateWidth);
  EXPECT_EQ(frame.value().height(), dailyboy::test::kPlateHeight);
  expect_rgb(frame.value().buf(), 0, 0, 1.0f, 0.0f, 0.0f);
}

/*!
 * \brief Rejects a job that has no source plan.
 */
TEST(BurnIns, WriteBurnins_EmptyPlans_ReturnsRenderUserError1) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "burnins_no_plans";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job;
  dailyboy::JobOutputImageSequence item;
  item.set_id("archive");
  item.set_enabled(true);
  item.set_display_view(passthrough_display_view());
  item.set_path_pattern((dir / "out.%04d.png").string());
  job.output().image_sequences().image_sequences().push_back(std::move(item));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());

  // Test
  const dailyboy::Status status =
      dailyboy::write_burnins(job, out, color_pipeline);
  ASSERT_TRUE(out.close().ok());

  // Assert
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.message(), std::string(dailyboy::USER_ERROR_RENDER_1));
}
