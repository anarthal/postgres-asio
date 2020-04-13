#ifndef INCLUDE_PSQL_METADATA_H_
#define INCLUDE_PSQL_METADATA_H_

#include "psql/messages.h"
#include "psql/types.h"

namespace psql
{

class field_metadata
{
	single_row_description msg_;
public:
	field_metadata() = default;
	field_metadata(const single_row_description& msg) noexcept: msg_(msg) {};

	std::string_view field_name() const noexcept { return msg_.name.value; }
	std::int32_t type_oid() const noexcept { return msg_.type_oid; }
	std::int16_t format() const noexcept { return msg_.format; }
};


class resultset_metadata
{
	bytestring buffer_;
	std::vector<field_metadata> fields_;
public:
	resultset_metadata() = default;
	resultset_metadata(bytestring&& buffer, std::vector<field_metadata>&& fields):
		buffer_(std::move(buffer)), fields_(std::move(fields)) {};
	resultset_metadata(const resultset_metadata&) = delete;
	resultset_metadata(resultset_metadata&&) = default;
	resultset_metadata& operator=(const resultset_metadata&) = delete;
	resultset_metadata& operator=(resultset_metadata&&) = default;
	~resultset_metadata() = default;
	const auto& fields() const noexcept { return fields_; }
};

resultset_metadata make_resultset_metadata(
	const row_description& msg,
	bytestring&& buffer
)
{
	std::vector<field_metadata> m;
	for (const auto& single: msg.rows)
	{
		m.push_back(field_metadata(single));
	}
	return resultset_metadata(std::move(buffer), std::move(m));
}

}

#endif /* INCLUDE_PSQL_METADATA_H_ */
