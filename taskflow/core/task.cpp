#include "task.hpp"
namespace tf
{

	// Constructor
	TaskView::TaskView(Node& node) : _node{ &node } {
	}

	// Constructor
	TaskView::TaskView(Node* node) : _node{ node } {
	}

	// Constructor
	TaskView::TaskView(const TaskView& rhs) : _node{ rhs._node } {
	}

	// Constructor
	TaskView::TaskView(const Task& task) : _node{ task._node } {
	}

	// Operator =
	TaskView& TaskView::operator = (const TaskView& rhs) {
		_node = rhs._node;
		return *this;
	}

	// Operator =
	TaskView& TaskView::operator = (const Task& rhs) {
		_node = rhs._node;
		return *this;
	}

	// Operator =
	TaskView& TaskView::operator = (std::nullptr_t ptr) {
		_node = ptr;
		return *this;
	}

	// Function: name
	const std::string& TaskView::name() const {
		return _node->_name;
	}

	// Function: num_dependents
	size_t TaskView::num_dependents() const {
		return _node->num_dependents();
	}

	// Function: num_successors
	size_t TaskView::num_successors() const {
		return _node->num_successors();
	}

	// Function: reset
	void TaskView::reset() {
		_node = nullptr;
	}

	// Function: empty
	bool TaskView::empty() const {
		return _node == nullptr;
	}
	// Function: name
	Task& Task::name(const std::string& name) {
		_node->_name = name;
		return *this;
	}

	// Procedure: reset
	Task& Task::reset() {
		_node = nullptr;
		return *this;
	}

	// Function: name
	const std::string& Task::name() const {
		return _node->_name;
	}

	// Function: num_dependents
	size_t Task::num_dependents() const {
		return _node->num_dependents();
	}

	// Function: num_successors
	size_t Task::num_successors() const {
		return _node->num_successors();
	}

	// Function: empty
	bool Task::empty() const {
		return _node == nullptr;
	}

	// Function: has_work
	bool Task::has_work() const {
		return _node ? _node->_work.index() != 0 : false;
	}

	// Function: succeed
	Task& Task::succeed(std::vector<Task>& tgts) {
		_succeed(tgts);
		return *this;
	}

	// Function: succeed
	Task& Task::succeed(std::initializer_list<Task> tgts) {
		_succeed(tgts);
		return *this;
	}

	// Operator =
	Task& Task::operator = (const Task& rhs) {
		_node = rhs._node;
		return *this;
	}

	// Operator =
	Task& Task::operator = (std::nullptr_t ptr) {
		_node = ptr;
		return *this;
	}
	// Constructor
	Task::Task(Node& node) : _node{ &node } {
	}

	// Constructor
	Task::Task(Node* node) : _node{ node } {
	}

	// Constructor
	Task::Task(const Task& rhs) : _node{ rhs._node } {
	}

	// Function: precede
	Task& Task::precede(std::vector<Task>& tgts) {
		_precede(tgts);
		return *this;
	}

	// Function: precede
	Task& Task::precede(std::initializer_list<Task> tgts) {
		_precede(tgts);
		return *this;
	}


	// Function: gather
	Task& Task::gather(std::vector<Task>& tgts) {
		_gather(tgts);
		return *this;
	}

	// Function: gather
	Task& Task::gather(std::initializer_list<Task> tgts) {
		_gather(tgts);
		return *this;
	}

}