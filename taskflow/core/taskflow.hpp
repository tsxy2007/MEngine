#pragma once

#include <stack>
#include "flow_builder.hpp"
#include "topology.hpp"

namespace tf {

/**
@class Taskflow 

@brief the class to create a task dependency graph

*/
class Taskflow : public FlowBuilder {

  friend class Topology;
  friend class Executor;

  public:

    /**
    @brief constructs a taskflow with a given name
    */
    Taskflow(const std::string& name);

    /**
    @brief constructs a taskflow
    */
    Taskflow();

    /**
    @brief destroy the taskflow (virtual call)
    */
    virtual ~Taskflow();
    
    /**
    @brief dumps the taskflow to a std::ostream in DOT format

    @param ostream a std::ostream target
    */
    void dump(std::ostream& ostream) const;
    
    /**
    @brief dumps the taskflow in DOT format to a std::string
    */
    std::string dump() const;
    
    /**
    @brief queries the number of nodes in the taskflow
    */
    size_t num_nodes() const;
    
    /**
    @brief queries the emptiness of the taskflow
    */
    bool empty() const;

    /**
    @brief creates a module task from a taskflow

    @param taskflow a taskflow object to create the module
    */
    tf::Task composed_of(Taskflow& taskflow);

    /**
    @brief sets the name of the taskflow
    
    @return @c *this
    */
    tf::Taskflow& name(const std::string&); 

    /**
    @brief queries the name of the taskflow
    */
    const std::string& name() const ;
    
    /**
    @brief clears the associated task dependency graph
    */
    void clear();

  private:
 
    std::string _name;
   
    Graph _graph;

    std::mutex _mtx;

    std::list<Topology> _topologies;

    //std::deque<Topology*> _topologies;
};


// Backward compatibility
using Framework = Taskflow;

}  // end of namespace tf. ---------------------------------------------------

