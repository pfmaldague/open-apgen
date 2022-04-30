/**
 * @file TLLibrary.h
 * @author amcp
 * @date Feb 26, 2013
 * @brief The main header of the CREST library
 * @copyright Copyright 2013 California Institute of Technology
 */

#ifndef INCLUDE_LIBCREST_H_
#define INCLUDE_LIBCREST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libcrest_dto.h"

/**
 * Validates a StateTimelineDTO structure
 * @param[in] dto the state timeline dto to validate
 * @return 0 if dto is null
 * or if dto.name is null
 * or if dto.name is the null terminated empty string
 * or if for all i in [0, dto.numRecords) dto.timestamps + i is null when dto.numRecords > 0
 * or if for all i in [0, dto.numRecords) *(dto.timestamps + i) is the empty string when dto.numRecords > 0
 * or if for all i in [0, dto.numrecords) dto.values + i is null when dto.numRecords > 0
 * and 1 otherwise
 * @warning user must also make sure that the timestamps are unique
 */
typedef unsigned int (*validateStateTimelineDTO_func)(StateTimelineDTO* dto);

/**
 * Creates an ActivityDTO from the inputs
 * @param start start time of this activity
 * @param end end time of this activity
 * @param name this activity's name
 * @param type the type string of this activity.
 * @return the new activity dto created
 * @pre name must not be null nor empty
 * @post dto.numParams = 0, dto.paramNames = NULL, dto.paramValues = NULL
 */
typedef ActivityDTO* (*createActivityDTO_func)(const char* start,
                                               const char* end,
                                               const char* name,
                                               const char* type);

/**
 * Adds the specified parameter to an activity
 * @param[in,out] dto the activity to which to add the parameter
 * @param[in] parName name of parameter to add
 * @param[in] parVal value of parameter to add
 * @pre dto is not null, par name is not null, par val is not null
 */
typedef void (*addParameterToActivityDTO_func)(ActivityDTO* dto,
                                               const char* parName,
                                               const char* parVal);

/**
 * Frees an activity DTO structure
 * @param[in] dto activity dto structure to free
 * @pre dto is valid per validateActivityDTO
 */
typedef void (*freeActivityDTOmembers_func)(ActivityDTO* dto);

/**
 * Creates a new emtpy ActivityTimelineDTO
 * @param[in] name the name to give the new act timeline
 * @return the new act timeline with name = name_in (a copy),
 * and numActivities = 0
 */
typedef ActivityTimelineDTO* (*createActivityTimelineDTO_func)(
    const char* name);

typedef ActivityDTO* (*addActivityStartDTOToActivityTimelineDTO_func)(
    ActivityTimelineDTO* tl, const char* name, const char* type,
    const char* start);

/**
 *
 * @param dto
 * @param endTime
 */
typedef void (*setActivityDTOEndTime_func)(ActivityDTO* dto,
                                           const char* endTime);

/**
 * Frees an activity timeline DTO structure
 * @param[in] dto activity timeline dto structure to free
 * @pre dto is valid per validateActivityTimelineDTO
 */
typedef void (*freeActivityTimelineDTO_func)(ActivityTimelineDTO* dto);

/**
 * Allocates and initializes a new state timeline according to the given parameters
 * @param[in] name the name of the state timeline dto
 * @param[in] type the type of the state timeline dto
 * @return the new state timeline dto
 * @pre name is not null
 * @post numRecords of new dto will be zero, and the name is copied
 */
typedef StateTimelineDTO* (*createStateTimelineDTO_func)(
    const char* name, StateTimelineTypeDTO type);

/**
 * Adds a records to a state timeline dto
 * @param[in] dto the state timeline dto to modify
 * @param[in] time the time stamp of the record
 * @param[in] value the value of the record
 * @pre time is not null, value is not null
 * @post dto.timestamps.append(time), dto.values.append(value). both strings
 * are copied
 */
typedef void (*addRecordToStateTimelineDTO_func)(StateTimelineDTO* dto,
                                                 const char* time,
                                                 const char* value);

/**
 * Add multiple records to a state timeline DTO
 * @param dto state timeline to which to add records
 * @param numRecs number of records to add
 * @param time an array of C strings of size numRecs
 * @param value an array of C strings of size numRecs
 */
typedef void (*addRecordsToStateTimelineDTO_func)(StateTimelineDTO* dto,
                                                  size_t numRecs,
                                                  const char** timestamps,
                                                  const char** values);

