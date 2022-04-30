#ifndef _CMD_GRAMMAR_INCL_H_
#define _CMD_GRAMMAR_INCL_H_

extern int	zzlex();
extern int	zzerror(const char*);

void clear_lists();
#ifdef OBSOLETE
void grab_and_push_text();
void process_abstract_activity();
void process_abstract_all();
void process_abstract_all_quiet();
void process_act_leg_select();
void process_act_list1();
void process_act_list2();
void process_ad_select();
void process_add_resource();
void process_add_resource_name();
void process_by_duration();
// void process_carry();
void process_close_act_display();
void process_close_act_disp_name();
void process_close_disp_name();
void process_close_res_display();
void process_copy_activity();
void process_cut_activity(	int	which);
void process_debug();
void process_delete_all_descendants();
void process_delete_legend();
void process_detail_activity(	int	which);
void process_detail_all();
void process_detail_all_quiet();
void process_disp_name();
void process_drag_children();
// void process_edit_activity(	int	which_form);
// void process_edit_assign_name();
// void process_edit_assign_value();
void process_empty_time();
void process_end_time();
void process_export_file();
void process_file_list1();
void process_file_list2();
void process_grab_id();
void process_group_activity(	int	which);
void process_horizon(		int	w1);
void process_legend();
void process_list_and_name();
void process_modifier();
void process_move_activity();
void process_move_legend();
void process_new_activity();
void process_new_activities();
void process_new_act_display();
void process_new_legend(	int	which);
void process_new_res_display();
// void process_nocarry();
void process_null_close_disp_name();
void process_null_disp_name();
void process_null_edit_assign_value();
void process_null_name();
void process_parent();
void process_paste_activity();
void process_pause(		int	which);
void process_print();
void process_purge(		int	planOnly);
void process_quit(		int	fast);
void process_redetail_activity();
void process_redetail_all();
void process_redetail_all_quiet();
void process_remodel(		int	which);
void process_remove_resource();
void process_remove_resource_name_spec();
void process_regen_children(	int	which);
void process_res_leg_select(	int	which);
void process_res_list();
void process_res_scroll(	int	which);
void process_save_partial_file();
void process_scheduling(	int	is_it_real_time,
				int	is_it_all);
void process_start_time();
void process_template();
void process_tol_format();
void process_to_time();
void process_ui_activity();
void process_ui_global();
void process_unfreeze_resources();
void process_ungroup_activity();
void process_unscheduling(	int	is_it_all);
void process_write_sasf(	int	which_type);
void process_write_tol();
void process_write_xmltol();
void process_xcmd();
void process_xmltol_filter();
void process_xmltol_timesystem();
void process_xmltol_schema();
void process_xmltol_allactsvisible(int);
#endif /* OBSOLETE */

void reset_error_flag();
void reset_exp_stack();
void tok_consolidate();

#endif /* _CMD_GRAMMAR_INCL_H_ */
