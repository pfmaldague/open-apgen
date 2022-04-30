#ifndef IO_APGEN_DATA_OPTIONS_H
#define IO_APGEN_DATA_OPTIONS_H

#include "UTL_stl.H"
#include "action_request.H"

class IO_SASFWriteOptions {
  string DesiredFileName;
  StringVect SymbolicFileNames;
  CTime_base Start;
  CTime_base End;

  int InclusionFlag;

public:
  IO_SASFWriteOptions(
		  const string& desiredFileName,
		  const StringVect& symbolicFileNames,
		  const CTime_base &start,
		  const CTime_base &end,
		  int theInclusionFlag);
  string GetFileName() const {return DesiredFileName;};
  StringVect GetSymbolicFileNames() const {return SymbolicFileNames;};
  CTime_base GetStartTime() const {return Start;};
  CTime_base GetEndTime() const {return End;};
  int GetInclusionFlag() const {return InclusionFlag;}; };


class IO_APFWriteOptions {
private:
	Action_request::save_option GlobalsOption;
	Action_request::save_option EpochsOption;
	Action_request::save_option TimeSystemsOption;
	Action_request::save_option LegendsOption;
	Action_request::save_option TimeframeOption;
	Action_request::save_option WindowSizeOption;
	int RegisteredFunctionOption;
	int FormatOption;

public:
	IO_APFWriteOptions(std::ostream& outstream,
		Action_request::save_option globalsOption,
		Action_request::save_option epochsOption,
		Action_request::save_option timeSystemsOption,
		Action_request::save_option legendsOption,
		Action_request::save_option timeParamsOption,
		Action_request::save_option windowSizeOption,
		int registeredFunctionOption,
		int formatOption);
	IO_APFWriteOptions(aoString &outstream,
		Action_request::save_option globalsOption,
		Action_request::save_option epochsOption,
		Action_request::save_option timeSystemsOption,
		Action_request::save_option legendsOption,
		Action_request::save_option timeParamsOption,
		Action_request::save_option windowSizeOption,
		int registeredFunctionOption,
		int formatOption);

#ifdef OBSOLETE
	enum Options {
		//made to match options stuff in IO_write.C
		OptionsNone = 0,
		OptionsComment = 1,
		OptionsCode = 2 };
#endif /* OBSOLETE */
	Action_request::save_option GetGlobalsOption() const;
	Action_request::save_option GetEpochsOption() const;
	Action_request::save_option GetTimeSystemsOption() const;
	Action_request::save_option GetLegendsOption() const;
	Action_request::save_option GetTimeframeOption() const;
	Action_request::save_option GetWindowSizeOption() const;
	int GetRegisteredFunctionOption() const;
	int GetFormatOption() const;
	aoString *StreamPtr;
	std::ostream &OutStream; };

#endif
