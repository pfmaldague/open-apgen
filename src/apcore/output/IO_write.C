#if HAVE_CONFIG_H
#include <config.h>
#endif

#include	<sstream>

#include	"ActivityInstance.H"
#include	"ACT_exec.H"
#include	"action_request.H"	// for INCLUDE_AS_CODE etc.
// #include	"apcoreWaiter.H"	// for APcloptions::output_chunky_xml
#include	"apDEBUG.H"
#include	"Constraint.H"
#include	"IO_ApgenDataOptions.H"
#include	"IO_write.H"
#include	"Legends.H"		// for ACTVERUNIT etc.
#include	"Multiterator.H"
#include	"RES_def.H"		// for evaluation strategy
#include	"RES_exec.H"
#include	"EventImpl.H"		// for predEventPLD
#include	"UTL_defs.H"
#include	"aafReader.H"

#ifdef have_xmltol
#	include <libxml++/libxml++.h>   // for generating XML TOL's
#endif /* have_xmltol */

#include <assert.h>

#include "IO_ApgenData.H"
#include "IO_SASFWriteImpl.H"

using namespace std;

#include <string.h>
#include <sys/param.h>


#include <unistd.h>

// below:
void		    add_missing_file_names(stringslist& l);

IO_writer::IO_writer () {
	; }

IO_writer::~IO_writer () {
	; }

void IO_writer::writeFunctions(aoString &funcStream, long theSectionNode) {
#ifdef PREMATURE
	if(eval_intfc::AAF_defined_functions().get_length()) {
		List_iterator	funcs(eval_intfc::AAF_defined_functions());
		AAF_function	*F;
		char		*func_text;

		while((F = (AAF_function *) funcs())) {
			F->transfer_to_stream(funcStream);
		}
	}
	if(eval_intfc::AAF_defined_scripts().get_length()) {
		List_iterator	funcs(eval_intfc::AAF_defined_scripts());
		AAF_function	*F;
		char		*func_text;

		while((F = (AAF_function *) funcs())) {
			F->transfer_to_stream(funcStream);
		}
	}
#endif /* PREMATURE */
}

void IO_writer::writeActivityTypes(aoString &activityStream, long theSectionNode) {
	ACT_exec::ACT_subsystem().TransferAllActivityTypes(activityStream, theSectionNode);
}

void IO_writer::writeConstraints(aoString &constraintStream, long theSectionNode) {
	// CON_exec::theConstraintSubsystem().TransferAllConstraints(constraintStream, theSectionNode);
}

apgen::RETURN_STATUS
IO_writer::writeXML(    const Cstring&		  planFilename,
			stringtlist&		    list_of_files,
			const List&		     Legs,
			Action_request::save_option     globs,
			Action_request::save_option     epchs,
			Action_request::save_option     Tmsysss,
			Action_request::save_option     lged,
			Action_request::save_option     tmarametrs,
			Action_request::save_option     magnitudeOFwindozes,
			long			    registeredFunctions,
			int			     formats,
			Cstring&			errors) {
	errors = "writing plan files in XML format is no longer supported, due to lack of agreement on a good XML specification. "
		"Use \"Generate XMLTOL\" instead. ";
	return apgen::RETURN_STATUS::FAIL; }

