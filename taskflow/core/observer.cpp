#include "observer.hpp"
namespace tf
{
	// Procedure: set_up
	void ExecutorObserver::set_up(unsigned num_workers) {

		_timeline.executions.resize(num_workers);

		for (unsigned w = 0; w < num_workers; ++w) {
			_timeline.executions[w].reserve(1024);
		}

		_timeline.origin = std::chrono::steady_clock::now();
	}

	// Procedure: on_entry
	void ExecutorObserver::on_entry(unsigned w, TaskView tv) {
		_timeline.executions[w].emplace_back(tv, std::chrono::steady_clock::now());
	}

	// Procedure: on_exit
	void ExecutorObserver::on_exit(unsigned w, TaskView tv) {
		static_cast<void>(tv);  // avoid warning from compiler
		assert(_timeline.executions[w].size() > 0);
		_timeline.executions[w].back().end = std::chrono::steady_clock::now();
	}

	// Function: clear
	void ExecutorObserver::clear() {
		for (size_t w = 0; w < _timeline.executions.size(); ++w) {
			_timeline.executions[w].clear();
		}
	}

	// Procedure: dump
	void ExecutorObserver::dump(std::ostream& os) const {

		size_t first;

		for (first = 0; first < _timeline.executions.size(); ++first) {
			if (_timeline.executions[first].size() > 0) {
				break;
			}
		}

		os << '[';

		for (size_t w = first; w < _timeline.executions.size(); w++) {

			if (w != first && _timeline.executions[w].size() > 0) {
				os << ',';
			}

			for (size_t i = 0; i < _timeline.executions[w].size(); i++) {

				os << '{'
					<< "\"cat\":\"ExecutorObserver\","
					<< "\"name\":\"" << _timeline.executions[w][i].task_view.name() << "\","
					<< "\"ph\":\"X\","
					<< "\"pid\":1,"
					<< "\"tid\":" << w << ','
					<< "\"ts\":" << std::chrono::duration_cast<std::chrono::microseconds>(
						_timeline.executions[w][i].beg - _timeline.origin
						).count() << ','
					<< "\"dur\":" << std::chrono::duration_cast<std::chrono::microseconds>(
						_timeline.executions[w][i].end - _timeline.executions[w][i].beg
						).count();

				if (i != _timeline.executions[w].size() - 1) {
					os << "},";
				}
				else {
					os << '}';
				}
			}
		}
		os << "]\n";
	}

	// Function: dump
	std::string ExecutorObserver::dump() const {
		std::ostringstream oss;
		dump(oss);
		return oss.str();
	}

	// Function: num_tasks
	size_t ExecutorObserver::num_tasks() const {
		return std::accumulate(
			_timeline.executions.begin(), _timeline.executions.end(), size_t{ 0 },
			[](size_t sum, const auto& exe) {
			return sum + exe.size();
		}
		);
	}
}