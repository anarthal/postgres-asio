#ifndef INCLUDE_PSQL_DESERIALIZE_ROW_H_
#define INCLUDE_PSQL_DESERIALIZE_ROW_H_

#include "psql/error.h"
#include "psql/value.h"
#include "psql/messages.h"
#include "psql/metadata.h"
#include <vector>

constexpr std::int32_t int2_oid = 21;
constexpr std::int32_t int4_oid = 23;
constexpr std::int32_t varchar_oid = 1043;

inline void check_error_code(errc err)
{
	if (err != errc::ok)
		throw boost::system::system_error(make_error_code(err));
}

inline value deserialize_single(
	std::string_view from,
	const field_metadata& meta
)
{
	if (meta.format() == 0) // text
	{
		if (meta.type_oid() == int2_oid || meta.type_oid() == int4_oid)
		{
			return value(std::stoi(std::string(from)));
		}
		else if (meta.type_oid() == varchar_oid)
		{
			return value(from);
		}
		else
		{
			throw std::runtime_error("Unknown type OID");
		}
	}
	else // binary
	{
		throw std::runtime_error("Unsupported binary format");
	}
}

inline std::vector<value> deserialize_row(
	const std::vector<field_metadata>& meta,
	const bytestring& buffer
)
{
	// Context
	deserialization_context ctx (buffer.data(), buffer.data() + buffer.size());

	// Field count
	std::int16_t field_count = 0;
	auto err = deserialize(field_count, ctx);
	check_error_code(err);
	assert(field_count == meta.size());

	// Each field
	std::vector<value> res;
	res.reserve(field_count);
	for (int i = 0; i < field_count; ++i)
	{
		std::int32_t size = 0;
		err = deserialize(size, ctx);
		check_error_code(err);
		if (size == -1)
		{
			// NULL
			res.push_back(value(nullptr));
		}
		else
		{
			if (!ctx.enough_size(size))
				throw boost::system::system_error(make_error_code(errc::incomplete_message));
			std::string_view buff = get_string(ctx.first(), size);
			res.push_back(deserialize_single(buff, meta[i]));
			ctx.advance(size);
		}
	}
	if (!ctx.empty())
		throw boost::system::system_error(make_error_code(errc::extra_bytes));
	return res;
}


#endif /* INCLUDE_PSQL_DESERIALIZE_ROW_H_ */
