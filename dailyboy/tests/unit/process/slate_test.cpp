#include "process/slate.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "error/process.hpp"
#include "image/frame.hpp"
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
#include "support/test_fonts.hpp"

namespace {

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}


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

bool has_glyph_near(const OIIO::ImageBuf& buf, int origin_x, int origin_y,
                    int radius) {
  const int x0 = std::max(0, origin_x);
  const int y0 = std::max(0, origin_y);
  const int x1 = std::min(buf.spec().width, origin_x + radius);
  const int y1 = std::min(buf.spec().height, origin_y + radius);
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      if (!is_background(pixel_rgb(buf, x, y))) {
        return true;
      }
    }
  }
  return false;
}

dailyboy::JobLayoutSlateLine make_line(const std::string& text,
                                       dailyboy::TextPosition position,
                                       const dailyboy::RGBColor& font_color) {
  dailyboy::JobLayoutSlateLine line;
  line.set_text(text);
  line.set_position(std::move(position));
  dailyboy::TextFont font;
  font.set_path(dailyboy::test::kDejaVuSans);
  font.set_size_px(16);
  font.set_color(font_color);
  line.set_font(std::move(font));
  return line;
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

dailyboy::Job make_job(std::vector<dailyboy::JobLayoutSlateLine> lines) {
  dailyboy::Job job;
  dailyboy::JobLayoutSlate slate;
  slate.set_lines(std::move(lines));
  dailyboy::JobLayout layout;
  layout.set_slate(std::move(slate));
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::OverlayTokenContext make_tokens() {
  dailyboy::OverlayTokenContext tokens;
  tokens.frame = 1001;
  tokens.frame_start = 1001;
  tokens.frame_end = 1002;
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
 * \brief Leaves the canvas unchanged when \c slate.lines is empty.
 */
TEST(Slate, DrawSlateTexts_EmptyLines_IsNoOp) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({});
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status =
      dailyboy::draw_slate_texts(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_rgb(frame.buf(), 0, 0, 0.0f, 1.0f, 0.0f);
  expect_rgb(frame.buf(), 32, 32, 0.0f, 1.0f, 0.0f);
  expect_rgb(frame.buf(), 63, 63, 0.0f, 1.0f, 0.0f);
}

/*!
 * \brief Places glyphs at the bottom-left inset, not at the canvas center.
 */
TEST(Slate, DrawSlateTexts_BottomLeftAnchor_TouchesCornerNotCenter) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_line(
      "H",
      layout_position(dailyboy::TextPositionModeLayout::Anchor::BottomLeft),
      dailyboy::RGBColor(1.0, 1.0, 1.0))});
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status =
      dailyboy::draw_slate_texts(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(has_glyph_near(frame.buf(), 10, 40, 24));
  EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 32, 32)));
}

/*!
 * \brief Centers glyphs on each canvas edge for the four mid-side anchors.
 */
TEST(Slate, DrawSlateTexts_MidSideAnchors_SitOnEdgeCenters) {
  // Prepare

  // Test
  auto check = [](dailyboy::TextPositionModeLayout::Anchor anchor, int x,
                  int y) {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_line(
        "H", layout_position(anchor), dailyboy::RGBColor(1.0, 1.0, 1.0))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status =
        dailyboy::draw_slate_texts(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_TRUE(has_glyph_near(frame.buf(), x, y, 16))
        << "anchor origin (" << x << "," << y << ")";
  };
  check(dailyboy::TextPositionModeLayout::Anchor::TopCenter, 24, 10);
  check(dailyboy::TextPositionModeLayout::Anchor::BottomCenter, 24, 40);
  check(dailyboy::TextPositionModeLayout::Anchor::CenterLeft, 10, 24);
  check(dailyboy::TextPositionModeLayout::Anchor::CenterRight, 40, 24);

  // Assert
}

/*!
 * \brief Places glyphs over the canvas center with \c center_center.
 */
TEST(Slate, DrawSlateTexts_CenterCenter_CoversCanvasCenter) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::Job job = make_job({make_line(
      "H",
      layout_position(dailyboy::TextPositionModeLayout::Anchor::CenterCenter),
      dailyboy::RGBColor(1.0, 1.0, 1.0))});
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status =
      dailyboy::draw_slate_texts(frame, job, make_tokens());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(has_glyph_near(frame.buf(), 24, 24, 20));
}

/*!
 * \brief Honors pixel and percent top-left placement of the glyph box.
 */
TEST(Slate, DrawSlateTexts_PixelAndPercent_PlaceGlyphTopLeft) {
  // Prepare

  // Test
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_line(
        "H", pixel_position(20, 12), dailyboy::RGBColor(1.0, 1.0, 1.0))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status =
        dailyboy::draw_slate_texts(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_TRUE(has_glyph_near(frame.buf(), 20, 12, 16));
    EXPECT_TRUE(is_background(pixel_rgb(frame.buf(), 0, 0)));
  }
  {
    dailyboy::Frame frame = green_canvas();
    dailyboy::Job job = make_job({make_line(
        "H", percent_position(25.0, 50.0), dailyboy::RGBColor(1.0, 1.0, 1.0))});
    dailyboy::ColorPipeline color_pipeline = must_prepare(job);
    dailyboy::Status status =
        dailyboy::draw_slate_texts(frame, job, make_tokens());
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_TRUE(has_glyph_near(frame.buf(), 16, 32, 16));
  }

  // Assert
}

