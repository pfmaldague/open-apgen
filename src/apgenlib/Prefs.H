#ifndef PREFS_H
#define PREFS_H

#include <string>
using std::string;

/**a class for storing preferences.
Note: prefNames and PrefValues may not contain '=' or '\n'

preference conventions:
boolean preferences should be "TRUE" or "FALSE"
preference Names should be MixedCase (no underscores).
preference Values other than type boolean should be mixed case.
*/
class Prefs
{
public:

  ///returns 1 if there was a user preference file
  virtual int Init() = 0;

  ///always sets the preference, returns 1 if preference already was set
  virtual int SetPreference(const string& prefName, const string& prefVal) = 0;

  ///returns 1 if the preference is there
  virtual int GetPreference(const string& prefName, string* prefVal) const = 0;

  ///if there is not a preference set, it will set it to default, the result is returned
  virtual string GetSetPreference(const string& prefName, const string& defaultValue) = 0;

  ///returns 1 on success
  virtual int SavePreferences() = 0;

};

///A function to get a handle to the preferences
Prefs& Preferences();

//void TestPrefs();



#endif
