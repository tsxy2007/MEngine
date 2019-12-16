#include "executor.hpp"
namespace tf
{

	// Procedure: remove_observer
	void Executor::remove_observer() {
		_observer.reset();
	}

	// Procedure: _schedule
	// The main procedure to schedule a give task node.
	// Each task node has two types of tasks - regular and subflow.
	void Executor::_schedule(Node* node, bool bypass) {

		assert(_workers.size() != 0);

		// module node need another initialization
		if (node->_module != nullptr && !node->_module->empty() && !node->is_spawned()) {
			_init_module_node(node);
		}

		// caller is a worker to this pool
		if (auto& pt = _per_thread(); pt.pool == this) {
			if (!bypass) {
				_workers[pt.worker_id].queue.push(node);
			}
			else {
				assert(!_workers[pt.worker_id].cache);
				_workers[pt.worker_id].cache = node;
			}
			return;
		}

		// other threads
		{
			std::scoped_lock lock(_queue_mutex);
			_queue.push(node);
		}

		_notifier.notify(false);
	}

	// Procedure: _schedule
	// The main procedure to schedule a set of task nodes.
	// Each task node has two types of tasks - regular and subflow.
	void Executor::_schedule(PassiveVector<Node*>& nodes) {

		assert(_workers.size() != 0);

		// We need to cacth the node count to avoid accessing the nodes
		// vector while the parent topology is removed!
		const auto num_nodes = nodes.size();

		if (num_nodes == 0) {
			return;
		}

		for (auto node : nodes) {
			if (node->_module != nullptr && !node->_module->empty() && !node->is_spawned()) {
				_init_module_node(node);
			}
		}

		// worker thread
		if (auto& pt = _per_thread(); pt.pool == this) {
			for (size_t i = 0; i < num_nodes; ++i) {
				_workers[pt.worker_id].queue.push(nodes[i]);
			}
			return;
		}

		// other threads
		{
			std::scoped_lock lock(_queue_mutex);
			for (size_t k = 0; k < num_nodes; ++k) {
				_queue.push(nodes[k]);
			}
		}

		if (num_nodes >= _workers.size()) {
			_notifier.notify(true);
		}
		else {
			for (size_t k = 0; k < num_nodes; ++k) {
				_notifier.notify(false);
			}
		}
	}

	// Procedure: _init_module_node
	void Executor::_init_module_node(Node* node) {

		node->_work = [node = node, this, tgt{ PassiveVector<Node*>() }]() mutable {

			// second time to enter this context
			if (node->is_spawned()) {
				node->_dependents.resize(node->_dependents.size() - tgt.size());
				for (auto& t : tgt) {
					t->_successors.clear();
				}
				return;
			}

			// first time to enter this context
			node->set_spawned();

			PassiveVector<Node*> src;

			for (auto& n : node->_module->_graph.nodes()) {
				n->_topology = node->_topology;
				if (n->num_dependents() == 0) {
					src.push_back(n.get());
				}
				if (n->num_successors() == 0) {
					n->precede(*node);
					tgt.push_back(n.get());
				}
			}

			_schedule(src);
		};
	}

	// Procedure: _invoke
	void Executor::_invoke(unsigned me, Node* node) {

		assert(_workers.size() != 0);

		// Here we need to fetch the num_successors first to avoid the invalid memory
		// access caused by topology clear.
		const auto num_successors = node->num_successors();

		// static task
		// The default node work type. We only need to execute the callback if any.
		if (auto index = node->_work.index(); index == 1) {
			if (node->_module != nullptr) {
				bool first_time = !node->is_spawned();
				_invoke_static_work(me, node);
				if (first_time) {
					return;
				}
			}
			else {
				_invoke_static_work(me, node);
			}
		}
		// dynamic task
		else if (index == 2) {

			// Clear the subgraph before the task execution
			if (!node->is_spawned()) {
				if (node->_subgraph) {
					node->_subgraph->clear();
				}
				else {
					node->_subgraph.emplace();
				}
			}

			Subflow fb(*(node->_subgraph));

			_invoke_dynamic_work(me, node, fb);

			// Need to create a subflow if first time & subgraph is not empty 
			if (!node->is_spawned()) {
				node->set_spawned();
				if (!node->_subgraph->empty()) {
					// For storing the source nodes
					PassiveVector<Node*> src;
					for (auto& n : node->_subgraph->nodes()) {
						n->_topology = node->_topology;
						n->set_subtask();
						if (n->num_successors() == 0) {
							if (fb.detached()) {
								node->_topology->_num_sinks++;
							}
							else {
								n->precede(*node);
							}
						}
						if (n->num_dependents() == 0) {
							src.push_back(n.get());
						}
					}

					_schedule(src);

					if (fb.joined()) {
						return;
					}
				}
			}
		} // End of DynamicWork -----------------------------------------------------

		// Recover the runtime change due to dynamic tasking except the target & spawn tasks 
		// This must be done before scheduling the successors, otherwise this might cause 
		// race condition on the _dependents
		//if(num_successors && !node->_subtask) {
		if (!node->is_subtask()) {
			// Only dynamic tasking needs to restore _dependents
			// TODO:
			if (node->_work.index() == 2 && !node->_subgraph->empty()) {
				while (!node->_dependents.empty() && node->_dependents.back()->is_subtask()) {
					node->_dependents.pop_back();
				}
			}
			node->_num_dependents = static_cast<int>(node->_dependents.size());
			node->unset_spawned();
		}

		// At this point, the node storage might be destructed.
		Node* cache{ nullptr };

		for (size_t i = 0; i < num_successors; ++i) {
			if (--(node->_successors[i]->_num_dependents) == 0) {
				if (cache) {
					_schedule(cache, false);
				}
				cache = node->_successors[i];
			}
		}

		if (cache) {
			_schedule(cache, true);
		}

		// A node without any successor should check the termination of topology
		if (num_successors == 0) {
			if (--(node->_topology->_num_sinks) == 0) {
				_tear_down_topology(node->_topology);
			}
		}
	}

