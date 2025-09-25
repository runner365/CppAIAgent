#include "timeex.hpp"

namespace cpp_streamer
{
	static int64_t s_now_ms = 0;

	void UpdateNowMilliSec(int64_t now_ms) {
		s_now_ms = now_ms;
	}
	int64_t GetNowMilliSec() {
		if (s_now_ms <= 0) {
			return now_millisec();
		}
		return s_now_ms;
	}
}