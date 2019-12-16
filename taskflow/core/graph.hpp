#pragma once

#include "../error/error.hpp"
#include "../utility/traits.hpp"
#include "../utility/passive_vector.hpp"

namespace tf {

// Forward declaration
class Node;
class Topology;
class Task;
class FlowBuilder;
class Subflow;
class Taskflow;

// ----------------------------------------------------------------------------

// Class: Graph
class Graph {

  friend class Node;
  
  public:

    Graph() = default;
    Graph(const Graph&) = delete;
    Graph(Graph&&);

    Graph& operator = (const Graph&) = delete;
    Graph& operator = (Graph&&);
    
    void clear();

    bool empty() const;

    size_t size() const;
    
    template <typename C>
    Node& emplace_back(C&&); 

    Node& emplace_back();

    std::vector<std::unique_ptr<Node>>& nodes();

    const std::vector<std::unique_ptr<Node>>& nodes() const;

  private:
    
    std::vector<std::unique_ptr<Node>> _nodes;
};

// ----------------------------------------------------------------------------

// Class: Node
class Node {

  friend class Task;
  friend class TaskView;
  friend class Topology;
  friend class Taskflow;
  friend class Executor;

  using StaticWork  = std::function<void()>;
  using DynamicWork = std::function<void(Subflow&)>;

  constexpr static int SPAWNED = 0x1;
  constexpr static int SUBTASK = 0x2;

  public:

    Node() = default;

    template <typename C>
    Node(C&&);
    
    ~Node();
    
    void precede(Node&);
    void dump(std::ostream&) const;

    size_t num_successors() const;
    size_t num_dependents() const;
    
    const std::string& name() const;

    std::string dump() const;

    // Status-related functions
    bool is_spawned() const { return _status & SPAWNED; }
    bool is_subtask() const { return _status & SUBTASK; }

    void set_spawned()   { _status |= SPAWNED;  }
    void set_subtask()   { _status |= SUBTASK;  }
    void unset_spawned() { _status &= ~SPAWNED; }
    void unset_subtask() { _status &= ~SUBTASK; }
    void clear_status()  { _status = 0;         }

  private:
    
    std::string _name;
    std::variant<std::monostate, StaticWork, DynamicWork> _work;

    tf::PassiveVector<Node*> _successors;
    tf::PassiveVector<Node*> _dependents;
    
    std::optional<Graph> _subgraph;

    Topology* _topology {nullptr};
    Taskflow* _module {nullptr};

    int _status {0};
    
    std::atomic<int> _num_dependents {0};
};

// Constructor
template <typename C>
Node::Node(C&& c) : _work {std::forward<C>(c)} {
}

// ----------------------------------------------------------------------------

/*// Class: NodePool
class NodePool {

  public:

    template <typename C>
    std::unique_ptr<Node> acquire(C&&);

    std::unique_ptr<Node> acquire();

    void release(std::unique_ptr<Node>);
  
  private:
    
    //std::mutex _mutex;

    std::vector<std::unique_ptr<Node>> _nodes;

    void _recycle(Node&);
};

// Function: acquire
template <typename C>
std::unique_ptr<Node> NodePool::acquire(C&& c) {
  if(_nodes.empty()) {
    return std::make_unique<Node>(std::forward<C>(c));
  }
  else {
    auto node = std::move(_nodes.back());
    node->_work = std::forward<C>(c);
    _nodes.pop_back();
    return node;
  }
}

// Function: acquire
std::unique_ptr<Node> NodePool::acquire() {
  if(_nodes.empty()) {
    return std::make_unique<Node>();
  }
  else {
    auto node = std::move(_nodes.back());
    _nodes.pop_back();
    return node;
  }
}

// Procedure: release
void NodePool::release(std::unique_ptr<Node> node) {

  return;

  //assert(node);
  if(_nodes.size() >= 65536) {
    return;
  }
  
  auto children = node->_extract_children();

  for(auto& child : children) {
    _recycle(*child);
  }
  _recycle(*node);

  std::move(children.begin(), children.end(), std::back_inserter(_nodes));  
  _nodes.push_back(std::move(node));
}

// Procedure: _recycle
void NodePool::_recycle(Node& node) {
  node._name.clear();
  node._work = {};
  node._successors.clear();
  node._dependents.clear();
  node._topology = nullptr;
  node._module = nullptr;
  node._status = 0;
  node._num_dependents.store(0, std::memory_order_relaxed);
  //assert(!node._subgraph);
}

// ----------------------------------------------------------------------------

namespace this_thread {
  thread_local NodePool nodepool;
}
*/

// ----------------------------------------------------------------------------

// Function: emplace_back
// create a node from a give argument; constructor is called if necessary
template <typename C>
Node& Graph::emplace_back(C&& c) {
  _nodes.push_back(std::make_unique<Node>(std::forward<C>(c)));
  return *(_nodes.back());
}



}  // end of namespace tf. ---------------------------------------------------





