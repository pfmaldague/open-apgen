#ifndef IOWRITEH
#define IOWRITEH

#include <sys/time.h>
#include <time.h>
#ifdef GUI
#	include <Xm/Xm.h>
#endif

#include "APdata.H"
#include "UTL_stl.H"
#include "UTL_time.H"


enum IOWriteDataType {IOWDT_LEGEND,IOWDT_ACTIVITYINS};

class IO_writer {
protected:
	apgen::FileType	fileType;

	Cstring		outputFileName;	///used by some fileType's
	int		timeFormat;
	bool		displayLegend;
	bool		displayDesc;
	Cstring		startTimeString;
	Cstring		durationString;
        static void writeActivities(
				List &list_of_files,
				aoString &fout,
				CTime_base start,
				CTime_base end);
        static void writeActivityTypes(aoString &fout, long top_level_chunk = 0);
        static void writeConstraints(aoString &fout, long top_level_chunk = 0);

        static void writeLegends (	const List&	list_of_legends,
					aoString&	fout,
					int		comments_or_code);
        static void writeEpochs(	aoString&	fout,
					int		comments_or_code);
        static void writeTimeSystems(	aoString&	fout,
					int		comments_or_code);
        static void writeGlobals(	aoString&	fout,
					int		comments_or_code);
        static void writeFunctions(	aoString&	fout,
					long		top_level_chunk = 0);
public:
	struct timeval current_time;

	typedef enum _TOL_STYLE {
		regular,
		xml } TOL_STYLE;

	IO_writer();
	~IO_writer();

	static apgen::RETURN_STATUS	write_act_interactions_to_Json_strings(
						bool		singleFile,
						const Cstring&	Filename,
						stringslist&	result,
						Cstring&	errors);
	static apgen::RETURN_STATUS	write_res_defs_to_Json_string(
						const Cstring&	Namespace,
						Cstring&	result,
						Cstring&	errors);
	static apgen::RETURN_STATUS	write_res_hist_to_Json_strings(
						bool		singleFile,
						const Cstring&	Namespace,
						stringslist&	result,
						Cstring&	errors);

	static apgen::RETURN_STATUS	write_globals_to_Json_strings(
						map<string, string>&	output_stream,
						Cstring&		errors);

	static apgen::RETURN_STATUS	write_Json_string_to_TMS(
						const Cstring&	Json,
						const Cstring&	URLserver,
						const Cstring&	URLport,
						const Cstring&	URLscn,
						const Cstring&	URLschema,
						Cstring&	errors);
	static apgen::RETURN_STATUS	write_Json_string_to_TMS_meta(
						const Cstring&	Json,
						const Cstring&	URLserver,
						const Cstring&	URLport,
						const Cstring&	URLscn,
						const Cstring&	URLschema,
						Cstring&	errors);

	static apgen::RETURN_STATUS	writeXML(
						const Cstring&			plan_filename,
						stringtlist&			list_of_planfiles,
						const List&			legends, 
						Action_request::save_option	globs,
						Action_request::save_option	epchs,
						Action_request::save_option	Tmsysss,
						Action_request::save_option	lged,
						Action_request::save_option	tmarametrs,
						Action_request::save_option	magnitudeOFwindozes,
						long				registeredFunctions,
						int				formats, // ignored
						Cstring&			any_errors);

				/* Method to write a plan file; used by atm_client.
				 * If the apcoreWaiter::output_chunky_xml flag is set,
				 * a few tags will be inserted in the output, in a way
				 * consistent with the SEQRP database schema. */
	static apgen::RETURN_STATUS	writeAPF(
						const Cstring&			plan_filename,
						stringslist&			list_of_planfiles,
						const List&			legends,
						Action_request::save_option	globs,
						Action_request::save_option	epchs,
						Action_request::save_option	Tmsysss,
						Action_request::save_option	lged,
						Action_request::save_option	tmarametrs,
						Action_request::save_option	magnitudeOFwindozes,
						long				registeredFunctions,
						int             		formats,	
						Cstring&			errors);

				/* Simplified version of writeAPF which sends the plan to a stream
				 * instead of a file. Used by apcore_server. */
	static apgen::RETURN_STATUS	writeAPFtoStream(
						aoString&			theStream,
						Action_request::save_option	incl_globals,
						Action_request::save_option	incl_epochs,
						Action_request::save_option	incl_time_systems,
						Action_request::save_option	incl_legends,
						Cstring&			errors);

				/* To support saving a file containing the legend layout
				 * (both activity and resource legends.) */
	apgen::RETURN_STATUS	writeLAYOUT(	const Cstring&			plan_filename,
						const Cstring&			layout_tag);

	//
	// throws eval_error if problem:
	//
        void             	ExportSASFData(	const stringslist&		list_of_files,
						const List&			legends);

	//
	// Used for both TOLs and XMLTOLs; works both in
	// single- and multi-threaded environments;
	// throws eval_error if problem:
	//
        static void		writeTOL(	const Cstring&		file_name,
						const Cstring&		xmltolfilename,
						CTime_base		start,
						CTime_base		end,
						bool			do_tol,
						bool			do_xmltol,
						const Cstring&		optional_timesystem,
						bool			all_acts_visible,
						const vector<string>&	resources,
						thread_intfc*		T);
        void			writeJSON(	FILE*			f,
						CTime_base		start,
						CTime_base		end,
						const vector<string>&	resources);

	//
	// throws eval_error if problem:
	//
        void			writeSASF2(
			const Cstring&		desiredSASFFilename,
			const stringslist&	theListOfSymbolicSASFFilenames,
			CTime_base		theStart,
			CTime_base		theEnd,
			int whatToDoWhenTheStartTimeOfAnActivityCoincidesWithTheEndPointOfTheTimeInterval);

	static bool		create_sasf_file_list(
						StringVect*	fileName,
						Cstring&	allErrors);

        char*			get_current_gmtime(); };



#endif
