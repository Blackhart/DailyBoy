#include "process/tokens.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

#include "job/metadata.hpp"

namespace {

dailyboy::OverlayTokenContext make_context() {
  dailyboy::OverlayTokenContext context;
  context.frame = 1005;
  context.frame_start = 1001;
  context.frame_end = 1048;
  context.source_file = "/tmp/plate.1005.exr";
  context.plan_id = "beauty";
  return context;
}

}  // namespace

/*!
 * \brief Replaces a string substitution and leaves surrounding text.
 */
TEST(OverlayTokens, ExpandOverlayTokens_StringSubstitution_InsertsValue) {
  // Prepare
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;
  subs["shot"] = std::string("sh010");

  // Test
  const std::string out =
      dailyboy::expand_overlay_tokens("SHOT {shot} END", make_context(), subs);

  // Assert
  EXPECT_EQ(out, "SHOT sh010 END");
}

/*!
 * \brief Resolves an exact frame-map key and a covering range; miss is empty.
 */
TEST(OverlayTokens, ExpandOverlayTokens_FrameMap_HitsMissAndRange) {
  // Prepare
  std::map<std::string, std::string> notes;
  notes["1001"] = "first";
  notes["1005-1010"] = "action";
  notes["1048"] = "last";
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;
  subs["note"] = notes;
  dailyboy::OverlayTokenContext context = make_context();

  // Test
  // Assert
  EXPECT_EQ(dailyboy::expand_overlay_tokens("{note}", context, subs), "action");
  context.frame = 1001;
  EXPECT_EQ(dailyboy::expand_overlay_tokens("{note}", context, subs), "first");
  context.frame = 1011;
  EXPECT_EQ(dailyboy::expand_overlay_tokens("{note}", context, subs), "");
}

/*!
 * \brief Prefers a builtin over a substitution of the same name.
 */
TEST(OverlayTokens, ExpandOverlayTokens_BuiltinNameClash_BuiltinWins) {
  // Prepare
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;
  subs["frame"] = std::string("nope");
  subs["plan_id"] = std::string("other");

  // Test
  const std::string out = dailyboy::expand_overlay_tokens(
      "{frame} {plan_id} {frame_start}-{frame_end} {source_file}",
      make_context(), subs);

  // Assert
  EXPECT_EQ(out, "1005 beauty 1001-1048 /tmp/plate.1005.exr");
}

/*!
 * \brief Leaves an unknown token intact.
 */
TEST(OverlayTokens, ExpandOverlayTokens_UnknownKey_LeavesLiteral) {
  // Prepare
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;

  // Test
  const std::string out =
      dailyboy::expand_overlay_tokens("see {unknown}", make_context(), subs);

  // Assert
  EXPECT_EQ(out, "see {unknown}");
}
