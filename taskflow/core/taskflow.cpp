#include "taskflow.hpp"
namespace tf
{

	// Constructor
	Taskflow::Taskflow(const std::string& name) :
		FlowBuilder{ _graph },
		_name{ name } {
	}

	// Constructor
	Taskflow::Taskflow() : FlowBuilder{ _graph } {
	}

	// Destructor
	Taskflow::~Taskflow() {
		assert(_topologies.empty());
	}

	// Procedure:
	void Taskflow::clear() {
		_graph.clear();
	}

	// Function: num_noces
	size_t Taskflow::num_nodes() const {
		return _graph.size();
	}

	// Function: empty
	bool Taskflow::empty() const {
		return _graph.empty();
	}

	// Function: name
	Taskflow& Taskflow::name(const std::string &name) {
		_name = name;
		return *this;
	}

	// Function: name
	const std::string& Taskflow::name() const {
		return _name;
	}

	// Function: composed_of
	tf::Task Taskflow::composed_of(Taskflow& taskflow) {
		auto &node = _graph.emplace_back();
		node._module = &taskflow;
		return Task(node);
	}

	// Procedure: dump
	std::string Taskflow::dump() const {
		std::ostringstream oss;
		dump(oss);
		return oss.str();
	}

	// Function: dump
	void Taskflow::dump(std::ostream& os) const {

		std::stack<const Taskflow*> stack;
		std::unordered_set<const Taskflow*> visited;

		os << "digraph Taskflow_";
		if (_name.empty()) os << 'p' << this;
		else os << _name;
		os << " {\nrankdir=\"LR\";\n";

		stack.push(this);
		visited.insert(this);

		while (!stack.empty()) {

			auto f = stack.top();
			stack.pop();

			// create a subgraph field for this taskflow
			os << "subgraph cluster_";
			if (f->_name.empty()) os << 'p' << f;
			else os << f->_name;
			os << " {\n";

			os << "label=\"Taskflow_";
			if (f->_name.empty()) os << 'p' << f;
			else os << f->_name;
			os << "\";\n";

			// dump the details of this taskflow
			for (const auto& n : f->_graph.nodes()) {

				// regular task
				if (auto module = n->_module; !module) {
					n->dump(os);
				}
				// module task
				else {
					os << 'p' << n.get() << "[shape=box3d, color=blue, label=\"";
					if (n->_name.empty()) os << n.get();
					else os << n->_name;
					os << " (Taskflow_";
					if (module->_name.empty()) os << module;
					else os << module->_name;
					os << ")\"];\n";

					if (visited.find(module) == visited.end()) {
						visited.insert(module);
						stack.push(module);
					}

					for (const auto s : n->_successors) {
						os << 'p' << n.get() << "->" << 'p' << s << ";\n";
					}
				}
			}
			os << "}\n";
		}

		os << "}\n";
	}
}