/**
 * Frees an state timeline DTO structure
 * @param[in] dto state timeline dto structure to free
 * @pre dto is valid per validateStateTimelineDTO
 */
typedef void (*freeStateTimelineDTO_func)(StateTimelineDTO* dto);

/**
 * Performs a deep copy of a state timeline DTO
 * @param[in] from the state timeline to copy
 * @param[out] to the state timeline to write to
 */
typedef StateTimelineDTO* (*stateTimelineDTODeepCopy_func)(
    const StateTimelineDTO* from);

/**
 * Performs a deep copy of a activity DTO
 * @param[in] from the activity to copy
 * @param[out] to the activity to write to
 */
typedef void (*activityDTODeepCopy_func)(const ActivityDTO* from,
                                         ActivityDTO* to);

/**
 * Performs a deep copy of a activity timeline DTO
 * @param[in] from the activity timeline to copy
 * @param[out] to the activity timeline to write to
 */
typedef void (*activityTimelineDTODeepCopy_func)(
    const ActivityTimelineDTO* from, ActivityTimelineDTO* to);

// StateTimeline* getStateTimeline(const std::string& qualifiedName);
// ActivityTimeline* getActivityTimeline(const std::string& qualifiedName);

/**
 * Initializes the dependencies of TLLibrary (OpenSSL and cURL)
 */
typedef void (*initializeTLLibrary_func)();

/**
 * Cleans up TLLibrary dependencies
 */
typedef void (*cleanupTLLibrary_func)();

/**
 * This function deletes a list of state timelines from TMS using a REST call.
 * @bug this function currently has a bug in it. Do not use it yet.
 */
typedef void (*deleteStateTimelines_func)(const StateTimelineDTO**, size_t,
                                          const char *, const char *,
                                          const char *, const char *);

/**
 * This function deletes activity timelines
 * @param[in]
 * @param[in]
 * @param[in]
 * @param[in]
 * @param[in]
 * @param[in]
 * @param[in]
 */
typedef void (*deleteActivityTimelines_func)(const ActivityTimelineDTO**,
                                             size_t,
                                             const char *, const char *,
                                             const char *, const char *);

/**
 * Inserts an array of StateTimelineDTO objects into TMS using multiple
 * parallel REST calls.
 * @param[in] timelines a pointer to an array of StateTimeline pointers
 * @param[in] numTimelines the size of timelines
 * @param[in] baseUrl the base URL of the TMS server
 * @param[in] scn the change number string at which to insert these state timelines
 * @param[in] dbNamespace the database instance to write to
 * @param[in] ssoTokenCookie the SSO token string
 * @param[in] creating 0 to use existing timelines and 1 to create new timelines
 * @param[in] inserting 0 to not insert data and not 0 to insert data
 * @pre This command will raise SIGABRT if OpenSSL failed to initialize.
 * @pre This command will raise SIGABRT if timelines is NULL
 * @pre This command will raise SIGABRT if baseUrl is NULL
 * @pre This command will raise SIGABRT if scn is NULL
 * @pre This command will raise SIGABRT if dbNamespace is NULL
 * @pre This command will raise SIGABRT if ssoTokenCookie is NULL
 * @post This command will add all the timelines in input to TMS using
 * parallel REST calls in a non-deterministic order
 */
typedef void (*insertStateTimelines_func)(const StateTimelineDTO** timelines,
                                          size_t numTimelines,
                                          const char* baseUrl, const char* scn,
                                          const char* dbNamespace,
                                          const char* ssoTokenCookie,
                                          unsigned int creating,
                                          unsigned int inserting,
                                          int debug);

/**
 * This function generates a state timeline with one value, namely, the value of
 * a state timeline in TMS at a given timestamp.
 * @param[in] baseUrl the base URL of the TMS server
 * @param[in] scn the change number string at which to insert these state timelines
 * @param[in] ns the database instance to write to
 * @param[in] tlName the SSO token string
 * @param[in] sso the SSO token string
 * @param[in] timestamp the time
 * @param[out] output the address of a StateTimeline pointer
 * @pre This command will raise SIGABRT if baseUrl is NULL
 * @pre This command will raise SIGABRT if scn is NULL
 * @pre This command will raise SIGABRT if ns is NULL
 * @pre This command will raise SIGABRT if sso is NULL
 * @pre This command will raise SIGABRT if timestamp is NULL or empty string
 * @pre if tl specified by baseUrl/scn/ns/tlName is not a state timeline, raise SIGABRT
 * @post if tl specified by baseUrl/scn/ns/tlName is a state timeline, output is a state
 * timeline with the one value, namely the value of the state timeline at the time
 * specified by the timestamp. If the timeline in TMS was empty, output = NULL.
 */
