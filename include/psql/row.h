#ifndef INCLUDE_PSQL_ROW_H_
#define INCLUDE_PSQL_ROW_H_

#include "psql/value.h"
#include "psql/types.h"

namespace psql
{

class row
{
	std::vector<value> values_;
public:
	row(std::vector<value>&& values = {}):
		values_(std::move(values)) {};

	const std::vector<value>& values() const noexcept { return values_; }
	std::vector<value>& values() noexcept { return values_; }
};

class owning_row : public row
{
	bytestring buffer_;
public:
	owning_row() = default;
	owning_row(std::vector<value>&& values, bytestring&& buffer) :
			row(std::move(values)), buffer_(std::move(buffer)) {};
	owning_row(const owning_row&) = delete;
	owning_row(owning_row&&) = default;
	owning_row& operator=(const owning_row&) = delete;
	owning_row& operator=(owning_row&&) = default;
	~owning_row() = default;
};

}

#endif /* INCLUDE_PSQL_ROW_H_ */
