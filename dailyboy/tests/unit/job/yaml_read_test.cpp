#include "job/yaml_read.hpp"

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <string>

#include "error/job.hpp"

/*!
 * \brief A mapping node is accepted as a map.
 */
TEST(YamlRead, ExpectMap_MappingNode_ReturnsNode) {
  // Prepare
  const YAML::Node node = YAML::Load("{a: 1}");

  // Test
  dailyboy::StatusOr<YAML::Node> map = dailyboy::expect_map(node, "root");

  // Assert
  ASSERT_TRUE(map.ok()) << map.status().message();
  EXPECT_TRUE(map.value().IsMap());
}

/*!
 * \brief A sequence node is rejected as a map.
 */
TEST(YamlRead, ExpectMap_SequenceNode_ReturnsJobUserError10) {
  // Prepare
  const YAML::Node node = YAML::Load("[1, 2]");

  // Test
  dailyboy::StatusOr<YAML::Node> map = dailyboy::expect_map(node, "root");

  // Assert
  ASSERT_FALSE(map.ok());
  EXPECT_NE(
      map.status().message().find(std::string(dailyboy::USER_ERROR_JOB_10)),
      std::string::npos);
}

/*!
 * \brief Quoted non-empty scalar is returned as a string.
 */
TEST(YamlRead, ReadQuotedNonemptyString_QuotedScalar_ReturnsText) {
  // Prepare
  const YAML::Node node = YAML::Load("\"hello\"");

  // Test
  dailyboy::StatusOr<std::string> text = dailyboy::read_quoted_nonempty_string(
      node, dailyboy::USER_ERROR_JOB_48, "color.ocio_config");

  // Assert
  ASSERT_TRUE(text.ok()) << text.status().message();
  EXPECT_EQ(text.value(), "hello");
}

/*!
 * \brief yaml_to_json converts a small map.
 */
TEST(YamlRead, YamlToJson_SmallMap_ReturnsObject) {
  // Prepare
  const YAML::Node node = YAML::Load("{n: 2}");

  // Test
  const nlohmann::json json = dailyboy::yaml_to_json(node);

  // Assert
  EXPECT_EQ(json["n"], 2);
}

/*!
 * \brief A sequence node is accepted as a sequence.
 */
TEST(YamlRead, ExpectSequence_SequenceNode_ReturnsNode) {
  // Prepare
  const YAML::Node node = YAML::Load("[1, 2]");

  // Test
  dailyboy::StatusOr<YAML::Node> seq = dailyboy::expect_sequence(node, "plans");

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  EXPECT_TRUE(seq.value().IsSequence());
  EXPECT_EQ(seq.value().size(), 2u);
}

/*!
 * \brief A map node is rejected as a sequence.
 */
TEST(YamlRead, ExpectSequence_MapNode_ReturnsJobUserError10) {
  // Prepare
  const YAML::Node node = YAML::Load("{a: 1}");

  // Test
  dailyboy::StatusOr<YAML::Node> seq = dailyboy::expect_sequence(node, "plans");

  // Assert
  ASSERT_FALSE(seq.ok());
  EXPECT_NE(
      seq.status().message().find(std::string(dailyboy::USER_ERROR_JOB_10)),
      std::string::npos);
}

/*!
 * \brief Absent key in a map is treated as an empty sequence.
 */
TEST(YamlRead, OptionalSequence_NullNode_ReturnsEmptySequence) {
  // Prepare
  const YAML::Node node;

  // Test
  dailyboy::StatusOr<YAML::Node> seq =
      dailyboy::optional_sequence(node, "output.videos");

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  EXPECT_TRUE(seq.value().IsSequence());
  EXPECT_EQ(seq.value().size(), 0u);
}

/*!
 * \brief Reads a required string field from a map.
 */
TEST(YamlRead, AsRequired_PresentString_ReturnsValue) {
  // Prepare
  const YAML::Node node = YAML::Load("{id: plate}");

  // Test
  dailyboy::StatusOr<std::string> id =
      dailyboy::as_required<std::string>(node, "id", "plans[0]");

  // Assert
  ASSERT_TRUE(id.ok()) << id.status().message();
  EXPECT_EQ(id.value(), "plate");
}

/*!
 * \brief Missing required key is a user error.
 */
TEST(YamlRead, AsRequired_MissingKey_ReturnsJobUserError11) {
  // Prepare
  const YAML::Node node = YAML::Load("{id: plate}");

  // Test
  dailyboy::StatusOr<std::string> path =
      dailyboy::as_required<std::string>(node, "path", "plans[0]");

  // Assert
  ASSERT_FALSE(path.ok());
  EXPECT_NE(
      path.status().message().find(std::string(dailyboy::USER_ERROR_JOB_11)),
      std::string::npos);
}

/*!
 * \brief Quoted YAML scalar is recognized as a quoted string.
 */
TEST(YamlRead, IsYamlQuotedString_DoubleQuoted_ReturnsTrue) {
  // Prepare
  const YAML::Node node = YAML::Load("\"hello\"");

  // Test
  const bool quoted = dailyboy::is_yaml_quoted_string(node);

  // Assert
  EXPECT_TRUE(quoted);
}

/*!
 * \brief Parses a YAML boolean scalar written as text.
 */
TEST(YamlRead, ParseBoolScalar_TrueLiteral_SetsTrue) {
  // Prepare
  bool value = false;

  // Test
  const bool parsed = dailyboy::parse_bool_scalar("true", value);

  // Assert
  EXPECT_TRUE(parsed);
  EXPECT_TRUE(value);
}

/*!
 * \brief Missing optional key returns the fallback.
 */
TEST(YamlRead, AsOptional_MissingKey_ReturnsFallback) {
  // Prepare
  const YAML::Node node = YAML::Load("{id: plate}");

  // Test
  dailyboy::StatusOr<int> fps =
      dailyboy::as_optional<int>(node, "fps", 24, "output.videos[0]");

  // Assert
  ASSERT_TRUE(fps.ok()) << fps.status().message();
  EXPECT_EQ(fps.value(), 24);
}

/*!
 * \brief Reads an unquoted integer scalar.
 */
TEST(YamlRead, ReadYamlInteger_UnquotedInt_ReturnsValue) {
  // Prepare
  const YAML::Node node = YAML::Load("1001");

  // Test
  dailyboy::StatusOr<int> value = dailyboy::read_yaml_integer(
      node, dailyboy::USER_ERROR_JOB_56, "layout.canvas.width");

  // Assert
  ASSERT_TRUE(value.ok()) << value.status().message();
  EXPECT_EQ(value.value(), 1001);
}