typedef void (*readStateTimelineRecord_func)(const char* baseUrl,
                                             const char* scn, const char* ns,
                                             const char* tlName,
                                             const char* sso,
                                             const char* timestamp,
                                             StateTimelineDTO** output);

/**
 * This function generates a state timeline with all the values contained in a state
 * timeline in TMS.
 * @param[in] baseUrl the base URL of the TMS server
 * @param[in] scn the change number string at which to insert these state timelines
 * @param[in] ns the database instance to write to
 * @param[in] tlName the SSO token string
 * @param[in] sso the SSO token string
 * @param[out] output the address of a StateTimeline pointer
 * @pre This command will raise SIGABRT if baseUrl is NULL
 * @pre This command will raise SIGABRT if scn is NULL
 * @pre This command will raise SIGABRT if ns is NULL
 * @pre This command will raise SIGABRT if sso is NULL
 * @pre This command will raise SIGABRT if timestamp is NULL or empty string
 * @pre if tl specified by baseUrl/scn/ns/tlName is not a state timeline, raise SIGABRT
 * @post if tl specified by baseUrl/scn/ns/tlName is a state timeline, output is a state
 * timeline with all values in the state timeline specified.
 */
typedef void (*readStateTimeline_func)(const char* baseUrl, const char* scn,
                                       const char* ns, const char* tlName,
                                       const char* sso,
                                       StateTimelineDTO** output);

/**
 * Inserts an array of ActivityTimelineDTO objects into TMS using multiple
 * parallel REST calls.
 * @param[in] timelines a pointer to an array of ActivityTimelineDTO pointers
 * @param[in] numTimelines the size of timelines
 * @param[in] baseUrl the base URL of the TMS server
 * @param[in] scn the change number string at which to insert these activity timelines
 * @param[in] dbNamespace the database instance to write to
 * @param[in] ssoTokenCookie the SSO token string
 * @param[in] creating 0 to use existing timelines and 1 to create new timelines
 * @param[in] inserting 0 to not insert data and not 0 to insert data
 * @pre This command will raise SIGABRT if OpenSSL failed to initialize.
 * @pre This command will raise SIGABRT if timelines is NULL
 * @pre This command will raise SIGABRT if baseUrl is NULL
 * @pre This command will raise SIGABRT if scn is NULL
 * @pre This command will raise SIGABRT if dbNamespace is NULL
 * @pre This command will raise SIGABRT if ssoTokenCookie is NULL
 * @post This command will add all the timelines in input to TMS using
 * parallel REST calls in a non-deterministic order
 */
typedef void (*insertActivityTimelines_func)(
    const ActivityTimelineDTO** timelines, size_t numTimelines,
    const char* baseUrl, const char* scn, const char* dbNamespace,
    const char* ssoTokenCookie, unsigned int creating, unsigned int inserting,
    int debug);

/**
 * This function gets an SSO token for the user
 * @param[in] serverUrl the server URL of the application server
 * @param[in] user username
 * @param[in] pass password string to use
 * @pre This command will raise SIGABRT if baseUrl is NULL
 * @pre This command will raise SIGABRT if scn is NULL
 * @pre This command will raise SIGABRT if dbNamespace is NULL
 * @pre This command will raise SIGABRT if user is NULL or empty string
 * @pre This command will raise SIGABRT if pass is NULL
 * @post token is obtained and parsed, memory allocated, and returned.
 * @return The SSO token, or NULL in case of an error.
 */
typedef char* (*getSsoTokenCookie_func)(const char* serverUrl, const char* user,
                                        const char* pass, int debug);

/**
 * Deletes a named resource in TMS/IMS (entity or namespace)
 * @param serverUrl the server url up to and including the port
 * @param scn the scn to work with
 * @param dbNamespace the database and namespace suburl
 * @param resourceName the name of the resource to delete.
 * @param type the entity type of the resource to delete
 * @param sso the sso token
 * @param debug not 1 to debug and 0 to not debug
 * @pre raise signal if serverUrl is null or zero length
 * @pre raise signal if scn is null
 * @pre raise signal if dbNamespace is null
 * @pre raise signal if sso is null
 * @post if resourceName is NULL and entity type is not SEQR_TIMELINE_RESOURCE,
 * do nothing as deleting IMS namespaces is not supported. Otherwise, if
 * resourceName is NULL and entity type is SEQR_TIMELINE_RESOURCE,
 * delete the specified namespace in TMS.  If resourceName is not NULL,
 * delete the specified entity in TMS/IMS.
 */