	// Procedure: _increment_topology
	void Executor::_increment_topology() {
		std::scoped_lock lock(_topology_mutex);
		++_num_topologies;
	}

	// Procedure: _decrement_topology_and_notify
	void Executor::_decrement_topology_and_notify() {
		std::scoped_lock lock(_topology_mutex);
		if (--_num_topologies == 0) {
			_topology_cv.notify_all();
		}
	}

	// Procedure: _decrement_topology
	void Executor::_decrement_topology() {
		std::scoped_lock lock(_topology_mutex);
		--_num_topologies;
	}

	// Procedure: wait_for_all
	void Executor::wait_for_all() {
		std::unique_lock lock(_topology_mutex);
		_topology_cv.wait(lock, [&]() { return _num_topologies == 0; });
	}
	// Function: _tear_down_topology
	void Executor::_tear_down_topology(Topology* tpg) {

		auto &f = tpg->_taskflow;

		//assert(&tpg == &(f._topologies.front()));

		// case 1: we still need to run the topology again
		if (!std::invoke(tpg->_pred)) {
			tpg->_recover_num_sinks();
			_schedule(tpg->_sources);
		}
		// case 2: the final run of this topology
		else {

			if (tpg->_call != nullptr) {
				std::invoke(tpg->_call);
			}

			f._mtx.lock();

			// If there is another run (interleave between lock)
			if (f._topologies.size() > 1) {

				// Set the promise
				tpg->_promise.set_value();
				f._topologies.pop_front();
				f._mtx.unlock();

				// decrement the topology but since this is not the last we don't notify
				_decrement_topology();

				f._topologies.front()._bind(f._graph);
				_schedule(f._topologies.front()._sources);
			}
			else {
				assert(f._topologies.size() == 1);

				// Need to back up the promise first here becuz taskflow might be 
				// destroy before taskflow leaves
				auto p{ std::move(tpg->_promise) };

				f._topologies.pop_front();

				f._mtx.unlock();

				// We set the promise in the end in case taskflow leaves before taskflow
				p.set_value();

				_decrement_topology_and_notify();
			}
		}
	}

	// Procedure: _invoke_static_work
	void Executor::_invoke_static_work(unsigned me, Node* node) {
		if (_observer) {
			_observer->on_entry(me, TaskView(node));
			std::invoke(std::get<Node::StaticWork>(node->_work));
			_observer->on_exit(me, TaskView(node));
		}
		else {
			std::invoke(std::get<Node::StaticWork>(node->_work));
		}
	}

	// Procedure: _invoke_dynamic_work
	void Executor::_invoke_dynamic_work(unsigned me, Node* node, Subflow& sf) {
		if (_observer) {
			_observer->on_entry(me, TaskView(node));
			std::invoke(std::get<Node::DynamicWork>(node->_work), sf);
			_observer->on_exit(me, TaskView(node));
		}
		else {
			std::invoke(std::get<Node::DynamicWork>(node->_work), sf);
		}
	}

	// Function: run
	std::future<void> Executor::run(Taskflow& f) {
		return run_n(f, 1, []() {});
	}

	// Function: run
	template <typename C>
	std::future<void> Executor::run(Taskflow& f, C&& c) {
		static_assert(std::is_invocable<C>::value);
		return run_n(f, 1, std::forward<C>(c));
	}

	// Function: run_n
	std::future<void> Executor::run_n(Taskflow& f, size_t repeat) {
		return run_n(f, repeat, []() {});
	}

	// Constructor
	Executor::Executor(unsigned N) :
		_workers{ N },
		_waiters{ N },
		_notifier{ _waiters } {
		_spawn(N);
	}

	// Destructor
	Executor::~Executor() {

		// wait for all topologies to complete
		wait_for_all();

		// shut down the scheduler
		_done = true;
		_notifier.notify(true);

		for (auto& t : _threads) {
			t.join();
		}
	}

