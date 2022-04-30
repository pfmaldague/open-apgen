#ifndef _PROCESS_STUFF_H
#define _PROCESS_STUFF_H

#include <stdio.h>

void cd_process_directive();
void cd_process_directive_expression();
void cd_process_directive_header();
void cd_process_epoch_end();
void cd_process_epoch_header();
void cd_process_footer();
void cd_process_global_header();
void cd_process_id(char *c, int indentation, int append_equal_flag);
void cd_process_newline();
void cd_process_parameter_type(char *c, int indentation, FILE *F);
void cd_process_seq_header(char *c);
void cd_process_timesystem_header();

#endif /* _PROCESS_STUFF_H */
