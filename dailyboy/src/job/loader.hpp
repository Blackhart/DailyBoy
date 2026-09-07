#pragma once

#include <filesystem>

#include "job/job.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Validates a job YAML file against the JSON Schema contract.
 *
 * Loads the file, converts the document to JSON, validates with
 * \c dailyboy/schemas/job.schema.json, then runs DailyBoy-specific rules
 * (substitution token usage, file sequence patterns, unique output ids,
 * at least one enabled video or image sequence).
 *
 * \param job_path Path to the job YAML file.
 * \return \c Status::Internal if \a job_path is empty or the schema file is
 *         missing; \c Status::User on I/O, YAML parse, schema, or contract
 *         validation errors.
 */
Status validate_job_schema(const std::filesystem::path& job_path);

/*!
 * \brief Parses a job YAML file into in-memory \c Job structures.
 *
 * Does not run JSON Schema validation; call validate_job_schema() first when
 * a validated job is required.
 *
 * \param job_path Path to the job YAML file.
 * \return Parsed job (tokens in strings are not expanded), or a user/internal
 *         status on I/O or parse errors (via \c YmlLoader).
 */
StatusOr<Job> load_job(const std::filesystem::path& job_path);

}  // namespace dailyboy
