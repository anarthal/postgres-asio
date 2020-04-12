#ifndef INCLUDE_PSQL_MESSAGES_H_
#define INCLUDE_PSQL_MESSAGES_H_

#include "psql/serialization.h"

// Handshake
struct startup_message
{
	std::int32_t protocol_version;
	string_null param0;
	string_null value0;
	string_null param1;
	string_null value1;
	std::uint8_t terminator = 0;

	static constexpr std::uint8_t message_type = 0;
};

template <>
struct get_struct_fields<startup_message>
{
	static constexpr auto value = std::make_tuple(
		&startup_message::protocol_version,
		&startup_message::param0,
		&startup_message::value0,
		&startup_message::param1,
		&startup_message::value1,
		&startup_message::terminator
	);
};

struct authentication_request
{
	std::int32_t auth_type;
	string_eof auth_data;

	static constexpr std::uint8_t message_type = 0x52;
};

template <>
struct get_struct_fields<authentication_request>
{
	static constexpr auto value = std::make_tuple(
		&authentication_request::auth_type,
		&authentication_request::auth_data
	);
};

struct password_message
{
	string_null password;

	static constexpr std::uint8_t message_type = std::uint8_t('p');
};

template <>
struct get_struct_fields<password_message>
{
	static constexpr auto value = std::make_tuple(
		&password_message::password
	);
};

// Row description
struct single_row_description
{
	string_null name;
	std::int32_t table_oid;
	std::int16_t column_number;
	std::int32_t type_oid;
	std::int16_t size;
	std::int32_t type_modifier;
	std::int16_t format;
};

template <>
struct get_struct_fields<single_row_description>
{
	static constexpr auto value = std::make_tuple(
		&single_row_description::name,
		&single_row_description::table_oid,
		&single_row_description::column_number,
		&single_row_description::type_oid,
		&single_row_description::size,
		&single_row_description::type_modifier,
		&single_row_description::format
	);
};

struct row_description
{
	std::vector<single_row_description> rows;

	static constexpr std::uint8_t message_type = std::uint8_t('T');
};

template <>
struct serialization_traits<row_description, serialization_tag::none> :
	noop_serialize<row_description>
{

	static inline errc deserialize_(row_description& output, deserialization_context& ctx)
	{
		std::int16_t num_fields = 0;
		auto err = deserialize(num_fields, ctx);
		if (err != errc::ok) return err;

		output.rows.resize(num_fields); // noexcept specifiers are not correct but ok for a prototype
		for (int i = 0; i < num_fields; ++i)
		{
			err = deserialize(output.rows[i], ctx);
			if (err != errc::ok) return err;
		}

		return errc::ok;
	}
};


// Query
struct query_message
{
	string_null query;

	static constexpr std::uint8_t message_type = std::uint8_t('Q');
};

template <>
struct get_struct_fields<query_message>
{
	static constexpr auto value = std::make_tuple(
		&query_message::query
	);
};




#endif /* INCLUDE_PSQL_MESSAGES_H_ */
