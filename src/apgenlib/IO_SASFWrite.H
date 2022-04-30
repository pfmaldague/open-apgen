#ifndef IO_SASF_WRITE_H
#define IO_SASF_WRITE_H

#include "IO_ApgenData.H"

class IO_SASFWriteOptions;

//
// An IO_ApgenExportClient that writes SASFs.
// This class can be passed to ExportData in IO_writer 
// and then you can either GetAvailableSymbolicFiles
// or WriteSASF.
//
class IO_SASFWrite
{
public:
  // to create
  static IO_SASFWrite* CreateSASFWrite();

  // used by export Data interface
  virtual long GetClientName(string* nameString) const = 0;
  virtual long AddOneGlobal(const string& name, const TypedValue& value) = 0;
  virtual long AddOneEpoch(const IO_Epoch& value) = 0;
  virtual long AddOneActivity(const IO_ActivityInstance& instance) = 0;

  //
  // Writes the sasf. Throws eval_error if errors found.
  //
  virtual void WriteSASF(IO_SASFWriteOptions& options) = 0;
    
  // Returns the Available Symbolic Files
  virtual long GetAvailableSymbolicFiles(StringVect* vect) const = 0;
    
};




#endif
