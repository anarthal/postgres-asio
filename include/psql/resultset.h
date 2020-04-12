#ifndef INCLUDE_PSQL_RESULTSET_H_
#define INCLUDE_PSQL_RESULTSET_H_

#include "psql/channel.h"
#include "psql/row.h"
#include "psql/deserialize_row.h"

template <typename StreamType>
class resultset
{
	using channel_type = channel<StreamType>;

	channel_type* channel_;
	resultset_metadata meta_;
	row current_row_;
	bytestring buffer_;
	bool complete_ {false};
public:
	/// Default constructor.
	resultset(): channel_(nullptr) {};

	// Private, do not use
	resultset(channel_type& channel, resultset_metadata&& meta):
		channel_(&channel), meta_(std::move(meta)) {};

	const row* fetch_one()
	{
		assert(channel_);
		if (complete_) return nullptr;

		// Read message
		std::uint8_t msg_type = 0;
		channel_->read(buffer_, msg_type);

		// Check for end of resultset
		if (msg_type == std::uint8_t('C')) // Complete
		{
			channel_->read(buffer_, msg_type);
			if (msg_type != std::uint8_t('Z')) // Ready for query
			{
				throw std::runtime_error("Expected ready for query");
			}
			complete_ = true;
			return nullptr;
		}

		// We got an actual row, deserialize it
		current_row_ = row(deserialize_row(meta_.fields(), buffer_));
		return &current_row_;
	}

	const std::vector<field_metadata>& fields() const noexcept { return meta_.fields(); }
};



#endif /* INCLUDE_PSQL_RESULTSET_H_ */
