//******************************************************************
// 
//  Generated by IDL to C++ Translator
//  
//  File name: Sample_ListChilds_Response_.h
//  Source: /home/firesurfer/workspace/kairo3_ws/src/kairo3/ros2_components/build/build/ros2_components/rosidl_generator_dds_idl/ros2_components_msg/srv/dds_opensplice/Sample_ListChilds_Response_.idl
//  Generated: Fri Jan 29 10:58:41 2016
//  OpenSplice V6.4.140407OSS
//  
//******************************************************************
#ifndef _SAMPLE_LISTCHILDS_RESPONSE__H_
#define _SAMPLE_LISTCHILDS_RESPONSE__H_

#include "sacpp_mapping.h"
#include "sacpp_DDS_DCPS.h"
#include "ListChilds_Response_.h"


namespace ros2_components_msg
{
   namespace srv
   {
      namespace dds_
      {
         struct Sample_ListChilds_Response_;

         struct Sample_ListChilds_Response_
         {
               DDS::ULongLong client_guid_0_;
               DDS::ULongLong client_guid_1_;
               DDS::LongLong sequence_number_;
               ListChilds_Response_ response_;
         };

         typedef DDS_DCPSStruct_var < Sample_ListChilds_Response_> Sample_ListChilds_Response__var;
         typedef DDS_DCPSStruct_out < Sample_ListChilds_Response_> Sample_ListChilds_Response__out;
      }
   }
}




#endif 
