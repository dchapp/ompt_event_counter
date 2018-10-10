#include <iostream>
#include <vector> 
#include <unordered_map>
#include <csignal> // Need signal handling to dump DAG on interrupt
#include <inttypes.h>
#include <stdio.h>
#include <atomic> 

#include "omp.h"
#include "ompt.h"

/******************************************************************************\
 * Definitions for tool configuration
\******************************************************************************/

#include "OMPT_helpers.hpp" 
//#define DEBUG

/******************************************************************************\
 * Some counters for tracking how many callbacks of each kind are encountered
\******************************************************************************/
std::atomic<uint64_t> n_thread_begin;
std::atomic<uint64_t> n_thread_end;
std::atomic<uint64_t> n_parallel_begin;
std::atomic<uint64_t> n_parallel_end;
std::atomic<uint64_t> n_implicit_task_created;
std::atomic<uint64_t> n_implicit_task_completed;
std::atomic<uint64_t> n_explicit_task;
std::atomic<uint64_t> n_task_dependences;
std::atomic<uint64_t> n_task_dependence;
std::atomic<uint64_t> n_task_schedule_others;
std::atomic<uint64_t> n_task_schedule_cancel;
std::atomic<uint64_t> n_task_schedule_yield;
std::atomic<uint64_t> n_task_schedule_complete;
std::atomic<uint64_t> n_task_sync_region;
std::atomic<uint64_t> n_task_sync_region_begin_barrier;
std::atomic<uint64_t> n_task_sync_region_begin_taskwait;
std::atomic<uint64_t> n_task_sync_region_begin_taskgroup;
std::atomic<uint64_t> n_task_sync_region_end_barrier;
std::atomic<uint64_t> n_task_sync_region_end_taskwait;
std::atomic<uint64_t> n_task_sync_region_end_taskgroup;
std::atomic<uint64_t> n_task_sync_region_wait;
std::atomic<uint64_t> n_task_sync_region_wait_begin_barrier;
std::atomic<uint64_t> n_task_sync_region_wait_begin_taskwait;
std::atomic<uint64_t> n_task_sync_region_wait_begin_taskgroup;
std::atomic<uint64_t> n_task_sync_region_wait_end_barrier;
std::atomic<uint64_t> n_task_sync_region_wait_end_taskwait;
std::atomic<uint64_t> n_task_sync_region_wait_end_taskgroup;

#include "callbacks.hpp" 


void print_event_counts() {
  printf("Number of thread begin: %lu\n", 
         (uint64_t)n_thread_begin);
  printf("Number of thread end: %lu\n", 
         (uint64_t)n_thread_end);
  printf("Number of parallel begin: %lu\n", 
         (uint64_t)n_parallel_begin);
  printf("Number of parallel end: %lu\n", 
         (uint64_t)n_parallel_end);
  printf("Number of explicit task creation: %lu\n", 
         (uint64_t)n_explicit_task);
  printf("Number of implicit task creation: %lu\n", 
         (uint64_t)n_implicit_task_created);
  printf("Number of implicit task completion: %lu\n", 
         (uint64_t)n_implicit_task_completed);
  printf("Number of task dependences: %lu\n", 
         (uint64_t)n_task_dependences);
  printf("Number of task dependence: %lu\n", 
         (uint64_t)n_task_dependence);
  printf("Number of task schedule (others): %lu\n", 
         (uint64_t)n_task_schedule_others);
  printf("Number of task schedule (cancel): %lu\n", 
         (uint64_t)n_task_schedule_cancel);
  printf("Number of task schedule (yield): %lu\n", 
         (uint64_t)n_task_schedule_yield);
  printf("Number of task schedule (complete): %lu\n", 
         (uint64_t)n_task_schedule_complete);
  printf("Number of task sync region begin barrier: %lu\n", 
         (uint64_t)n_task_sync_region_begin_barrier);
  printf("Number of task sync region begin taskwait: %lu\n", 
         (uint64_t)n_task_sync_region_begin_taskwait);
  printf("Number of task sync region begin taskgroup: %lu\n", 
         (uint64_t)n_task_sync_region_begin_taskgroup);
  printf("Number of task sync region end barrier: %lu\n", 
         (uint64_t)n_task_sync_region_end_barrier);
  printf("Number of task sync region end taskwait: %lu\n", 
         (uint64_t)n_task_sync_region_end_taskwait);
  printf("Number of task sync region end taskgroup: %lu\n", 
         (uint64_t)n_task_sync_region_end_taskgroup);
  printf("Number of task sync region wait begin barrier: %lu\n", 
         (uint64_t)n_task_sync_region_wait_begin_barrier);
  printf("Number of task sync region wait begin taskwait: %lu\n", 
         (uint64_t)n_task_sync_region_wait_begin_taskwait);
  printf("Number of task sync region wait begin taskgroup: %lu\n", 
         (uint64_t)n_task_sync_region_wait_begin_taskgroup);
  printf("Number of task sync region wait end barrier: %lu\n", 
         (uint64_t)n_task_sync_region_wait_end_barrier);
  printf("Number of task sync region wait end taskwait: %lu\n", 
         (uint64_t)n_task_sync_region_wait_end_taskwait);
  printf("Number of task sync region wait end taskgroup: %lu\n", 
         (uint64_t)n_task_sync_region_wait_end_taskgroup);
}

