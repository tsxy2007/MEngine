#pragma once

#include "flow_builder.hpp"

namespace tf {

	// Procedure: join
	void Subflow::join() {
		_detached = false;
	}

	// Procedure: detach
	void Subflow::detach() {
		_detached = true;
	}

	// Function: detached
	bool Subflow::detached() const {
		return _detached;
	}

	// Function: joined
	bool Subflow::joined() const {
		return !_detached;
	}

	// Procedure: linearize
	void FlowBuilder::linearize(std::vector<Task>& keys) {
		_linearize(keys);
	}

	// Procedure: linearize
	void FlowBuilder::linearize(std::initializer_list<Task> keys) {
		_linearize(keys);
	}

	// Constructor
	FlowBuilder::FlowBuilder(Graph& graph) :
		_graph{ graph } {
	}

	// Procedure: precede
	void FlowBuilder::precede(Task from, Task to) {
		from._node->precede(*(to._node));
	}

	// Procedure: broadcast
	void FlowBuilder::broadcast(Task from, std::vector<Task>& keys) {
		from.precede(keys);
	}

	// Procedure: broadcast
	void FlowBuilder::broadcast(Task from, std::initializer_list<Task> keys) {
		from.precede(keys);
	}

	// Function: gather
	void FlowBuilder::gather(std::vector<Task>& keys, Task to) {
		to.gather(keys);
	}

	// Function: gather
	void FlowBuilder::gather(std::initializer_list<Task> keys, Task to) {
		to.gather(keys);
	}

	// Function: placeholder
	Task FlowBuilder::placeholder() {
		auto& node = _graph.emplace_back();
		return Task(node);
	}


}  // end of namespace tf. ---------------------------------------------------
