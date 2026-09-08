#pragma once

namespace dailyboy {

/*!
 * \brief Four-sided margin in pixels (top, right, bottom, left).
 *
 * YAML may supply a single scalar (applied to all sides) or a per-side map.
 */
class Margin {
 public:
  Margin() = default;
  int top() const { return top_; }
  void set_top(int top) { top_ = top; }

  int bottom() const { return bottom_; }
  void set_bottom(int bottom) { bottom_ = bottom; }

  int left() const { return left_; }
  void set_left(int left) { left_ = left; }

  int right() const { return right_; }
  void set_right(int right) { right_ = right; }

 private:
  int top_ = 0;
  int bottom_ = 0;
  int left_ = 0;
  int right_ = 0;
};

/*!
 * \brief Linear RGB color components in the 0–1 range.
 */
class RGBColor {
 public:
  RGBColor() = default;
  /*! \brief Constructs a color from \a r, \a g, \a b in the 0–1 range. */
  RGBColor(double r, double g, double b) : r_(r), g_(g), b_(b) {}
  double r() const { return r_; }
  void set_r(double r) { r_ = r; }

  double g() const { return g_; }
  void set_g(double g) { g_ = g; }

  double b() const { return b_; }
  void set_b(double b) { b_ = b; }

 private:
  double r_ = 0.0;
  double g_ = 0.0;
  double b_ = 0.0;
};

}  // namespace dailyboy
