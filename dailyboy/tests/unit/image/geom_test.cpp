#include "image/geom.hpp"

#include <gtest/gtest.h>

/*!
 * \brief Default-constructed geometry is zeros.
 */
TEST(Geom, SizePointRect_Default_AreZero) {
  // Prepare
  dailyboy::Size size;
  dailyboy::Point point;
  dailyboy::Rect rect;

  // Test
  // Assert
  EXPECT_EQ(size.width, 0);
  EXPECT_EQ(size.height, 0);
  EXPECT_EQ(point.x, 0);
  EXPECT_EQ(point.y, 0);
  EXPECT_EQ(rect.width, 0);
  EXPECT_EQ(rect.height, 0);
}

/*!
 * \brief Aggregate initialization stores the given values.
 */
TEST(Geom, Rect_AggregateInit_KeepsMembers) {
  // Prepare
  const dailyboy::Rect rect{2, 4, 8, 16};

  // Test
  // Assert
  EXPECT_EQ(rect.x, 2);
  EXPECT_EQ(rect.y, 4);
  EXPECT_EQ(rect.width, 8);
  EXPECT_EQ(rect.height, 16);
}
