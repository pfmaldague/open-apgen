/**
 * @file libcrest_dto.h
 * @author Alexander Patrikalakis
 * @date Mar 31, 2013
 * @brief Header file containing DTO structures used to pass data
 * through the C interface of the CREST library.
 * @copyright Copyright 2013 California Institute of Technology
 */

#ifndef INCLUDE_LIBCREST_DTO_H_
#define INCLUDE_LIBCREST_DTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/**
 * @struct ActivityDTO
 * @brief ActivityDTO is a data transfer object for
 * Activities. Users of ActivityDTO are responsible for
 * allocating the memory it consumes. ActivityDTO instances
 * can be deallocated using the freeActivityDTO handle in
 * TLLibraryAPI.
 *
 * Representation Fields:
 *   - startTime : C string
 *   - endTime : C string
 *   - name : C string
 *   - type : C string
 *   - numParams : size_t
 *   - paramNames : array of C strings
 *   - paramValues : array of C strings
 *
 * Derived Fields:
 *   - parameterMap : map<string, string> :==
 *                    for all i in [0,numParams) parameterMap[paramNames[i]] = paramValues[i]
 */
struct ActivityDTO {
  char* startTime;
  char* endTime;
  char* name;
  char* type;
  size_t numParams;
  char** paramNames;
  char** paramValues;
};
typedef struct ActivityDTO ActivityDTO;

/**
 * @struct ActivityTimelineDTO
 * @brief ActivityTimelineDTO is a data transfer object
 * for activity timelines. Users of ActivityTimelineDTO
 * instances are responsible for allocating the memory of
 * instance members. ActivityTimelineDTO instances
 * can be deallocated using the freeActivityTimelineDTO handle
 * in TLLibraryAPI.
 *
 * Representation fields:
 *   - name : C string
 *   - numActivities : the number of activities in this activity timeline
 *   - activities : size-numActivities array of ActivityDTO instances
 */
struct ActivityTimelineDTO {
  char* name;
  size_t numActivities;
  ActivityDTO* activities;
};
typedef struct ActivityTimelineDTO ActivityTimelineDTO;

/**
 * @enum StateTimelineTypeDTO
 *
 */
enum StateTimelineTypeDTO {
  CREST_STRING_TYPE_DTO = 0,  // !< CREST_STRING_TYPE_DTO
  CREST_FLOAT_TYPE_DTO        // !< CREST_FLOAT_TYPE_DTO
};

/**
 *
 */
enum SeqrResourceType {
  SEQR_TIMELINE_RESOURCE = 0,   // !< SEQR_TIMELINE_RESOURCE
  SEQR_FILESTORE_RESOURCE,      // !< SEQR_FILESTORE_RESOURCE
  SEQR_TRIPLESTORE_RESOURCE,    // !< SEQR_TRIPLESTORE_RESOURCE
  SEQR_NAMESPACE_RESOURCE       // !< SEQR_NAMESPACE_RESOURCE
};

/**
 * @enum SeqrEntityType
 */
enum SeqrEntityType {
  SEQR_STATE_TIMELINE = 0,    //  !< SEQR_STATE_TIMELINE
  SEQR_ACTIVITY_TIMELINE,     //  !< SEQR_ACTIVITY_TIMELINE
  SEQR_EVENT_TIMELINE,        //  !< SEQR_EVENT_TIMELINE
  SEQR_MEASUREMENT_TIMELINE,  //  !< SEQR_MEASUREMENT_TIMELINE
  SEQR_FILESTORE,             //  !< SEQR_FILESTORE
  SEQR_TRIPLESTORE            //  !< SEQR_TRIPLESTORE
};

/**
 * @enum SeqrActionType
 */
enum SeqrActionType {
  SEQR_CREATE = 0,  //  !< SEQR_CREATE
  SEQR_INSERT,      //  !< SEQR_INSERT
  SEQR_READ,        //  !< SEQR_READ
  SEQR_DELETE,      //  !< SEQR_DELETE
  SEQR_PUT          //  !< SEQR_PUT
};

/**
 * @struct StateTimelineDTO
 * @brief StateTimelineDTO is a data transfer object for
 * State Timelines. Users of StateTimelineDTO are responsible for
 * allocating the memory it consumes. StateTimelineDTO instances
 * can be deallocated using the freeStateTimelineDTO handle
 * in TLLibraryAPI.
 *
 * Representation Fields:
 *   - name : C string
 *   - type : StateTimelineTypeDTO (enum)
 *   - numRecords : size_t
 *   - timestamps : array of C strings
 *   - values : array of C strings
 *
 * Derived Fields:
 *   - recordMap : map<string, string> :==
 *                    for all i in [0,numRecords) recordMap[timestamps[i]] = values[i]
 */
struct StateTimelineDTO {
  char* name;
  enum StateTimelineTypeDTO type;
  size_t numRecords;
  char** timestamps;
  char** values;
};
typedef struct StateTimelineDTO StateTimelineDTO;

/**
 * @struct TripleStoreRelationDTO
 * @brief TripleStoreRelationDTO is a data transfer object for
 * IMS triplestore rows.
 */
struct TripleStoreRelationDTO {
  char* subject;
  char* predicate;
  char* object;
};
typedef struct TripleStoreRelationDTO TripleStoreRelationDTO;

/**
 * @struct TripleStoreDTO
 * @brief TripleStoreDTO is a data transfer object for
 * IMS triplestores.
 */
struct TripleStoreDTO {
  char* name;
  size_t size;
  TripleStoreRelationDTO* relations;
};
typedef struct TripleStoreDTO TripleStoreDTO;

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_LIBCREST_DTO_H_