/*!
 * \brief Expands \c {frame} like a literal and skips an empty \c {source_file}.
 */
TEST(Slate, DrawSlateTexts_Tokens_ExpandFrameAndEmptySourceFile) {
  // Prepare
  dailyboy::OverlayTokenContext tokens = make_tokens();
  dailyboy::Frame literal = green_canvas();
  dailyboy::Job literal_job = make_job({make_line(
      "1001", pixel_position(8, 8), dailyboy::RGBColor(1.0, 1.0, 1.0))});
  dailyboy::Frame token_frame = green_canvas();
  dailyboy::Job token_job = make_job({make_line(
      "{frame}", pixel_position(8, 8), dailyboy::RGBColor(1.0, 1.0, 1.0))});
  dailyboy::Frame empty_source = green_canvas();
  dailyboy::Job empty_job =
      make_job({make_line("{source_file}", pixel_position(8, 8),
                          dailyboy::RGBColor(1.0, 1.0, 1.0))});
  dailyboy::ColorPipeline color_pipeline = must_prepare(literal_job);

  // Test
  ASSERT_TRUE(dailyboy::draw_slate_texts(literal, literal_job, tokens).ok());
  const dailyboy::Status token_status =
      dailyboy::draw_slate_texts(token_frame, token_job, tokens);
  const dailyboy::Status empty_status =
      dailyboy::draw_slate_texts(empty_source, empty_job, tokens);

  // Assert
  ASSERT_TRUE(token_status.ok()) << token_status.message();
  ASSERT_TRUE(empty_status.ok()) << empty_status.message();
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      const std::array<float, 3> a = pixel_rgb(literal.buf(), x, y);
      const std::array<float, 3> b = pixel_rgb(token_frame.buf(), x, y);
      EXPECT_NEAR(a[0], b[0], 1e-5) << "at (" << x << "," << y << ")";
      EXPECT_NEAR(a[1], b[1], 1e-5) << "at (" << x << "," << y << ")";
      EXPECT_NEAR(a[2], b[2], 1e-5) << "at (" << x << "," << y << ")";
    }
  }
  expect_rgb(empty_source.buf(), 8, 8, 0.0f, 1.0f, 0.0f);
  expect_rgb(empty_source.buf(), 32, 32, 0.0f, 1.0f, 0.0f);
}

/*!
 * \brief Returns a user error when the font file is missing.
 */
TEST(Slate, DrawSlateTexts_MissingFont_ReturnsOverlayUserError3) {
  // Prepare
  dailyboy::Frame frame = green_canvas();
  dailyboy::JobLayoutSlateLine line =
      make_line("H", pixel_position(0, 0), dailyboy::RGBColor(1.0, 1.0, 1.0));
  dailyboy::TextFont font;
  font.set_path("/no/such/font.ttf");
  font.set_size_px(16);
  line.set_font(std::move(font));
  dailyboy::Job job = make_job({std::move(line)});
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::Status status =
      dailyboy::draw_slate_texts(frame, job, make_tokens());

  // Assert
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser);
  EXPECT_NE(status.message().find(std::string(dailyboy::USER_ERROR_OVERLAY_3)),
            std::string::npos);
}

/*!
 * \brief Allocates and fills an RGB canvas with \c layout.background.
 */
TEST(Slate, MakeFilledCanvas_BackgroundColor_FillsCanvas) {
  // Prepare
  dailyboy::Job job;
  dailyboy::JobLayout layout;
  dailyboy::JobLayoutCanvas canvas;
  canvas.set_width(8);
  canvas.set_height(8);
  layout.set_canvas(std::move(canvas));
  layout.set_background(dailyboy::RGBColor(0.2, 0.4, 0.6));
  job.set_layout(std::move(layout));

  // Test
  dailyboy::StatusOr<dailyboy::Frame> frame = dailyboy::make_filled_canvas(job);

  // Assert
  ASSERT_TRUE(frame.ok()) << frame.status().message();
  EXPECT_EQ(frame.value().width(), 8);
  EXPECT_EQ(frame.value().height(), 8);
  expect_rgb(frame.value().buf(), 0, 0, 0.2f, 0.4f, 0.6f);
  expect_rgb(frame.value().buf(), 7, 7, 0.2f, 0.4f, 0.6f);
}

/*!
 * \brief A slate duration of 0 writes no frames.
 */
TEST(Slate, WriteSlates_DurationZero_IsNoOp) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "slate_duration_zero";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job;
  dailyboy::JobPlan plan;
  plan.set_id("plate");
  dailyboy::JobSequence sequence;
  sequence.set_path("/unused.%04d.exr");
  sequence.set_frame_start(dailyboy::test::kPlateFrameStart);
  sequence.set_frame_end(dailyboy::test::kPlateFrameEnd);
  plan.set_sequence(std::move(sequence));
  job.plans().plans().push_back(std::move(plan));
  dailyboy::JobOutputImageSequence item;
  item.set_id("archive");
  item.set_enabled(true);
  item.set_display_view(passthrough_display_view());
  item.set_path_pattern((dir / "out.%04d.png").string());
  job.output().image_sequences().image_sequences().push_back(std::move(item));
  job.layout().slate().set_duration_frames(0);
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());

  // Test
  const dailyboy::Status status = dailyboy::write_slates(job, out);
  ASSERT_TRUE(out.close().ok());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(std::filesystem::exists(dir / "out.1001.png"));
}