apgen::RETURN_STATUS IO_writer::writeAPF(
			const Cstring&		  planFilename,
			stringslist&		    list_of_files,
			const List&		     Legs,
			Action_request::save_option     globs,
			Action_request::save_option     epchs,
			Action_request::save_option     Tmsysss,
			Action_request::save_option     lged,
			Action_request::save_option     tmarametrs,
			Action_request::save_option     magnitudeOFwindozes,
			long			    registeredFunctions,
			int			     formats,
			Cstring&			errors) {
	aoString	as;
	slist<alpha_void, dumb_actptr> bad_activities;
	int	     dum_prio = 0;
	status_aware_multiterator* miter = eval_intfc::get_instance_multiterator(dum_prio, true, true);
	CTime_base      startTime, endTime;
	int	     success;
	ActivityInstance* req = NULL;

		FILE*	fout = fopen(*planFilename, "w");

		if (!fout) { // problem opening file
			Cstring msg("Unable to open file \"");
	 
			msg << planFilename << "\"";
			ACT_exec::displayMessage("WRITE APF FILE", "FILE I/O ERROR", (char *) *msg);
			delete miter;
			return apgen::RETURN_STATUS::FAIL;
		}

		// Constructor is in IO_ApgenDataOptions.C. All it does is store the data passed to it.
		IO_APFWriteOptions options(     as,
						globs,
						epchs,
						Tmsysss,
						lged,
						tmarametrs,
						magnitudeOFwindozes,
						registeredFunctions,
						formats);

		add_missing_file_names(list_of_files);
		ActivityInstance::we_are_writing_an_SASF() = false;

		// debug

		// cerr << "writeAPF: list of files\n";

		// for(emptySymbol* NN = list_of_files.first_node(); NN; NN = NN->next_node()) {
		//     cerr << "\t" << NN->get_key() << "\n";
		// }

		success = ACT_exec::WriteDataToStream(  as,
							list_of_files,
							Legs,
							bad_activities,
							&options);

		as.write_to(fout);
		fclose(fout);

	/*for Steve Wissler -- rename 'New' activities so they are from the file we just saved*/
	model_intfc::FirstEvent(startTime);
	model_intfc::LastEvent(endTime);
	endTime = endTime + CTime_base(1, 0, true);

	while((req = (ActivityInstance *) miter->next())) {
		Cstring reqPlan = req->get_APFplanname();

		if(list_of_files.find(reqPlan)) {
			if(     (req->getetime() >= startTime)
				&& (req->getetime() < endTime)) {
				Cstring planname = req->get_APFplanname();

				if(planname == "New") {
					req->set_APFplanname(planFilename);
				}
			}
		} else if(!eval_intfc::ListOfAllFileNames().find(reqPlan)) {
			eval_intfc::ListOfAllFileNames() << new emptySymbol(reqPlan);
		}
	}
	delete miter;
	if(!eval_intfc::ListOfAllFileNames().find(planFilename)) {
		eval_intfc::ListOfAllFileNames() << new emptySymbol(planFilename);
	}

	if(!success) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void add_missing_file_names(stringslist& l) {
    int                             dum_prio = 0;
    status_aware_multiterator*      miter = eval_intfc::get_instance_multiterator(dum_prio, true, true);
    ActivityInstance*               req = NULL;

    while((req = miter->next())) {
	Cstring reqPlan = req->get_APFplanname();

	if(!eval_intfc::ListOfAllFileNames().find(reqPlan)) {
	    eval_intfc::ListOfAllFileNames() << new emptySymbol(reqPlan);
	    l << new emptySymbol(reqPlan);
	}
    }
    delete miter;
}

apgen::RETURN_STATUS
IO_writer::writeAPFtoStream(
			aoString&		       outStream,
			Action_request::save_option     incl_globs,	     // incl parameter conventions:
			Action_request::save_option     incl_epochs,	    //      0 - do not include
			Action_request::save_option     incl_time_systems,      //      1 - include as comments
			Action_request::save_option     incl_legends,	   //      2 - include as "code"
			Cstring&			errors) {
	slist<alpha_void, dumb_actptr>  bad_activities;
	int			     success;
	stringtlist		     list_of_files(eval_intfc::ListOfAllFileNames());
	List			    legends_to_save(Dsource::theLegends());

	// Constructor is in IO_ApgenDataOptions.C. All it does is store the data passed to it.
	IO_APFWriteOptions options(				     outStream,
									incl_globs,
									incl_epochs,
									incl_time_systems,
									incl_legends,
					/* tmarametrs */		Action_request::INCLUDE_NOT_AT_ALL,
					/* magnitudeOFwindozes */       Action_request::INCLUDE_NOT_AT_ALL,
					/* registeredFunctions */       0,
					/* formats */		   0);

	add_missing_file_names(list_of_files);
	ActivityInstance::we_are_writing_an_SASF() = false;
	success = ACT_exec::WriteDataToStream(  outStream,
						list_of_files,
						legends_to_save,
						bad_activities,
						&options);

	if(!success) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
IO_writer::writeLAYOUT( const Cstring   &planFilename,
			const Cstring   &layoutTag) {
	aoString	as;
	FILE	    *fout = fopen(*planFilename, "w");

	if (!fout) { // problem opening file
		return apgen::RETURN_STATUS::FAIL; }
	as << "APGEN version \"" << get_apgen_version() << "\"\n\n\n" ;
	ACT_exec::WriteLegendLayoutToStream(as, layoutTag);
	as.write_to(fout);
	fclose(fout);
	return apgen::RETURN_STATUS::SUCCESS; }



void IO_writer::writeJSON(
		FILE*		        fout,
		CTime_base	        start,
		CTime_base	        end,
		const vector<string>&   resources) {
	stringslist	result;
	Cstring		any_errors;

	write_res_hist_to_Json_strings(
		/* singleFile = */ true,
		/* namespace = */ "std",
		result,
		any_errors);
	if(any_errors.length()) {
		throw(eval_error(any_errors));
	}
	stringslist::iterator iter(result);
	emptySymbol* es;
	while((es = iter())) {
		fwrite(*es->get_key(), es->get_key().length(), 1, fout);
	}
}

char* IO_writer::get_current_gmtime() {
	static char buffer[100];
	struct tm* ptm;
	struct timezone tmp_zone;

	gettimeofday(&current_time, &tmp_zone);
	ptm = gmtime((time_t *)&current_time.tv_sec);
	sprintf(buffer, "%d-%03dT%02d:%02d:%02d",
		ptm->tm_year + 1900,
		ptm->tm_yday + 1,
		ptm->tm_hour,
		ptm->tm_min,
		ptm->tm_sec);
	return buffer;
}

IO_SASFWriteOptions::IO_SASFWriteOptions(
			const string& desiredFileName,
			const StringVect& symbolicFileNames,
			const CTime_base& start,
			const CTime_base& end,
			int inclusionFlag)
	: DesiredFileName(desiredFileName), 
		SymbolicFileNames(symbolicFileNames),
		Start(start), 
		End(end),
		InclusionFlag(inclusionFlag) {}

//
// resurrected
//
void
IO_writer::writeSASF2(
		const Cstring&			desiredSasfFilename,
		const stringslist&		theListOfSymbolicSasfFiles, 
		CTime_base			theStart,
		CTime_base			theEnd, 
		int				theInclusionFlag) {

	//
	// The design of this capability is due to Adam Chase, who
	// duplicated a lot of the ActivityInstance design using
	// STL containers and their throwaway iterators. He created
	// a bridge between ActivityInstance and a "subsystem"
	// implemented as the IO_SASFWriteImpl class (an instance
	// of an ambitious class template involving even more
	// duplication.) The task of creating an SASF consists of
	// three steps:
	//
	// 	- prepare the receiving end (the "client") by
	// 	  defining the writeSASF parameters as globals
	//
	// 	- send the ActivityInstance data via the bridge
	//
	// 	- actually create the output file on the
	// 	  IO_SASFWrite side of the bridge

	//
	// Step 1: preliminaries - define global parameters for the
	// use of IO_SASFWrite
	//
	emptySymbol*	thePointerToTheSymbolicFileName;
	stringslist::iterator   symFiles(theListOfSymbolicSasfFiles);
	StringVect	symbolicFileNames;

	while((thePointerToTheSymbolicFileName = symFiles())) {
		Cstring theSymbolicSasfFile = thePointerToTheSymbolicFileName->get_key();
		symbolicFileNames.push_back(*theSymbolicSasfFile);
	}
	CTime_base evStart(theStart);
	CTime_base evEnd(theEnd);
	IO_SASFWriteOptions options(
			*desiredSasfFilename,
			symbolicFileNames,
			theStart,
			theEnd,
			theInclusionFlag);

	//
	// Step 2: export activities to IO_SASFWrite using
	// client-server nonsense.
	//

	//
	// set up "client" - just a receiving structure on
	// the IO_SASFWrite side
	//
	IO_SASFWriteImpl::SASFwriter().clear();

	//
	// Do the real work here: extract all activities
	// and send them in the IO_SASFWrite format, which
	// duplicates a lot of the obvious stuff in
	// ActivityInstance
	//
	ActivityInstance::we_are_writing_an_SASF() = true;

	//
	// throws if problems:
	//
	ExportSASFData(
			eval_intfc::ListOfAllFileNames(),
			Dsource::theLegends());

	//
	// Step 3: write the actual SASF by scanning the
	// "Plan" created on the IO_SASFWrite side of the
	// bridge.
	//
	// This method throws eval_error if problems are found.
	// It also throws an error if the SASF contains no steps,
	// in which case no file was written.
	//
	try {
		IO_SASFWriteImpl::SASFwriter().WriteSASF(options);
	} catch(eval_error Err) {
		if(Err.msg == "The SASF contains no steps\n") {
			cerr << "WriteSASF: WARNING - no steps found, no file witten.\n";
		} else {
			throw(Err);
		}
	}

	ActivityInstance::we_are_writing_an_SASF() = false;
}

void
IO_writer::ExportSASFData(     
		const stringslist& list_of_files,
		const List&	   legends) {
	slist<alpha_void, dumb_actptr>  bad_activities;
	Cstring	errs;

	//
	// The work is done in ACT_exec.C:
	//
	if(!ACT_exec::SendDataToClient(
			list_of_files,
			legends,
			bad_activities,
			errs)) {
		throw(eval_error(errs));
	}
}

// used by the generate SASF GUI panel:
bool
IO_writer::create_sasf_file_list(StringVect *fileNames, Cstring &errs) {

	IO_SASFWriteImpl::SASFwriter().clear();

	slist<alpha_void, dumb_actptr>  bad_ones;

	//get data to sasfWriter
	int success = ACT_exec::SendDataToClient(bad_ones, errs);
	if(!success) {
		// debug
		// cerr << "IO_writer::create_sasf_file_list: error in ACT_exec::SendDataToClient; "
		//      " details:\n" << errs << endl;
		return false;
	}
	IO_SASFWriteImpl::SASFwriter().GetAvailableSymbolicFiles(fileNames);
	return true;
}

apgen::RETURN_STATUS
IO_writer::write_globals_to_Json_strings(
		map<string, string>& result,
		Cstring& any_errors) {
	return ACT_exec::WriteGlobalsToJsonStrings(result, any_errors);
}

apgen::RETURN_STATUS
IO_writer::write_act_interactions_to_Json_strings(
		bool		singleFile,
		const Cstring&	Filename,
		stringslist&	result,
		Cstring&	any_errors) {
	ACT_exec::WriteActInteractionsToJsonStrings(
				singleFile,
				Filename,
				result,
				any_errors);
	if(!result.get_length()) {
		any_errors = "write act interactions: no data\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	FILE*	fout = fopen(*Filename, "w");
	if(!fout) {
		any_errors = "write act interactions: cannot open file ";
		any_errors << Filename;
		return apgen::RETURN_STATUS::FAIL;
	}
	const char* s = *result.first_node()->get_key();
	fwrite(s, strlen(s), 1, fout);
	fclose(fout);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
IO_writer::write_res_defs_to_Json_string(
		const Cstring& Namespace,
		Cstring& result,
		Cstring& any_errors) {
	return ACT_exec::WriteResourceDefsToJsonString(
				Namespace,
				result,
				any_errors);
}

apgen::RETURN_STATUS
IO_writer::write_res_hist_to_Json_strings(
		bool singleFile,
		const Cstring& Namespace,
		stringslist& result,
		Cstring& any_errors) {
	return ACT_exec::WriteResourcesToJsonStrings(
				singleFile,
				Namespace,
				result,
				any_errors);
}

apgen::RETURN_STATUS
IO_writer::write_Json_string_to_TMS(
			const Cstring&  Json,
			const Cstring&  URLserver,
			const Cstring&  URLport,
			const Cstring&  URLscn,
			const Cstring&  URLschema,
			Cstring&	errors) {
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
IO_writer::write_Json_string_to_TMS_meta(
			const Cstring&  Json,
			const Cstring&  URLserver,
			const Cstring&  URLport,
			const Cstring&  URLscn,
			const Cstring&  URLschema,
			Cstring&	errors) {
	return apgen::RETURN_STATUS::SUCCESS;
}
