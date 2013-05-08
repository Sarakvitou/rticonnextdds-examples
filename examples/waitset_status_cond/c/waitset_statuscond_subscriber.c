/* waitset_statuscond_subscriber.c

   A subscription example

   This file is derived from code automatically generated by the rtiddsgen 
   command:

   rtiddsgen -language C -example <arch> waitset_statuscond.idl

   Example subscription of type waitset_statuscond automatically generated by 
   'rtiddsgen'. To test them, follow these steps:

   (1) Compile this file and the example publication.

   (2) Start the subscription with the command
       objs/<arch>/waitset_statuscond_subscriber <domain_id> <sample_count>

   (3) Start the publication with the command
       objs/<arch>/waitset_statuscond_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and 
       multicast receive addresses via an environment variable or a file 
       (in the current working directory) called NDDS_DISCOVERY_PEERS. 
       
   You can run any number of publishers and subscribers programs, and can 
   add and remove them dynamically from the domain.
              
                                   
   Example:
        
       To run the example application on domain <domain_id>:
                          
       On UNIX systems: 
       
       objs/<arch>/waitset_statuscond_publisher <domain_id> 
       objs/<arch>/waitset_statuscond_subscriber <domain_id> 
                            
       On Windows systems:
       
       objs\<arch>\waitset_statuscond_publisher <domain_id>  
       objs\<arch>\waitset_statuscond_subscriber <domain_id>   
       
       
modification history
------------ -------       
*/

#include <stdio.h>
#include <stdlib.h>
#include "ndds/ndds_c.h"
#include "waitset_statuscond.h"
#include "waitset_statuscondSupport.h"



/* Delete all entities */
static int subscriber_shutdown(
    DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides the finalize_instance() method on
       domain participant factory for users who want to release memory used
       by the participant factory. Uncomment the following block of code for
       clean destruction of the singleton. */
/*
    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        printf("finalize_instance error %d\n", retcode);
        status = -1;
    }
*/

    return status;
}

static int subscriber_main(int domainId, int sample_count)
{
    DDS_DomainParticipant *participant = NULL;
    DDS_Subscriber *subscriber = NULL;
    DDS_Topic *topic = NULL;
    struct DDS_DataReaderListener reader_listener =
        DDS_DataReaderListener_INITIALIZER;
    DDS_DataReader *reader = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    DDS_StatusCondition *status_condition;
    DDS_WaitSet *waitset = NULL;
    waitset_statuscondDataReader *waitsets_reader = NULL;
    struct DDS_Duration_t timeout = {4,0};

    /* To customize participant QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant(
        DDS_TheParticipantFactory, domainId, &DDS_PARTICIPANT_QOS_DEFAULT,
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize subscriber QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    subscriber = DDS_DomainParticipant_create_subscriber(
        participant, &DDS_SUBSCRIBER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        printf("create_subscriber error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = waitset_statuscondTypeSupport_get_type_name();
    retcode = waitset_statuscondTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    topic = DDS_DomainParticipant_create_topic(
        participant, "Example waitset_statuscond",
        type_name, &DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        subscriber_shutdown(participant);
        return -1;
    }


    /* To customize data reader QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    reader = DDS_Subscriber_create_datareader(
        subscriber, DDS_Topic_as_topicdescription(topic),
        &DDS_DATAREADER_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
    if (reader == NULL) {
        printf("create_datareader error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    status_condition = DDS_Entity_get_statuscondition((DDS_Entity*)reader);
    if (status_condition == NULL) {
        printf("get_statuscondition error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    // Since a single status condition can match many statuses, 
    // enable only those we're interested in.
    retcode = DDS_StatusCondition_set_enabled_statuses(
        status_condition,
        DDS_DATA_AVAILABLE_STATUS);
    if (retcode != DDS_RETCODE_OK) {
        printf("set_enabled_statuses error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    // Create WaitSet, and attach conditions
    waitset = DDS_WaitSet_new();
    if (waitset == NULL) {
        printf("create waitset error\n");
        subscriber_shutdown(participant);
        return -1;        
    }


    retcode = DDS_WaitSet_attach_condition(waitset, (DDS_Condition*)status_condition);
    if (retcode != DDS_RETCODE_OK) {
        printf("attach_condition error\n");
        subscriber_shutdown(participant);
        return -1;
    }    

    // Get narrowed datareader
    waitsets_reader = waitset_statuscondDataReader_narrow(reader);
    if (waitsets_reader == NULL) {
        printf("DataReader narrow error\n");
        return -1;
    }

    /* Main loop */
    for (count=0; (sample_count == 0) || (count < sample_count); ++count) {
        struct DDS_ConditionSeq active_conditions = DDS_SEQUENCE_INITIALIZER;
        int i, j;

        retcode = DDS_WaitSet_wait(waitset, &active_conditions, &timeout);
        if (retcode == DDS_RETCODE_TIMEOUT) {
            printf("wait timed out\n");
            count+=2;
            continue;
        } else if (retcode != DDS_RETCODE_OK) {
            printf("wait returned error: %d\n", retcode);
            break;
        }

        printf("got %d active conditions\n",
               DDS_ConditionSeq_get_length(&active_conditions));
        
        for (i = 0; i < DDS_ConditionSeq_get_length(&active_conditions); ++i) {
            if (DDS_ConditionSeq_get(&active_conditions, i) == (DDS_Condition*)status_condition) {
                // A status condition triggered--see which ones
                DDS_StatusMask triggeredmask;
                triggeredmask = DDS_Entity_get_status_changes((DDS_Entity*)waitsets_reader);

                if (triggeredmask & DDS_DATA_AVAILABLE_STATUS) {
					struct waitset_statuscondSeq data_seq = 
						DDS_SEQUENCE_INITIALIZER;
					struct DDS_SampleInfoSeq info_seq = 
						DDS_SEQUENCE_INITIALIZER;
                
					retcode = waitset_statuscondDataReader_take(
						waitsets_reader, &data_seq, &info_seq,
						DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
						DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
					if (retcode != DDS_RETCODE_OK) {
						printf("take error %d\n", retcode);
						break;
					}
                
					for (j = 0; j < waitset_statuscondSeq_get_length(&data_seq); ++j) {
						if (!DDS_SampleInfoSeq_get_reference(&info_seq, j)->valid_data) {
							printf("   Got metadata\n");                     
							continue;
						}
						waitset_statuscondTypeSupport_print_data(
							waitset_statuscondSeq_get_reference(&data_seq, j));
					}

					retcode = waitset_statuscondDataReader_return_loan(
						waitsets_reader, &data_seq, &info_seq);
					if (retcode != DDS_RETCODE_OK) {
						printf("return loan error %d\n", retcode);
					}

				}

            }
        }
    }    

    /* Delete all entities */
    retcode = DDS_WaitSet_delete(waitset);
    if (retcode != DDS_RETCODE_OK) {
        printf("delete waitset error %d\n", retcode);
    }

    /* Cleanup and delete all entities */ 
    return subscriber_shutdown(participant);
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t** argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */
    
    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API, 
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
    
    return subscriber_main(domainId, sample_count);
}
#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API, 
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
    
    return subscriber_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char* __ctype = NULL;

void usrAppInit ()
{
#ifdef  USER_APPL_INIT
    USER_APPL_INIT;         /* for backwards compatibility */
#endif
    
    /* add application specific code here */
    taskSpawn("sub", RTI_OSAPI_THREAD_PRIORITY_NORMAL, 0x8, 0x150000, (FUNCPTR)subscriber_main, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   
}
#endif