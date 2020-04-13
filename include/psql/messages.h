#ifndef INCLUDE_PSQL_MESSAGES_H_
#define INCLUDE_PSQL_MESSAGES_H_

#include "psql/serialization.h"

// Generic utils
template <std::uint8_t msg_type>
struct empty_message
{
	static constexpr std::uint8_t message_type = msg_type;
};

template <std::uint8_t msg_type>
struct get_struct_fields<empty_message<msg_type>>
{
	static constexpr auto value = std::make_tuple(); // no fields
};

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

// Extended query
struct parse_message
{
	string_null name;
	string_null statement;
	std::int16_t num_params = 0; // pre-specified parameter types, not supported yet

	static constexpr std::uint8_t message_type = std::uint8_t('P');
};

template <>
struct get_struct_fields<parse_message>
{
	static constexpr auto value = std::make_tuple(
		&parse_message::name,
		&parse_message::statement,
		&parse_message::num_params
	);
};

using parse_complete_message = empty_message<'1'>;

template <typename ForwardIterator>
struct bind_message
{
	string_null portal_name;
	string_null statement_name;
	ForwardIterator params_begin;
	ForwardIterator params_end;
	// std::int16_t num_format_codes = 0; // not supported
	// std::int16_t num_params; && std::int32_t param length (-1 for NULL), string_null param_value
	// std::int16_t num_output_format_codes = 0;
	static constexpr std::uint8_t message_type = std::uint8_t('B');
};

template <typename ForwardIterator>
struct serialization_traits<bind_message<ForwardIterator>, serialization_tag::none>
{
	using msg_type = bind_message<ForwardIterator>;

	static std::size_t get_size_(const msg_type& input, const serialization_context& ctx)
	{
		std::size_t res =
			get_size(input.portal_name, ctx) +
			get_size(input.statement_name, ctx) +
			2 + // num_format_codes
			2 + // num_params
			2;  // num_output_format_codes
		for (auto it = input.params_begin; it != input.params_end; ++it)
		{
			res += std::visit([](auto v) {
				using T = decltype(v);
				if constexpr (std::is_arithmetic_v<T>)
				{
					return std::size_t(std::to_string(v).size() + 4); // length
				}
				else if constexpr (std::is_same_v<T, std::string_view>)
				{
					return std::size_t(v.size() + 4); // length
				}
				else // NULL
				{
					return std::size_t(4);
				}
			}, *it);
		}
		return res;
	}

	static void serialize_(const msg_type& input, serialization_context& ctx)
	{
		serialize(input.portal_name, ctx);
		serialize(input.statement_name, ctx);
		serialize(std::int16_t(0), ctx); // format codes
		serialize(std::int16_t(std::distance(input.params_begin, input.params_end)), ctx);
		for (auto it = input.params_begin; it != input.params_end; ++it)
		{
			std::visit([&ctx](auto v) {
				using T = decltype(v);
				if constexpr (std::is_arithmetic_v<T>)
				{
					auto s = std::to_string(v);
					//serialize(std::int32_t(s.size()), ctx);
					serialize(string_lenenc(s), ctx);
				}
				else if constexpr (std::is_same_v<T, std::string_view>)
				{
					//serialize(std::int32_t(v.size()), ctx);
					serialize(string_lenenc(v), ctx);
				}
				else // NULL
				{
					serialize(std::int32_t(-1), ctx);
				}
			}, *it);
		}
		serialize(std::int16_t(0), ctx); // output format codes
	}
};

using bind_complete_message = empty_message<'2'>;

struct execute_message
{
	string_null portal_name;
	std::int32_t max_rows = 0;

	static constexpr std::uint8_t message_type = std::uint8_t('E');
};

template <>
struct get_struct_fields<execute_message>
{
	static constexpr auto value = std::make_tuple(
		&execute_message::portal_name,
		&execute_message::max_rows
	);
};

struct describe_message
{
	std::uint8_t type; // 'P' for Portal, 'S' for statement
	string_null name;

	static constexpr std::uint8_t message_type = std::uint8_t('D');
};

template <>
struct get_struct_fields<describe_message>
{
	static constexpr auto value = std::make_tuple(
		&describe_message::type,
		&describe_message::name
	);
};


using flush_message = empty_message<'H'>;
using sync_message = empty_message<'S'>;


#endif /* INCLUDE_PSQL_MESSAGES_H_ */
