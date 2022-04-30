#ifndef _GLOB_DATA_H_
#define _GLOB_DATA_H_

#include <APdata.H>
#include <APbasic.H>
#include <json.h>

class IO_APFWriteOptions;


/* For a while we are going to maintain a dual system, with global variables
 * being defined both here - in globalData::ListOfGlobalSymbols - and in
 * Behavior::ClassTypes()[0 = Behavior::ClassIndex()["globals"]].
 *
 * The global object in which global values are stored
 * is behaving_object::GlobalObject(). This is basically
 * an interface class purely for convenience.
 */
class globalData {
public:
	globalData() {}
	~globalData() {
	}

	static void		copy_symbols(ListOVal&	in_here);

				// these functions make sense when purging and/or re-initializing
	static void		CreateReservedSymbols();
	static void		destroy();

	static TypedValue&	get_symbol(const Cstring&);

	// does not add the symbol to any symbol tables; only
	// qualifies it as reserved, epoch or time_system:
	static void		qualifySymbol(
					const Cstring&	name,
					apgen::DATA_TYPE type,
					bool		reserved = false,
					bool		is_epoch = false,
					bool		is_time_system = false
					);

	static void		get_all_timesystems(
					stringslist& l) {
		task* T = Behavior::GlobalType().tasks[0];
		for(int i = 0; i < T->get_varinfo().size(); i++) {
			if(isATimeSystem(T->get_varinfo()[i].first)) {
				l << new emptySymbol(T->get_varinfo()[i].first);
			}
		}
	}
	static void		get_all_epochs(
					stringslist& l) {
		task* T = Behavior::GlobalType().tasks[0];
		for(int i = 0; i < T->get_varinfo().size(); i++) {
			if(isAnEpoch(T->get_varinfo()[i].first)) {
				l << new emptySymbol(T->get_varinfo()[i].first);
			}
		}
	}

	static bool		isAnEpoch(const Cstring&);
	static bool		isATimeSystem(const Cstring&);
	static bool		isReserved(const Cstring&);
	static void		WriteGlobalsToStream(
					aoString &fout,
					IO_APFWriteOptions *options,
					long top_level_chunk = 0);
	static void		WriteGlobalsToJson(
					map<string, string>& fout);
	static json_object*	GetJsonGlobals();
	static void		WriteEpochsToStream(
					aoString&		fout,
					IO_APFWriteOptions*	options,
					long			top_level_chunk = 0);
	static void		WriteTimeSystemsToStream(
					aoString&		fout,
					IO_APFWriteOptions*	options,
					long			top_level_chunk = 0);
	static int		SendGlobalsToClient(
					Cstring&		any_errors);
	static int		SendEpochsToClient();
	static void		dump();

	static TypedValue&	modeling_pass();
};

#endif /* _GLOB_DATA_H_ */
