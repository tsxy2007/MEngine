#include "graph.hpp"
namespace tf
{
	// Destructor
	Node::~Node() {
		// this is to avoid stack overflow
		if (_subgraph.has_value()) {
			std::vector<std::unique_ptr<Node>> nodes;
			std::move(
				_subgraph->_nodes.begin(), _subgraph->_nodes.end(), std::back_inserter(nodes)
			);
			_subgraph->_nodes.clear();
			_subgraph.reset();
			size_t i = 0;
			while (i < nodes.size()) {
				if (auto& sbg = nodes[i]->_subgraph; sbg) {
					std::move(
						sbg->_nodes.begin(), sbg->_nodes.end(), std::back_inserter(nodes)
					);
					sbg->_nodes.clear();
					sbg.reset();
				}
				++i;
			}
		}
	}

	// Procedure: precede
	void Node::precede(Node& v) {
		_successors.push_back(&v);
		v._dependents.push_back(this);
		v._num_dependents.fetch_add(1, std::memory_order_relaxed);
	}

	// Function: num_successors
	size_t Node::num_successors() const {
		return _successors.size();
	}

	// Function: dependents
	size_t Node::num_dependents() const {
		return _dependents.size();
	}

	// Function: name
	const std::string& Node::name() const {
		return _name;
	}

	// Function: dump
	std::string Node::dump() const {
		std::ostringstream os;
		dump(os);
		return os.str();
	}

	// Function: dump
	void Node::dump(std::ostream& os) const {

		os << 'p' << this << "[label=\"";
		if (_name.empty()) os << 'p' << this;
		else os << _name;
		os << "\"];\n";

		for (const auto s : _successors) {
			os << 'p' << this << " -> " << 'p' << s << ";\n";
		}

		if (_subgraph && !_subgraph->empty()) {

			os << "subgraph cluster_";
			if (_name.empty()) os << 'p' << this;
			else os << _name;
			os << " {\n";

			os << "label=\"Subflow_";
			if (_name.empty()) os << 'p' << this;
			else os << _name;

			os << "\";\n" << "color=blue\n";

			for (const auto& n : _subgraph->nodes()) {
				n->dump(os);
			}
			os << "}\n";
		}
	}

	// Move constructor
	Graph::Graph(Graph&& other) :
		_nodes{ std::move(other._nodes) } {
	}

	// Move assignment
	Graph& Graph::operator = (Graph&& other) {
		_nodes = std::move(other._nodes);
		return *this;
	}

	// Procedure: clear
	// clear and recycle the nodes
	void Graph::clear() {
		_nodes.clear();
	}

	// Function: size
	// query the size
	size_t Graph::size() const {
		return _nodes.size();
	}

	// Function: empty
	// query the emptiness
	bool Graph::empty() const {
		return _nodes.empty();
	}

	// Function: nodes
	// return a mutable reference to the node data structure
	//std::vector<std::unique_ptr<Node>>& Graph::nodes() {
	std::vector<std::unique_ptr<Node>>& Graph::nodes() {
		return _nodes;
	}

	// Function: nodes
	// returns a constant reference to the node data structure
	//const std::vector<std::unique_ptr<Node>>& Graph::nodes() const {
	const std::vector<std::unique_ptr<Node>>& Graph::nodes() const {
		return _nodes;
	}
	// Function: emplace_back
// create a node from a give argument; constructor is called if necessary
	Node& Graph::emplace_back() {
		_nodes.push_back(std::make_unique<Node>());
		return *(_nodes.back());
	}

}