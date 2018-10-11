
// For getting thread ID in callbacks 
typedef struct my_ompt_data{
  uint64_t own;
  ompt_data_t client;
} my_ompt_data_t;

uint64_t get_own_data(ompt_data_t* data)
{
  return ((my_ompt_data_t*)data->ptr)->own;
}

uint64_t alloc_and_init_own_data(ompt_data_t* data, uint64_t init_data)
{
  data->ptr = malloc(sizeof(my_ompt_data_t));
  ((my_ompt_data_t*)data->ptr)->own = init_data;
  return init_data; 
}

static uint64_t my_next_id() {
  static uint64_t ID=0;
  uint64_t ret = __sync_fetch_and_add(&ID, 1); 
  return ret; 
}


static void
on_ompt_callback_thread_begin(
  ompt_thread_type_t thread_type,
  ompt_data_t *thread_data)
{
  n_thread_begin++;
  uint64_t tid = alloc_and_init_own_data(thread_data, my_next_id());
#ifdef DEBUG
  printf("Begin thread %lu\n", tid);
#endif 
}

static void
on_ompt_callback_thread_end(
  ompt_data_t *thread_data)
{
  n_thread_end++; 
  uint64_t tid = get_own_data(ompt_get_thread_data());
#ifdef DEBUG
  printf("End thread %lu\n", tid);
#endif 
}

static void
on_ompt_callback_parallel_begin(
  ompt_data_t *encountering_task_data,
  const omp_frame_t *encountering_task_frame,
  ompt_data_t* parallel_data,
  uint32_t requested_team_size,
  ompt_invoker_t invoker,
  const void *codeptr_ra)
{
  n_parallel_begin++;
#ifdef DEBUG
  printf("\nENTERING PARALLEL_BEGIN\n"); 
  if(parallel_data->ptr) {
    printf("0: parallel_data initially not null\n");
  }
#endif 
}

static void
on_ompt_callback_parallel_end(
  ompt_data_t *parallel_data,
  ompt_data_t *encountering_task_data,
  ompt_invoker_t invoker,
  const void *codeptr_ra)
{
  n_parallel_end++; 
#ifdef DEBUG
  printf("\nENTERING PARALLEL_END\n"); 
#endif
}

static void
on_ompt_callback_task_create(
    ompt_data_t *encountering_task_data,
    const omp_frame_t *encountering_task_frame,
    ompt_data_t* new_task_data,
    int type,
    int has_dependences,
    const void *codeptr_ra)
{
  n_explicit_task++; 
}

static void
on_ompt_callback_implicit_task(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    unsigned int team_size,
    unsigned int thread_num)
{
  if (endpoint == ompt_scope_begin) {
    n_implicit_task_created++;
  }
  else if (endpoint == ompt_scope_end) {
    n_implicit_task_completed++;
  }
  else {
#ifdef DEBUG
    printf("Endpoint not equal to ompt_scope_begin or ompt_scope_end "
           "encountered in implicit_task callback\n"); 
#endif 
  } 
}

static void
on_ompt_callback_task_dependences(
  ompt_data_t *task_data,
  const ompt_task_dependence_t *deps,
  int ndeps)
{
  n_task_dependences++;
}

static void
on_ompt_callback_task_dependence(
  ompt_data_t *first_task_data,
  ompt_data_t *second_task_data)
{
  n_task_dependence++; 
}

static void
on_ompt_callback_task_schedule(
    ompt_data_t *first_task_data,
    ompt_task_status_t prior_task_status,
    ompt_data_t *second_task_data)
{
  // Got error when using "ompt_task_switch" instead of "4" (See pg. 440 of TR7) 
  // Using "ompt_task_others" works though (see pg. 400 of TR4)
  if (prior_task_status == ompt_task_others) { 
    n_task_schedule_others++;
  }
  else if (prior_task_status == ompt_task_cancel) {
    n_task_schedule_cancel++;
  } 
  else if (prior_task_status == ompt_task_yield) {
    n_task_schedule_yield++;
  }
  else if (prior_task_status == ompt_task_complete) {
    n_task_schedule_complete++;
  }
  else {
#ifdef DEBUG
    printf("\t- Prior task status unrecognized: %u\n", prior_task_status); 
#endif
  }
}

static void
on_ompt_callback_sync_region(
  ompt_sync_region_kind_t kind,
  ompt_scope_endpoint_t endpoint,
  ompt_data_t *parallel_data,
  ompt_data_t *task_data,
  const void *codeptr_ra)
{
  n_task_sync_region++;
  switch(endpoint)
  {
    case ompt_scope_begin:
      switch(kind)
      {
        case ompt_sync_region_barrier:
          n_task_sync_region_begin_barrier++;
          break;
        case ompt_sync_region_taskwait:
          n_task_sync_region_begin_taskwait++;
          break;
        case ompt_sync_region_taskgroup:
          n_task_sync_region_begin_taskgroup++;
          break; 
      }
      break;
    case ompt_scope_end:
      switch(kind) 
      {
        case ompt_sync_region_barrier:
          n_task_sync_region_end_barrier++;
          break;
        case ompt_sync_region_taskwait:
          n_task_sync_region_end_taskwait++;
          break;
        case ompt_sync_region_taskgroup:
          n_task_sync_region_end_taskgroup++;
          break; 
      }
      break;
  }
}


static void
on_ompt_callback_sync_region_wait(
  ompt_sync_region_kind_t kind,
  ompt_scope_endpoint_t endpoint,
  ompt_data_t *parallel_data,
  ompt_data_t *task_data,
  const void *codeptr_ra)
{
  n_task_sync_region_wait++;
  switch(endpoint)
  {
    case ompt_scope_begin:
      switch(kind)
      {
        case ompt_sync_region_barrier:
          n_task_sync_region_wait_begin_barrier++;
          break;
        case ompt_sync_region_taskwait:
          n_task_sync_region_wait_begin_taskwait++;
          break;
        case ompt_sync_region_taskgroup:
          n_task_sync_region_wait_begin_taskgroup++;
          break; 
      }
      break;
    case ompt_scope_end:
      switch(kind)
      {
        case ompt_sync_region_barrier:
          n_task_sync_region_wait_end_barrier++;
          break;
        case ompt_sync_region_taskwait:
          n_task_sync_region_wait_end_taskwait++;
          break;
        case ompt_sync_region_taskgroup:
          n_task_sync_region_wait_end_taskgroup++;
          break; 
      }
      break;
  }
}