/******************************************************************************\
 * This function writes the event counts upon catching SIGINT 
\******************************************************************************/
void signal_handler(int signum) {
  printf("\n\nTool caught signal: %d\n", signum);
  print_event_counts(); 
  exit(signum);
}

int ompt_initialize(ompt_function_lookup_t lookup,
                    ompt_data_t * tool_data)
{

  // OMPT boilerplate 
  ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
  ompt_get_callback = (ompt_get_callback_t) lookup("ompt_get_callback");
  ompt_get_state = (ompt_get_state_t) lookup("ompt_get_state");
  ompt_get_task_info = (ompt_get_task_info_t) lookup("ompt_get_task_info");
  ompt_get_thread_data = (ompt_get_thread_data_t) lookup("ompt_get_thread_data");
  ompt_get_parallel_info = (ompt_get_parallel_info_t) lookup("ompt_get_parallel_info");
  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id");
  ompt_get_num_procs = (ompt_get_num_procs_t) lookup("ompt_get_num_procs");
  ompt_get_num_places = (ompt_get_num_places_t) lookup("ompt_get_num_places");
  ompt_get_place_proc_ids = (ompt_get_place_proc_ids_t) lookup("ompt_get_place_proc_ids");
  ompt_get_place_num = (ompt_get_place_num_t) lookup("ompt_get_place_num");
  ompt_get_partition_place_nums = (ompt_get_partition_place_nums_t) lookup("ompt_get_partition_place_nums");
  ompt_get_proc_id = (ompt_get_proc_id_t) lookup("ompt_get_proc_id");
  ompt_enumerate_states = (ompt_enumerate_states_t) lookup("ompt_enumerate_states");
  ompt_enumerate_mutex_impls = (ompt_enumerate_mutex_impls_t) lookup("ompt_enumerate_mutex_impls");

  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id"); 

  register_callback(ompt_callback_task_create);
  register_callback(ompt_callback_implicit_task);
  register_callback(ompt_callback_task_dependence);
  register_callback(ompt_callback_task_dependences);
  register_callback(ompt_callback_task_schedule);
  register_callback(ompt_callback_sync_region);
  register_callback_t(ompt_callback_sync_region_wait, ompt_callback_sync_region_t);
  register_callback(ompt_callback_parallel_begin);
  register_callback(ompt_callback_parallel_end);

  register_callback(ompt_callback_thread_begin);
  register_callback(ompt_callback_thread_end);

  // Register signal handler 
  signal(SIGINT, signal_handler); 

#ifdef DEBUG
  printf("\n\n\n");
  printf("=================================================================\n");
  printf("====================== OMPT_INITIALIZE ==========================\n");
  printf("=================================================================\n");
  printf("\n\n\n");
#endif

  // Returning 1 instead of 0 lets the OpenMP runtime know to load the tool 
  return 1;
}


void ompt_finalize(ompt_data_t *tool_data)
{
  print_event_counts();
#ifdef DEBUG
  printf("0: ompt_event_runtime_shutdown\n"); 
#endif 
}







/* "A tool indicates its interest in using the OMPT interface by providing a 
 * non-NULL pointer to an ompt_fns_t structure to an OpenMP implementation as a 
 * return value from ompt_start_tool." (OMP TR4 4.2.1)
 *
 * NOTE: This is a little confusing, is something of type 
 * "ompt_start_tool_result_t" equivalent to something of type 
 * "ompt_fns_t structure"? 
 */
ompt_start_tool_result_t* ompt_start_tool(unsigned int omp_version,
                                          const char *runtime_version)
{
  static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize,
                                                            &ompt_finalize, 
                                                            0};
  return &ompt_start_tool_result;
}