typedef void (*deleteResource_func)(const char* serverUrl,
                               const char* scn,
                               const char* dbNamespace,
                               const char* resourceName,
                               SeqrResourceType type,
                               const char* sso,
                               int debug);

/**
 *
 * @param filename
 * @param baseUrl
 * @param scn
 * @param dbNamespace
 * @param filestoreName
 * @param sso
 * @param[in] creating 0 to use existing timelines and not 0 to create new timelines
 * @param[in] inserting 0 to not insert data and not 0 to insert data
 */
typedef void (*uploadFileToIms_func)(const char* filename,
                                     const char* baseUrl,
                                     const char* scn,
                                     const char* dbNamespace,
                                     const char* filestoreName,
                                     const char* sso,
                                     const char* contentType,
                                     unsigned int creating,
                                     unsigned int inserting,
                                     int debug);

/**
 * Do an arbitrary
 * @param url
 * @param sso
 * @param data
 * @param length
 * @param nominalHttpCode
 * @param redirects
 */
typedef void (*postWithData_func)(const char* url,
                           const char* sso,
                           const char* contentType,
                           const char* data,
                           size_t length,
                           int nominalHttpCode,
                           int redirects, int debug,
                           char** resultData);

/**
 * Reads an entire file into memory
 * @param[in] fname the null delimited path to the file
 * @param[out] buf buffer to read file to
 * @param[out] read number of characters read
 * @pre raise signal if fname or buf is NULL
 * @post if returning 0, *buf points to the contents of the file
 * and read is the size of the file read.
 * @return 0 if file read successfully, and not zero otherwise
 */
typedef int (*readFile_func)(const char* fname, char** buf, unsigned int* read);

/**
 * Static libcrest API function handle structure.
 */
typedef struct TLLibraryAPI {
  validateStateTimelineDTO_func validateStateTimelineDTO;  // 1
  createActivityDTO_func createActivityDTO;                // ?
  addParameterToActivityDTO_func addParameterToActivityDTO;
  freeActivityDTOmembers_func freeActivityDTOmembers;                    // 2
  createActivityTimelineDTO_func createActivityTimelineDTO;
  addActivityStartDTOToActivityTimelineDTO_func addActivityStartDTOToActivityTimelineDTO;
  setActivityDTOEndTime_func setActivityDTOEndTime;
  freeActivityTimelineDTO_func freeActivityTimelineDTO;    // 3
  createStateTimelineDTO_func createStateTimelineDTO;
  addRecordToStateTimelineDTO_func addRecordToStateTimelineDTO;
  addRecordsToStateTimelineDTO_func addRecordsToStateTimelineDTO;
  freeStateTimelineDTO_func freeStateTimelineDTO;          // 4
  stateTimelineDTODeepCopy_func stateTimelineDTODeepCopy;  // 5
  activityDTODeepCopy_func activityDTODeepCopy;            // 6
  activityTimelineDTODeepCopy_func activityTimelineDTODeepCopy;  // 7
  initializeTLLibrary_func initializeTLLibrary;            // 8
  cleanupTLLibrary_func cleanupTLLibrary;                  // 9
  deleteStateTimelines_func deleteStateTimelines;          // 10
  deleteActivityTimelines_func deleteActivityTimelines;    // 11
  insertStateTimelines_func insertStateTimelines;          // 12
  readStateTimelineRecord_func readStateTimelineRecord;    // 12a
  readStateTimeline_func readStateTimeline;                // 12b
  insertActivityTimelines_func insertActivityTimelines;    // 13
  getSsoTokenCookie_func getSsoTokenCookie;                // 14
  deleteResource_func deleteResource;                      // 15
  uploadFileToIms_func uploadFileToIms;                    // 16
  postWithData_func postWithData;                          // 17
  readFile_func        readFile;                           // 18
  const char* packageVersion;                              // 19
} TLLibraryAPI;

typedef struct TLLibraryAPI TLLibraryAPI;

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_LIBCREST_H_