	// Function: num_workers
	size_t Executor::num_workers() const {
		return _workers.size();
	}

	// Function: _per_thread
	Executor::PerThread& Executor::_per_thread() const {
		thread_local PerThread pt;
		return pt;
	}

	// Procedure: _spawn
	void Executor::_spawn(unsigned N) {

		// Lock to synchronize all workers before creating _worker_maps
		for (unsigned i = 0; i < N; ++i) {
			_threads.emplace_back([this, i]() -> void {

				PerThread& pt = _per_thread();
				pt.pool = this;
				pt.worker_id = i;

				std::optional<Node*> t;

				// must use 1 as condition instead of !done
				while (1) {

					// execute the tasks.
					_exploit_task(i, t);

					// wait for tasks
					if (_wait_for_task(i, t) == false) {
						break;
					}
				}

			});
		}
	}

	// Function: _find_victim
	unsigned Executor::_find_victim(unsigned thief) {

		/*unsigned l = 0;
		unsigned r = _workers.size() - 1;
		unsigned vtm = std::uniform_int_distribution<unsigned>{l, r}(
		  _workers[thief].rdgen
		);

		// try to look for a task from other workers
		for(unsigned i=0; i<_workers.size(); ++i){

		  if((thief == vtm && !_queue.empty()) ||
			 (thief != vtm && !_workers[vtm].queue.empty())) {
			return vtm;
		  }

		  if(++vtm; vtm == _workers.size()) {
			vtm = 0;
		  }
		} */

		// try to look for a task from other workers
		for (unsigned vtm = 0; vtm < _workers.size(); ++vtm) {
			if ((thief == vtm && !_queue.empty()) ||
				(thief != vtm && !_workers[vtm].queue.empty())) {
				return vtm;
			}
		}

		return _workers.size();
	}

	// Function: _explore_task
	void Executor::_explore_task(unsigned thief, std::optional<Node*>& t) {

		//assert(_workers[thief].queue.empty());
		assert(!t);

		const unsigned l = 0;
		const unsigned r = _workers.size() - 1;

		const size_t F = (_workers.size() + 1) << 1;
		const size_t Y = 100;

		size_t f = 0;
		size_t y = 0;

		// explore
		while (!_done) {

			unsigned vtm = std::uniform_int_distribution<unsigned>{ l, r }(
				_workers[thief].rdgen
				);

			t = (vtm == thief) ? _queue.steal() : _workers[vtm].queue.steal();

			if (t) {
				break;
			}

			if (f++ > F) {
				if (std::this_thread::yield(); y++ > Y) {
					break;
				}
			}

			/*if(auto vtm = _find_victim(thief); vtm != _workers.size()) {
			  t = (vtm == thief) ? _queue.steal() : _workers[vtm].queue.steal();
			  // successful thief
			  if(t) {
				break;
			  }
			}
			else {
			  if(f++ > F) {
				if(std::this_thread::yield(); y++ > Y) {
				  break;
				}
			  }
			}*/
		}

	}

	// Procedure: _exploit_task
	void Executor::_exploit_task(unsigned i, std::optional<Node*>& t) {

		assert(!_workers[i].cache);

		if (t) {
			auto& worker = _workers[i];
			if (_num_actives.fetch_add(1) == 0 && _num_thieves == 0) {
				_notifier.notify(false);
			}
			do {
				_invoke(i, *t);

				if (worker.cache) {
					t = *worker.cache;
					worker.cache = std::nullopt;
				}
				else {
					t = worker.queue.pop();
				}

			} while (t);

			--_num_actives;
		}
	}

	// Function: _wait_for_task
	bool Executor::_wait_for_task(unsigned me, std::optional<Node*>& t) {

	wait_for_task:

		assert(!t);

		++_num_thieves;

	explore_task:

		if (_explore_task(me, t); t) {
			if (auto N = _num_thieves.fetch_sub(1); N == 1) {
				_notifier.notify(false);
			}
			return true;
		}

		_notifier.prepare_wait(&_waiters[me]);

		//if(auto vtm = _find_victim(me); vtm != _workers.size()) {
		if (!_queue.empty()) {

			_notifier.cancel_wait(&_waiters[me]);
			//t = (vtm == me) ? _queue.steal() : _workers[vtm].queue.steal();

			if (t = _queue.steal(); t) {
				if (auto N = _num_thieves.fetch_sub(1); N == 1) {
					_notifier.notify(false);
				}
				return true;
			}
			else {
				goto explore_task;
			}
		}

		if (_done) {
			_notifier.cancel_wait(&_waiters[me]);
			_notifier.notify(true);
			--_num_thieves;
			return false;
		}

		if (_num_thieves.fetch_sub(1) == 1 && _num_actives) {
			_notifier.cancel_wait(&_waiters[me]);
			goto wait_for_task;
		}

		// Now I really need to relinguish my self to others
		_notifier.commit_wait(&_waiters[me]);

		return true;
	}

}