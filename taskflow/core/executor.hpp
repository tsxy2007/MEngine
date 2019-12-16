#pragma once

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <random>
#include <atomic>
#include <memory>
#include <deque>
#include <optional>
#include <thread>
#include <algorithm>
#include <set>
#include <numeric>
#include <cassert>
#include <mutex>
#include "spmc_queue.hpp"
#include "notifier.hpp"
#include "observer.hpp"
#include "taskflow.hpp"

namespace tf {

/** @class Executor

@brief The executor class to run a taskflow graph.

An executor object manages a set of worker threads and implements 
an efficient work-stealing scheduling algorithm to run a task graph.

*/
class Executor {
  
  struct Worker {
    std::mt19937 rdgen { std::random_device{}() };
    WorkStealingQueue<Node*> queue;
    std::optional<Node*> cache;
  };
    
  struct PerThread {
    Executor* pool {nullptr}; 
    int worker_id  {-1};
  };

  public:
    
    /**
    @brief constructs the executor with N worker threads
    */
    explicit Executor(unsigned n = std::thread::hardware_concurrency());
    
    /**
    @brief destructs the executor 
    */
    ~Executor();

    /**
    @brief runs the taskflow once
    
    @param taskflow a tf::Taskflow object

    @return a std::future to access the execution status of the taskflow
    */
    std::future<void> run(Taskflow& taskflow);

    /**
    @brief runs the taskflow once and invoke a callback upon completion

    @param taskflow a tf::Taskflow object 
    @param callable a callable object to be invoked after this run

    @return a std::future to access the execution status of the taskflow
    */
    template<typename C>
    std::future<void> run(Taskflow& taskflow, C&& callable);

    /**
    @brief runs the taskflow for N times
    
    @param taskflow a tf::Taskflow object
    @param N number of runs

    @return a std::future to access the execution status of the taskflow
    */
    std::future<void> run_n(Taskflow& taskflow, size_t N);

    /**
    @brief runs the taskflow for N times and then invokes a callback

    @param taskflow a tf::Taskflow 
    @param N number of runs
    @param callable a callable object to be invoked after this run

    @return a std::future to access the execution status of the taskflow
    */
    template<typename C>
    std::future<void> run_n(Taskflow& taskflow, size_t N, C&& callable);

    /**
    @brief runs the taskflow multiple times until the predicate becomes true and 
           then invokes a callback

    @param taskflow a tf::Taskflow 
    @param pred a boolean predicate to return true for stop

    @return a std::future to access the execution status of the taskflow
    */
    template<typename P>
    std::future<void> run_until(Taskflow& taskflow, P&& pred);

    /**
    @brief runs the taskflow multiple times until the predicate becomes true and 
           then invokes the callback

    @param taskflow a tf::Taskflow 
    @param pred a boolean predicate to return true for stop
    @param callable a callable object to be invoked after this run

    @return a std::future to access the execution status of the taskflow
    */
    template<typename P, typename C>
    std::future<void> run_until(Taskflow& taskflow, P&& pred, C&& callable);

    /**
    @brief wait for all pending graphs to complete
    */
    void wait_for_all();

    /**
    @brief queries the number of worker threads (can be zero)

    @return the number of worker threads
    */
    size_t num_workers() const;
    
    /**
    @brief constructs an observer to inspect the activities of worker threads

    Each executor manages at most one observer at a time through std::unique_ptr.
    Createing multiple observers will only keep the lastest one.
    
    @tparam Observer observer type derived from tf::ExecutorObserverInterface
    @tparam ArgsT... argument parameter pack

    @param args arguments to forward to the constructor of the observer
    
    @return a raw pointer to the observer associated with this executor
    */
    template<typename Observer, typename... Args>
    Observer* make_observer(Args&&... args);
    
    /**
    @brief removes the associated observer
    */
    void remove_observer();

  private:
   
    std::condition_variable _topology_cv;
    std::mutex _topology_mutex;
    std::mutex _queue_mutex;

    unsigned _num_topologies {0};
    
    // scheduler field
    std::vector<Worker> _workers;
    std::vector<Notifier::Waiter> _waiters;
    std::vector<std::thread> _threads;

