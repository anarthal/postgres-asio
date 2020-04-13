#ifndef INCLUDE_PSQL_CONNECTION_PARAMS_H_
#define INCLUDE_PSQL_CONNECTION_PARAMS_H_

#include <string_view>

namespace psql
{


struct connection_params
{
	std::string_view username;
	std::string_view password;
	std::string_view database;
};

}

#endif /* INCLUDE_PSQL_CONNECTION_PARAMS_H_ */
