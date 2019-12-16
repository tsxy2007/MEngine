// 2019/07/31 - modified by Tsung-Wei Huang
//  - fixed the missing comma in outputing JSON  
//
// 2019/06/13 - modified by Tsung-Wei Huang
//  - added TaskView interface
//
// 2019/04/17 - created by Tsung-Wei Huang

#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <memory>
#include <deque>
#include <optional>
#include <thread>
#include <algorithm>
#include <set>
#include <numeric>
#include <cassert>

#include "task.hpp"

namespace tf {

/**
@class: ExecutorObserverInterface

@brief The interface class for creating an executor observer.

The tf::ExecutorObserver class let users define methods to monitor the behaviors
of an executor. 
This is particularly useful when you want to inspect the performance of an executor.
*/
class ExecutorObserverInterface {
  
  public:

  /**
  @brief virtual destructor
  */
  virtual ~ExecutorObserverInterface() = default;
  
  /**
  @brief constructor-like method to call when the executor observer is fully created
  @param num_workers the number of the worker threads in the executor
  */
  virtual void set_up(unsigned num_workers) = 0;
  
  /**
  @brief method to call before a worker thread executes a closure 
  @param worker_id the id of this worker thread 
  @param task_view a constant wrapper object to the task 
  */
  virtual void on_entry(unsigned worker_id, TaskView task_view) = 0;
  
  /**
  @brief method to call after a worker thread executed a closure
  @param worker_id the id of this worker thread 
  @param task_view a constant wrapper object to the task
  */
  virtual void on_exit(unsigned worker_id, TaskView task_view) = 0;
};

// ------------------------------------------------------------------

/**
@class: ExecutorObserver

@brief Default executor observer to dump the execution timelines

*/
class ExecutorObserver : public ExecutorObserverInterface {

  friend class Executor;
  
  // data structure to record each task execution
  struct Execution {

    TaskView task_view;

    std::chrono::time_point<std::chrono::steady_clock> beg;
    std::chrono::time_point<std::chrono::steady_clock> end;

    Execution(
      TaskView tv, 
      std::chrono::time_point<std::chrono::steady_clock> b
    ) :
      task_view {tv}, beg {b} {
    } 

    Execution(
      TaskView tv,
      std::chrono::time_point<std::chrono::steady_clock> b,
      std::chrono::time_point<std::chrono::steady_clock> e
    ) :
      task_view {tv}, beg {b}, end {e} {
    }
  };
  
  // data structure to store the entire execution timeline
  struct Timeline {
    std::chrono::time_point<std::chrono::steady_clock> origin;
    std::vector<std::vector<Execution>> executions;
  };  

  public:
    
    /**
    @brief dump the timelines in JSON format to an ostream
    @param ostream the target std::ostream to dump
    */
     void dump(std::ostream& ostream) const;

    /**
    @brief dump the timelines in JSON to a std::string
    @return a JSON string 
    */
     std::string dump() const;
    
    /**
    @brief clear the timeline data
    */
     void clear();

    /**
    @brief get the number of total tasks in the observer
    @return number of total tasks
    */
     size_t num_tasks() const;

  private:
    
     void set_up(unsigned num_workers) override final;
     void on_entry(unsigned worker_id, TaskView task_view) override final;
     void on_exit(unsigned worker_id, TaskView task_view) override final;

    Timeline _timeline;
};  


}  // end of namespace tf -------------------------------------------


