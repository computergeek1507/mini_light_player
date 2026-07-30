#ifndef RINGBUFFERSINK_H
#define RINGBUFFERSINK_H

#ifdef MINIPLAYER_ENABLE_TUI

#include "spdlog/details/log_msg.h"
#include "spdlog/sinks/base_sink.h"

#include <deque>
#include <mutex>
#include <string>
#include <vector>

// Keeps the last N formatted log lines in memory, so the terminal dashboard
// can show a scrolling log tail without reading log.txt back off disk, and
// without needing the plain stdout sink - which a full-screen interactive
// display can't share a terminal with.
class RingBufferSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
	explicit RingBufferSink(size_t capacity) : m_capacity(capacity) {}

	// Called from the UI thread, outside the base_sink's own locked call
	// path, so this locks independently.
	[[nodiscard]] std::vector<std::string> Lines() const
	{
		std::lock_guard<std::mutex> lock(m_linesMutex);
		return std::vector<std::string>(m_lines.begin(), m_lines.end());
	}

protected:
	void sink_it_(spdlog::details::log_msg const& msg) override
	{
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);

		std::lock_guard<std::mutex> lock(m_linesMutex);
		m_lines.emplace_back(formatted.data(), formatted.size());
		if (m_lines.size() > m_capacity)
		{
			m_lines.pop_front();
		}
	}

	void flush_() override {}

private:
	size_t const m_capacity;
	std::deque<std::string> m_lines;
	mutable std::mutex m_linesMutex;
};

#endif
#endif
