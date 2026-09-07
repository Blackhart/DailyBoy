#include "process/path_tokens.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "job/metadata.hpp"

/*!
 * \brief Replaces a string substitution in a path.
 */
TEST(PathTokens, ExpandPathTokens_StringKey_ReplacesToken) {
  // Prepare
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> substitutions;
  substitutions["shot"] = std::string("sh010");

  // Test
  dailyboy::StatusOr<std::string> path = dailyboy::expand_path_tokens(
      "/proj/{shot}/plate.%04d.exr", substitutions, "frame-map");

  // Assert
  ASSERT_TRUE(path.ok()) << path.status().message();
  EXPECT_EQ(path.value(), "/proj/sh010/plate.%04d.exr");
}

/*!
 * \brief A frame-map token in a path is a user error.
 */
TEST(PathTokens, ExpandPathTokens_FrameMapKey_ReturnsUserError) {
  // Prepare
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> substitutions;
  substitutions["frame"] = std::map<std::string, std::string>{{"1001", "a"}};

  // Test
  dailyboy::StatusOr<std::string> path = dailyboy::expand_path_tokens(
      "/proj/{frame}.exr", substitutions, "frame-map not allowed");

  // Assert
  ASSERT_FALSE(path.ok());
  EXPECT_EQ(path.status().message(), "frame-map not allowed");
}
