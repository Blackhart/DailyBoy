#pragma once

#include <optional>
#include <string>
#include <utility>

namespace dailyboy {

/*!
 * \brief Error result without C++ exceptions.
 *
 * Default-constructed Status is OK. Factories \c User and \c Internal build
 * failures; \c message() is the text to log or show.
 */
class Status {
 public:
  /*!
   * \brief Success (\c kOk), caller-fixable input (\c kUser), or engine bug
   *        (\c kInternal).
   */
  enum class Code {
    kOk = 0,
    kUser = 1,
    kInternal = 2,
  };

  Status() = default;

  /*! \brief Returns an OK status. */
  static Status Ok() { return Status(); }

  /*!
   * \brief Returns a caller-fixable error.
   * \param message Text for \c message() (typically a USER_ERROR_* constant).
   */
  static Status User(std::string message) {
    return Status(Code::kUser, std::move(message));
  }

  /*!
   * \brief Returns an internal/programming error.
   * \param message Text for \c message() (typically an INTERNAL_ERROR_* constant).
   */
  static Status Internal(std::string message) {
    return Status(Code::kInternal, std::move(message));
  }

  /*! \brief Returns whether this status is OK. */
  bool ok() const { return code_ == Code::kOk; }
  Code code() const { return code_; }
  const std::string& message() const { return message_; }

 private:
  Status(Code code, std::string message)
      : code_(code), message_(std::move(message)) {}

  Code code_ = Code::kOk;
  std::string message_;
};

/*!
 * \brief A value \c T or an error \c Status.
 *
 * Implicitly convertible from \c T (success) or \c Status (failure). Call
 * \c value() / \c operator* only when \c ok() is true.
 */
template <typename T>
class StatusOr {
 public:
  StatusOr(T value) : value_(std::move(value)) {}          // NOLINT
  StatusOr(Status status) : status_(std::move(status)) {}  // NOLINT

  bool ok() const { return status_.ok(); }
  const Status& status() const { return status_; }

  /*!
   * \brief Returns the stored value.
   * \note Requires \c ok(); otherwise the behavior is undefined.
   */
  const T& value() const& { return *value_; }
  T& value() & { return *value_; }
  T value() && { return std::move(*value_); }

  const T& operator*() const { return value(); }
  T& operator*() { return value(); }

 private:
  Status status_;
  std::optional<T> value_;
};

}  // namespace dailyboy

#define DAILYBOY_STATUS_CONCAT_INNER(a, b) a##b
#define DAILYBOY_STATUS_CONCAT(a, b) DAILYBOY_STATUS_CONCAT_INNER(a, b)

/*!
 * \def DAILYBOY_RETURN_IF_ERROR
 * \brief Returns from the current function if \a expr is a non-OK Status.
 *
 * \a expr must yield a \c Status. The enclosing function must return
 * \c Status or \c StatusOr.
 */
#define DAILYBOY_RETURN_IF_ERROR(expr)                 \
  do {                                                 \
    const ::dailyboy::Status dailyboy_status = (expr); \
    if (!dailyboy_status.ok()) {                       \
      return dailyboy_status;                          \
    }                                                  \
  } while (0)

/*!
 * \def DAILYBOY_ASSIGN_OR_RETURN
 * \brief Assigns a StatusOr value to \a lhs, or returns its Status on error.
 *
 * \a lhs may be a new declaration (\c const YAML::Node root) or an existing
 * object. The enclosing function must return \c Status or \c StatusOr.
 */
#define DAILYBOY_ASSIGN_OR_RETURN(lhs, rexpr)                                  \
  DAILYBOY_ASSIGN_OR_RETURN_IMPL(DAILYBOY_STATUS_CONCAT(status_or_, __LINE__), \
                                 lhs, rexpr)

#define DAILYBOY_ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                                   \
  if (!statusor.ok()) {                                      \
    return statusor.status();                                \
  }                                                          \
  lhs = std::move(statusor).value()
