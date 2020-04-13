#ifndef INCLUDE_PSQL_VALUE_H_
#define INCLUDE_PSQL_VALUE_H_

#include <variant>
#include <string_view>
#include <cstdint>
#include <date/date.h>

namespace psql
{

using date = ::date::sys_days;

/**
 * \ingroup values
 * \brief Type representing MySQL DATETIME and TIMESTAMP data types.
 */
using datetime = ::date::sys_time<std::chrono::microseconds>;

/**
 * \ingroup values
 * \brief Type representing MySQL TIME data type.
 */
using time = std::chrono::microseconds;

/**
 * \ingroup values
 * \brief Represents a value in the database of any of the allowed types.
 * \details If a value is NULL, the type of the variant will be nullptr_t.
 *
 * If a value is a string, the type will be string_view, and it will
 * point to a externally owned piece of memory. That implies that boost::mysql::value
 * does **NOT** own its string memory (saving copies).
 *
 * MySQL types BIT and GEOMETRY do not have direct support yet.
 * They are represented as binary strings.
 */
using value = std::variant<
	std::int32_t,      // signed TINYINT, SMALLINT, MEDIUMINT, INT
	std::int64_t,      // signed BIGINT
	std::uint32_t,     // unsigned TINYINT, SMALLINT, MEDIUMINT, INT, YEAR
	std::uint64_t,     // unsigned BIGINT
	std::string_view,  // CHAR, VARCHAR, BINARY, VARBINARY, TEXT (all sizes), BLOB (all sizes), ENUM, SET, DECIMAL, BIT, GEOMTRY
	float,             // FLOAT
	double,            // DOUBLE
	date,              // DATE
	datetime,          // DATETIME, TIMESTAMP
	time,              // TIME
	std::nullptr_t     // Any of the above when the value is NULL
>;

}

#endif /* INCLUDE_PSQL_VALUE_H_ */
