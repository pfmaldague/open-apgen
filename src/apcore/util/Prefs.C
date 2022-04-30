#include "Prefs.H"
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <stdlib.h>

typedef std::map<string, string> StringMap;

using namespace std;

class PrefsImpl : public Prefs
{
	StringMap Preferences;

	int GetPrefsFileName(string* fileName) const;

public:
	int Init();
	int SetPreference(const string& prefName, const string& prefVal);
	int GetPreference(const string& prefName, string* prefVal) const;
	string GetSetPreference(const string& prefName, const string& defaultValue);
	int SavePreferences();

};


Prefs&
Preferences()
{
	static PrefsImpl prefs;
	static bool was_initialized = false;
	if(!was_initialized) {
	was_initialized = true;
	prefs.Init(); }
	return prefs;
}

int
PrefsImpl::GetPrefsFileName(string *fileName) const
{
	char* home = getenv("HOME");

	if(home == NULL)
		return 0;

	*fileName = home;
	*fileName += "/.apgenrc";
	
	return 1;
}


int
PrefsImpl::Init()
{
	string prefsFileName;
	if(!GetPrefsFileName(&prefsFileName))
		return 0;

	//open the users .apgenrc file
	std::ifstream prefsFile(prefsFileName.c_str());

	//if not there, return 0
	if(!prefsFile)
		return 0;

	//load values into prefs
	string line;

	while(getline(prefsFile, line))
	{
		string::size_type loc = line.find('=');
		string name = line.substr(0, loc);
		string value = line.substr(loc+1);

		//rewrite any duplicates
		Preferences[name] = value;
	}

	prefsFile.close();

	return 1;
}

int
PrefsImpl::SetPreference(const string& prefName, const string& prefVal)
{
	StringMap::iterator iter = Preferences.find(prefName);

	if(iter == Preferences.end())
	{
		//not in Preferences, insert it
		Preferences.insert(make_pair(prefName, prefVal));
		return 0;
	}
	else
	{
		//re set the value to this value
		iter->second = prefVal;
		return 1;
	}
}

string
PrefsImpl::GetSetPreference(const string& prefName, const string& defaultValue)
{
	string value;
	if(GetPreference(prefName, &value))
		return value;

	SetPreference(prefName, defaultValue);
	return defaultValue;
}

int
PrefsImpl::GetPreference(const string& prefName, string* prefVal) const
{
	//try to find the preference
	StringMap::const_iterator iter = Preferences.find(prefName);

	//if its not there, return 0
	if(iter == Preferences.end())
		return 0;

	//set the value
	*prefVal = iter->second;

	return 1;
}


int
PrefsImpl::SavePreferences() {
	string prefsFileName;

	if(!GetPrefsFileName(&prefsFileName))
		return 0;
	
	stringstream tempFile;

	//iterate through the preferences
	StringMap::const_iterator iter = Preferences.begin();
	
	while(iter != Preferences.end()) {
		tempFile << iter->first << "=" << iter->second << "\n";
		iter++;
	}

	//write them to the .apgenrc file
	ofstream fileOut(prefsFileName.c_str());

	if(!fileOut)
		return 0;

	fileOut << tempFile.str();
	fileOut.close();
	
	return 1;
}


void
TestPrefs()
{
	Preferences().Init();

	string prefVal;

	if(!Preferences().GetPreference("Barry", &prefVal))
	{
		cerr << "No definition for Barry" << endl;
		assert(!Preferences().SetPreference("Barry", "Bonds"));
	} else {
		cerr << "Barry:" << prefVal << endl;
		prefVal += ".";
		assert(Preferences().SetPreference("Barry", prefVal));
	}

	if(!Preferences().GetPreference("Adam", &prefVal)) {
		cerr << "No definition for Adam" << endl;
		assert(!Preferences().SetPreference("Adam", "Chase"));
	} else {
		cerr << "Adam:" << prefVal << endl;
		prefVal += ".";
		assert(Preferences().SetPreference("Adam", prefVal));
	}

	assert(Preferences().SavePreferences());
	
}
