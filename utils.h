#ifndef UTILS_H_SENTRY
#define UTILS_H_SENTRY

#include "runner.h"

void set_sig_ign (void);
void set_sig_dfl (void);
void print_prompt1 (void);
void print_prompt2 (void);

process *pipeline_item_to_process (cmd_pipeline_item *simple_cmd);
job *make_job ();
void destroy_job (job *j);
void register_job (shell_info *sinfo, job *j);
void unregister_job (shell_info *sinfo, job *j);
int job_is_stopped (job *j);
int job_is_completed (job *j);
void mark_job_as_runned (job *j);

#endif
