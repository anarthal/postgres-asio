#ifndef INCLUDE_PSQL_VALUE_H_
#define INCLUDE_PSQL_VALUE_H_

#include <variant>
#include <string_view>
#include <cstdint>

using value = std::variant<
	std::int32_t,      // signed TINYINT, SMALLINT, MEDIUMINT, INT
	std::int64_t,      // signed BIGINT
	std::uint32_t,     // unsigned TINYINT, SMALLINT, MEDIUMINT, INT, YEAR
	std::uint64_t,     // unsigned BIGINT
	std::string_view,  // CHAR, VARCHAR, BINARY, VARBINARY, TEXT (all sizes), BLOB (all sizes), ENUM, SET, DECIMAL, BIT, GEOMTRY
	float,             // FLOAT
	double,            // DOUBLE
	std::nullptr_t     // Any of the above when the value is NULL
>;



#endif /* INCLUDE_PSQL_VALUE_H_ */