    WorkStealingQueue<Node*> _queue;

    std::atomic<size_t> _num_actives {0};
    std::atomic<size_t> _num_thieves {0};
    std::atomic<bool>   _done        {0};

    Notifier _notifier;
    
    std::unique_ptr<ExecutorObserverInterface> _observer;
    
    unsigned _find_victim(unsigned);

    PerThread& _per_thread() const;

    bool _wait_for_task(unsigned, std::optional<Node*>&);
    
    void _spawn(unsigned);
    void _exploit_task(unsigned, std::optional<Node*>&);
    void _explore_task(unsigned, std::optional<Node*>&);
    void _schedule(Node*, bool);
    void _schedule(PassiveVector<Node*>&);
    void _invoke(unsigned, Node*);
    void _invoke_static_work(unsigned, Node*);
    void _invoke_dynamic_work(unsigned, Node*, Subflow&);
    void _init_module_node(Node*);
    void _tear_down_topology(Topology*); 
    void _increment_topology();
    void _decrement_topology();
    void _decrement_topology_and_notify();
};

// Function: make_observer    
template<typename Observer, typename... Args>
Observer* Executor::make_observer(Args&&... args) {
	// use a local variable to mimic the constructor 
	auto tmp = std::make_unique<Observer>(std::forward<Args>(args)...);
	tmp->set_up(_workers.size());
	_observer = std::move(tmp);
	return static_cast<Observer*>(_observer.get());
}

// Function: run_n
template <typename C>
std::future<void> Executor::run_n(Taskflow& f, size_t repeat, C&& c) {
	return run_until(f, [repeat]() mutable { return repeat-- == 0; }, std::forward<C>(c));
}

// Function: run_until    
template<typename P>
std::future<void> Executor::run_until(Taskflow& f, P&& pred) {
	return run_until(f, std::forward<P>(pred), []() {});
}


// Function: run_until
template <typename P, typename C>
std::future<void> Executor::run_until(Taskflow& f, P&& pred, C&& c) {

	// Predicate must return a boolean value
	static_assert(std::is_invocable_v<C> && std::is_invocable_v<P>);

	_increment_topology();

	// Special case of predicate
	if (f.empty() || std::invoke(pred)) {
		std::promise<void> promise;
		promise.set_value();
		_decrement_topology_and_notify();
		return promise.get_future();
	}

	if (_workers.size() == 0) {
		TF_THROW(Error::EXECUTOR, "no workers to execute the graph");
	}

	//// Special case of zero workers requires:
	////  - iterative execution to avoid stack overflow
	////  - avoid execution of last_work
	//if(_workers.size() == 0) {
	//  
	//  Topology tpg(f, std::forward<P>(pred), std::forward<C>(c));

	//  // Clear last execution data & Build precedence between nodes and target
	//  tpg._bind(f._graph);

	//  std::stack<Node*> stack;

	//  do {
	//    _schedule_unsync(tpg._sources, stack);
	//    while(!stack.empty()) {
	//      auto node = stack.top();
	//      stack.pop();
	//      _invoke_unsync(node, stack);
	//    }
	//    tpg._recover_num_sinks();
	//  } while(!std::invoke(tpg._pred));

	//  if(tpg._call != nullptr) {
	//    std::invoke(tpg._call);
	//  }

	//  tpg._promise.set_value();
	//  
	//  _decrement_topology_and_notify();
	//  
	//  return tpg._promise.get_future();
	//}

	// Multi-threaded execution.
	bool run_now{ false };
	Topology* tpg;
	std::future<void> future;

	{
		std::scoped_lock lock(f._mtx);

		// create a topology for this run
		tpg = &(f._topologies.emplace_back(f, std::forward<P>(pred), std::forward<C>(c)));
		future = tpg->_promise.get_future();

		if (f._topologies.size() == 1) {
			run_now = true;
			//tpg->_bind(f._graph);
			//_schedule(tpg->_sources);
		}
	}

	// Notice here calling schedule may cause the topology to be removed sonner 
	// before the function leaves.
	if (run_now) {
		tpg->_bind(f._graph);
		_schedule(tpg->_sources);
	}

	return future;
}

}  // end of namespace tf -----------------------------------------------------


