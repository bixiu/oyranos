/** @file oyranos_cmm_oyX1.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2007-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    Oyranos X11 module for Oyranos
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2007/12/12
 */

#include "oyranos_cmm.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_i18n.h"
#include "oyranos_monitor.h"
#include "oyranos_monitor_internal.h"
#include "oyranos_texts.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>


/* --- internal definitions --- */

#define CMM_NICK "oyX1"
#define CMM_VERSION {OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C}

/* OYX1_MONITOR_REGISTRATION */

int                oyX1CMMInit       ( );
int            oyX1CMMMessageFuncSet ( oyMessage_f         message_func );

/* OYX1_MONITOR_REGISTRATION -------------------------------------------------*/

#define OYX1_MONITOR_REGISTRATION OY_TOP_SHARED OY_SLASH OY_DOMAIN_STD OY_SLASH OY_TYPE_STD OY_SLASH "config.device.icc_profile.monitor." CMM_NICK

oyMessage_f message = 0;

extern oyCMMapi8_s oyX1_api8;
oyRankPad oyX1_rank_map[];

int          oyX1DeviceFromName_     ( const char        * device_name,
                                       oyOptions_s       * options,
                                       oyConfig_s       ** device );
int            oyX1Configs_Modify    ( oyConfigs_s       * devices,
                                       oyOptions_s       * options );
const char * oyX1GetText             ( const char        * select,
                                       oyNAME_e            type );
const char * oyX1Api8UiGetText       ( const char        * select,
                                       oyNAME_e            type );

/* --- implementations --- */

int                oyX1CMMInit       ( oyStruct_s        * filter )
{
  int error = 0;
  return error;
}


/*
oyPointer          oyCMMallocateFunc   ( size_t            size )
{
  oyPointer p = 0;
  if(size)
    p = malloc(size);
  return p;
}

void               oyCMMdeallocateFunc ( oyPointer         mem )
{
  if(mem)
    free(mem);
}*/

/** @func  oyX1CMMMessageFuncSet
 *  @brief API requirement
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/12/12
 *  @since   2007/12/12 (Oyranos: 0.1.8)
 */
int            oyX1CMMMessageFuncSet ( oyMessage_f         message_func )
{
  message = message_func;
  return 0;
}

#define OPTIONS_ADD(opts, name, clear) if(!error && name) \
        error = oyOptions_SetFromText( &opts, \
                                     OYX1_MONITOR_REGISTRATION OY_SLASH #name, \
                                       name, OY_CREATE_NEW ); \
        if(clear && name) { oyDeAllocateFunc_( (char*)name ); name = 0; }
#define OPTIONS_ADD_INT(opts, name) if(!error && name) { \
        oySprintf_( num, "%d", name ); \
        error = oyOptions_SetFromText( &opts, \
                                     OYX1_MONITOR_REGISTRATION OY_SLASH #name, \
                                       num, OY_CREATE_NEW ); \
        }

const char * oyX1_help_list = 
      "The presence of option \"command=list\" will provide a list of \n"
      " available devices. The actual device name can be found in option\n"
      " \"device_name\". The call is as lightwight as possible.\n"
      " The option \"display_name\" is optional to pass the X11 display name\n"
      " and obtain a unfiltered result. Its the way to get all monitors\n"
      " connected to a display.\n"
      " The option \"oyNAME_NAME\" returns a string containting geometry and\n"
      " if available, the profile name or size.\n"
      " The bidirectional option \"device_rectangle\" will cause to\n"
      " additionally add display geometry information as a oyRectangle_s\n"
      " object.\n"
      " The bidirectional option \"icc_profile\" will always add a\n"
      " oyProfile_s being it filled or set to NULL to show it was not found.\n"
      " The bidirectional option \"oyNAME_DESCRIPTION\" adds a string\n"
      " containting all properties. The text is separated by newline. The\n"
      " first line contains the actual key word, the even one the belonging\n"
      " string.\n"
      " The bidirectional \"oyNAME_DESCRIPTION\" option turns the \"list\" \n"
      " call into a expensive one.\n"
      " The bidirectional optional \"edid\" (specific) key word will\n"
      " additionally add the EDID information inside a oyBlob_s struct.\n"
      " A option \"edid\" key word with value \"refresh\" will \n"
      " update the EDID atom if possible.\n"
      " The option \"device_name\" may be added as a filter.\n"
      " \"list\" is normally a cheap call, see oyNAME_DESCRIPTION\n"
      " above.\n"
      " Informations are stored in the returned oyConfig_s::data member.";
const char * oyX1_help_properties =
      "The presence of option \"command=properties\" will provide the devices\n"
      " properties. Requires one device identifier returned with the \n"
      " \"list\" option. The properties may cover following entries:\n"
      " - \"manufacturer\" description\n"
      " - \"mnft\" (decoded mnft_id)\n"
      " - \"model\" textual name\n"
      " - \"serial\" not always present\n"
      " - \"host\" not always present\n"
      " - \"system_port\"\n"
      " - \"week\" manufacture date\n"
      " - \"year\" manufacture year\n"
      " - \"mnft_id\" manufacturer ID (undecoded mnft)\n"
      " - \"model_id\" model ID\n"
      " - \"display_geometry\" (specific) widthxheight+x+y ,e.g."
      " \"1024x786+0+0\"\n"
      " - \"colour_matrix_text.from_edid."
                   "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma\","
      " colour characteristics as found in EDID as text\n"
      " - \"colour_matrix.from_edid."
                   "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma\","
      " colour characteristics as found in EDID as doubles\n"
      " \n"
      " One option \"device_name\" will select the according X display.\n"
      " If not the module will try to get this information from \n"
      " your \"DISPLAY\" environment variable or uses what the system\n"
      " provides. The \"device_name\" should be identical with the one\n"
      " returned from a \"list\" request.\n"
      " The \"properties\" call might be a expensive one.\n"
      " Informations are stored in the returned oyConfig_s::backend_core member."
;
const char * oyX1_help_setup =
      "The presence of option \"command=setup\" will setup the device from a\n"
      " profile.\n"
      " The option \"device_name\" must be present, see \"list\" above.\n"
      " The option \"profile_name\" must be present, containing a ICC profile\n"      " file name."
;
const char * oyX1_help_unset =
      "The presence of call \"command=unset\" will invalidate a profile of\n"
      " a device.\n"
      " The option \"device_name\" must be present, see \"list\" above.\n"
;
const char * oyX1_help_add_edid_to_icc =
      "The presence of option \"command=add-edid-meta-to-icc\" will embedds device\n"
      " informations from a provided EDID block to a provided ICC profile.\n"
      " The option \"edid\" must be present and contain an\n"
      " oyBlob_s object with the valid EDID data block.\n"
      " The bidirectional option \"icc_profile\" options must be present,\n"
      " containing a oyProfile_s object.\n"
;
const char * oyX1_help =
      "The following help text informs about the communication protocol."
;

void     oyX1ConfigsUsage( oyStruct_s        * options )
{
    /** oyMSG_WARN shall make shure our message will be visible. */
    message( oyMSG_WARN, options, OY_DBG_FORMAT_ "\n %s",
             OY_DBG_ARGS_, oyX1_help );
    message( oyMSG_WARN, options, "%s()\n %s", __func__, oyX1_help_list );
    message( oyMSG_WARN, options, "%s()\n %s", __func__, oyX1_help_properties );
    message( oyMSG_WARN, options, "%s()\n %s", __func__, oyX1_help_setup );
    message( oyMSG_WARN, options, "%s()\n %s", __func__, oyX1_help_unset );
    message( oyMSG_WARN, options, "%s()\n %s", __func__, oyX1_help_add_edid_to_icc );
#if 0
    message( oyMSG_WARN, options, "%s()\n %s", __func__,
      "The presence of option \"get\" will provide a oyProfile_s of the\n"
      " device in a \"icc_profile\" option.\n"
      " The option \"device_name\" must be present, see \"list\" above.\n"
      );
#endif

  return;
}

int          oyX1DeviceFillEdid      ( oyConfig_s       ** device,
                                       oyPointer           edi,
                                       size_t              edi_size,
                                       const char        * device_name,
                                       const char        * host,
                                       const char        * display_geometry,
                                       const char        * system_port,
                                       oyOptions_s       * options )
{
  int error = !device || !edi;
  char * text = 0;

  if(error <= 0)
  {
      char * manufacturer=0, * mnft=0, * model=0, * serial=0, * vendor = 0;
      double colours[9] = {0,0,0,0,0,0,0,0,0};
      uint32_t week=0, year=0, mnft_id=0, model_id=0;
      char num[16];

      oyUnrollEdid1_( edi, &manufacturer, &mnft, &model, &serial, &vendor,
                      &week, &year, &mnft_id, &model_id, colours, oyAllocateFunc_);

      if(error <= 0)
      {
        if(!*device)
          *device = oyConfig_New( OYX1_MONITOR_REGISTRATION, 0 );
        error = !*device;
        if(!error && device_name)
        error = oyOptions_SetFromText( &(*device)->backend_core,
                                       OYX1_MONITOR_REGISTRATION OY_SLASH "device_name",
                                       device_name, OY_CREATE_NEW );

        OPTIONS_ADD( (*device)->backend_core, manufacturer, 1 )
        OPTIONS_ADD( (*device)->backend_core, mnft, 1 )
        OPTIONS_ADD( (*device)->backend_core, model, 1 )
        OPTIONS_ADD( (*device)->backend_core, serial, 1 )
        OPTIONS_ADD( (*device)->backend_core, vendor, 1 )
        OPTIONS_ADD( (*device)->backend_core, display_geometry, 0 )
        OPTIONS_ADD( (*device)->backend_core, system_port, 0 )
        OPTIONS_ADD( (*device)->backend_core, host, 0 )
        OPTIONS_ADD_INT( (*device)->backend_core, week )
        OPTIONS_ADD_INT( (*device)->backend_core, year )
        OPTIONS_ADD_INT( (*device)->backend_core, mnft_id )
        OPTIONS_ADD_INT( (*device)->backend_core, model_id )
        if(!error)
        {
          int i;
          char * save_locale = 0;

          if(colours[0] != 0.0 && colours[1] != 0.0 && colours[2] != 0.0 &&
             colours[3] != 0.0 && colours[4] != 0.0 && colours[5] != 0.0 && 
             colours[6] != 0.0 && colours[7] != 0.0 && colours[8] != 0.0 )
          {
            for(i = 0; i < 9; ++i)
              error = oyOptions_SetFromDouble( &(*device)->data,
                       OYX1_MONITOR_REGISTRATION OY_SLASH "colour_matrix."
                       "from_edid."
                      "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma",
                                             colours[i], i, OY_CREATE_NEW );
          }
          else
          {
            message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_
                     "  No EDID matrix found; device_name: \"%s\"",OY_DBG_ARGS_,
                     oyNoEmptyString_m_( device_name ) );
            error = -1;
          }

          text = oyAllocateFunc_(1024);

          if(!error && text)
          {
            /* sensible printing */
            save_locale = oyStringCopy_( setlocale( LC_NUMERIC, 0 ),
                                         oyAllocateFunc_ );
            setlocale( LC_NUMERIC, "C" );
            sprintf( text, "%g,%g," "%g,%g," "%g,%g," "%g,%g,%g",
                     colours[0], colours[1], colours[2], colours[3],
                     colours[4], colours[5], colours[6], colours[7],colours[8]);
            setlocale(LC_NUMERIC, save_locale);
            if(save_locale)
              oyFree_m_( save_locale );

            error = oyOptions_SetFromText( &(*device)->backend_core,
                                         OYX1_MONITOR_REGISTRATION OY_SLASH
                                         "colour_matrix_text_from_edid_"
                      "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma",
                                         text, OY_CREATE_NEW );
          }
          oyDeAllocateFunc_( text ); text = 0;
        }

      }
  }

  return error;
}

int          oyX1DeviceFromName_     ( const char        * device_name,
                                       oyOptions_s       * options,
                                       oyConfig_s       ** device )
{
  const char * value3 = 0;
  int error = !device;
  oyOption_s * o = 0;

    value3 = oyOptions_FindString( options, "edid", 0 );

    if(!error)
    {
      char * manufacturer=0, * mnft=0, * model=0, * serial=0, * vendor = 0,
           * host=0, * display_geometry=0, * system_port=0;
      double colours[9] = {0,0,0,0,0,0,0,0,0};
      oyBlob_s * edid = 0;
      uint32_t week=0, year=0, mnft_id=0, model_id=0;

      if(!device_name)
      {
        message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_
                "The \"device_name\" argument is\n"
                " missed to select a appropriate device for the"
                " \"properties\" call.", OY_DBG_ARGS_ );
        error = 1;
      }

      if(error <= 0)
        error = oyGetMonitorInfo_lib( device_name,
                                      &manufacturer, &mnft, &model, &serial,
                                      &vendor, &display_geometry, &system_port,
                                      &host, &week, &year, &mnft_id, &model_id,
                                      colours,
                                      &edid, oyAllocateFunc_,
                                      (oyStruct_s*)options );

      if(error != 0)
        message( oyMSG_WARN, (oyStruct_s*)options, 
                 OY_DBG_FORMAT_ "\n  Could not complete \"properties\" call.\n"
                 "  oyGetMonitorInfo_lib returned with %s; device_name:"
                 " \"%s\"", OY_DBG_ARGS_, error > 0 ? "error(s)" : "issue(s)",
                 oyNoEmptyString_m_( device_name ) );

      if(error <= 0 && edid)
        error = oyX1DeviceFillEdid( device, edid->ptr, edid->size,
                                    device_name,
                                    host, display_geometry, system_port,
                                    options );
      if(error != 0)
        message( oyMSG_WARN, (oyStruct_s*)options,
                 OY_DBG_FORMAT_ "\n  Could not complete \"properties\" call.\n"
                 "  oyGetMonitorInfo_lib returned with %s %d; device_name:"
                 " \"%s\"",OY_DBG_ARGS_, error > 0 ? "error(s)" : "issue(s)",
                 error, oyNoEmptyString_m_( device_name ) );

      if(value3)
      {
        if(!error && edid)
        {
          int has = 0;
          o = oyConfig_Find( *device,
                             OYX1_MONITOR_REGISTRATION OY_SLASH "edid" );
          if(o)
            has = 1;
          else
            o = oyOption_New( OYX1_MONITOR_REGISTRATION OY_SLASH "edid", 0 );
          error = !o;
          if(!error)
          error = oyOption_SetFromData( o, edid->ptr, edid->size );
          if(!error)
          {
            if(has)
              oyOption_Release( &o );
            else
              oyOptions_MoveIn( (*device)->data, &o, -1 );
          }
        }
      }
      oyBlob_Release( &edid );
    }

  return error;
}


/** Function oyX1Configs_FromPattern
 *  @brief   oyX1 oyCMMapi8_s Xorg monitors
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/01/28
 */
int            oyX1Configs_FromPattern (
                                       const char        * registration,
                                       oyOptions_s       * options,
                                       oyConfigs_s      ** s )
{
  oyConfigs_s * devices = 0;
  oyConfig_s * device = 0;
  char ** texts = 0;
  char * text = 0,
       * device_name_temp = 0;
  int texts_n = 0, i,
      error = !s;
  const char * odevice_name = 0,
             * oprofile_name = 0,
             * odisplay_name = 0,
             * device_name = 0;
  int rank = oyFilterRegistrationMatch( oyX1_api8.registration, registration,
                                        oyOBJECT_CMM_API8_S );
  oyAlloc_f allocateFunc = malloc;
  const char * tmp = 0;


  /** 1. In case no option is provided or something fails, show a message. */
  if(!options || !oyOptions_Count( options ))
  {
    oyX1ConfigsUsage( (oyStruct_s*)options );
    return 0;
  }

  if(rank && error <= 0)
  {
    devices = oyConfigs_New(0);


    /** 2. obtain a proper device_name */
    odisplay_name = oyOptions_FindString( options, "display_name", 0 );
    odevice_name = oyOptions_FindString( options, "device_name", 0 );
    /*message(oyMSG_WARN, (oyStruct_s*)options, "list: %s", value2);*/

    if(odisplay_name && odisplay_name[0])
      device_name = odisplay_name;
    else if(odevice_name && odevice_name[0])
      device_name = odevice_name;
    else
    {
      tmp = getenv("DISPLAY");
#if !defined(__APPLE__)
      if(!tmp)
      {
        message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
              "DISPLAY variable not set: giving up\n. Options:\n%s",
                OY_DBG_ARGS_,
                oyOptions_GetText( options, oyNAME_NICK )
                );
        error = 1;
        return error;
      }

      device_name_temp = oyStringCopy_( tmp, oyAllocateFunc_ );
      if(device_name_temp &&
         (text = strchr(device_name_temp,':')) != 0)
        if( (text = strchr(device_name_temp, '.')) != 0 )
          text[0] = '\000';

      device_name = device_name_temp;
#endif
      text = 0;
    }

    /** 3.  handle the actual call */
    /** 3.1 "list" call */
    if(oyOptions_FindString( options, "command", "list" ) ||
       oyOptions_FindString( options, "command", "properties" ))
    {
      texts_n = oyGetAllScreenNames( device_name, &texts, allocateFunc );

      /** 3.1.1 iterate over all requested devices */
      for( i = 0; i < texts_n; ++i )
      {
        /* filter */
        if(odevice_name && strcmp(odevice_name, texts[i]) != 0)
          continue;

        device = oyConfig_New( OYX1_MONITOR_REGISTRATION, 0 );
        error = !device;

         /** 3.1.2 tell the "device_name" */
        if(error <= 0)
        error = oyOptions_SetFromText( &device->backend_core,
                                       OYX1_MONITOR_REGISTRATION OY_SLASH
                                       "device_name",
                                       texts[i], OY_CREATE_NEW );

        oyConfigs_MoveIn( devices, &device, -1 );
      }

      if(error <= 0)
      {
        if(devices && oyConfigs_Count(devices))
          error = oyX1Configs_Modify( devices, options );
        else
          message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                "No monitor devices found.\n Options:\n%s", OY_DBG_ARGS_,
                oyOptions_GetText( options, oyNAME_NICK )
                );
      }

      if(error <= 0)
        *s = devices;

      oyStringListRelease_( &texts, texts_n, free );

      goto cleanup;

    } else

    /** 3.3 "setup" call; bring a profile to the device */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "setup" ))
    {
      oprofile_name = oyOptions_FindString( options, "profile_name", 0 );
      error = !odevice_name || !oprofile_name;
      if(error >= 1)
        message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
              "The device_name/profile_name option is missed. Options:\n%s",
                OY_DBG_ARGS_,
                oyOptions_GetText( options, oyNAME_NICK )
                );
      else
        error = oyX1MonitorProfileSetup( odevice_name, oprofile_name );

      goto cleanup;

    } else

    /** 3.4 "unset" call; clear a profile from a device */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "unset" ))
    {
      error = !odevice_name;
      if(error >= 1)
        message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                "The device_name option is missed. Options:\n%s",
                OY_DBG_ARGS_, oyOptions_GetText( options, oyNAME_NICK )
                );
      else
        error = oyX1MonitorProfileUnset( odevice_name );

      goto cleanup;
    }

    /** 3.5 "help" call; display a help text */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "help" ))
    {
      oyX1ConfigsUsage( (oyStruct_s*)options );

      goto cleanup;
    }

    /** 3.6 internal "add-edid-meta-to-icc" call; Embedd infos to ICC profile 
     *      as meta tag. Might be moved to a oyCMMapi10_s object. */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "add-edid-meta-to-icc" ))
    {
      oyProfile_s * prof;
      oyBlob_s * edid;
      oyConfig_s * device = 0;

      prof = (oyProfile_s*)oyOptions_GetType( options, -1, "icc_profile",
                                        oyOBJECT_PROFILE_S );
      
      edid = (oyBlob_s*)oyOptions_GetType( options, -1, "edid",
                                        oyOBJECT_BLOB_S );

      if(!prof || !edid)
        error = 1;
      if(error >= 1)
      {
        message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                "\"edid\" or \"icc_profile\" missed:\n%s",
                OY_DBG_ARGS_, oyOptions_GetText( options, oyNAME_NICK )
                );
        oyX1ConfigsUsage( (oyStruct_s*)options );
      }
      else
      {
        error = oyX1DeviceFillEdid( &device, edid->ptr, edid->size,
                                    NULL,
                                    NULL, NULL, NULL,
                                    options );
        oyProfile_DeviceAdd( prof, device, 0 );
      }

      goto cleanup;
    }
  }


  message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                "This point should not be reached. Options:\n%s", OY_DBG_ARGS_,
                oyOptions_GetText( options, oyNAME_NICK )
                );

  oyX1ConfigsUsage( (oyStruct_s*)options );


  cleanup:
  if(device_name_temp)
    oyFree_m_( device_name_temp );


  return error;
}


/** Function oyX1Configs_Modify
 *  @brief   oyX1 oyCMMapi8_s Xorg monitor manipulation
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/08/21
 */
int            oyX1Configs_Modify    ( oyConfigs_s       * devices,
                                       oyOptions_s       * options )
{
  oyConfig_s * device = 0;
  oyOption_s * o = 0, * o_tmp = 0;
  oyRectangle_s * rect = 0;
  const oyRectangle_s * r = 0;
  oyProfile_s * prof = 0;
  char * text = 0;
  int n = 0, i,
      error = !devices || !oyConfigs_Count( devices ),
      t_err = 0,
      has;
  const char * oprofile_name = 0,
             * device_name = 0;
  int rank = 0;
  oyAlloc_f allocateFunc = malloc;
  const char * tmp = 0;


  /** 1. In case no option is provided or something fails, show a message. */
  if(!options || !oyOptions_Count( options ))
  {
    oyX1ConfigsUsage( (oyStruct_s*)options );
    return 0;
  }

  n = oyConfigs_Count( devices );
  for( i = 0; i < n; ++i )
  {
    device = oyConfigs_Get( devices, i );
    rank += oyFilterRegistrationMatch( oyX1_api8.registration,
                                       device->registration,
                                       oyOBJECT_CMM_API8_S );
    oyConfig_Release( &device );
  }

  if(rank && error <= 0)
  {
    /** 3.  handle the actual call */
    /** 3.1 "list" call */
    if(oyOptions_FindString( options, "command", "list" ) ||
       oyOptions_FindString( options, "command", "properties" ))
    {
      n = oyConfigs_Count( devices );

      /** 3.1.1 iterate over all provided devices */
      for( i = 0; i < n; ++i )
      {
        device = oyConfigs_Get( devices, i );
        rank = oyFilterRegistrationMatch( oyX1_api8.registration,
                                          device->registration,
                                          oyOBJECT_CMM_API8_S );
        if(!rank)
        {
          oyConfig_Release( &device );
          continue;
        }

        /** 3.1.2 get the "device_name" */
        if(error <= 0)
        device_name = oyConfig_FindString( device, "device_name", 0 );

        /** 3.1.3 tell the "device_rectangle" in a oyRectangle_s */
        if(oyOptions_FindString( options, "device_rectangle", 0 ) ||
           oyOptions_FindString( options, "oyNAME_NAME", 0 ))
        {
          has = 0;
          rect = oyX1Rectangle_FromDevice( device_name );
          if(!rect)
          {
            WARNc1_S( "Could not obtain rectangle information for %s",
                      device_name );
          } else
          {
            o = oyConfig_Find( device, "device_rectangle" );
            if(o)
              has = 1;
            else
              o = oyOption_New( OYX1_MONITOR_REGISTRATION OY_SLASH
                                "device_rectangle", 0 );
            error = oyOption_StructMoveIn( o, (oyStruct_s**) &rect );
            if(has)
              oyOption_Release( &o );
            else
              oyOptions_MoveIn( device->data, &o, -1 );
          }
        }

        /** 3.1.4 tell the "icc_profile" in a oyProfile_s */
        if( oyOptions_FindString( options, "icc_profile", 0 ) ||
            oyOptions_FindString( options, "oyNAME_NAME", 0 ))
        {
          size_t size = 0;
          uint32_t flags = 0;
          char * data = 0;
          oyProfile_s * p = 0;

          if(oyOptions_FindString( options, "net_color_region_target", 0 ))
          {
            if(oy_debug)
              message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                     "Try %s(_xxx) from %s",
                     OY_DBG_ARGS_,
                     oyOptions_FindString(options, "net_color_region_target", 0) ? 
                     OY_ICC_COLOUR_SERVER_TARGET_PROFILE_IN_X_BASE :
                     OY_ICC_V0_3_TARGET_PROFILE_IN_X_BASE,
                     device_name );
            flags |= 0x01;
          }
          data = oyX1GetMonitorProfile( device_name, flags, &size,
                                        allocateFunc );


          has = 0;
          o = oyConfig_Find( device, "icc_profile" );
          if(o)
          {
            /* the device might have assigned a dummy icc_profile, to show 
             * it can handle. But thats not relevant here. */
            p = (oyProfile_s*) oyOption_StructGet( o, oyOBJECT_PROFILE_S );
            if(oyProfile_GetSignature( p, oySIGNATURE_MAGIC ) == icMagicNumber)
              has = 1;
            else
              oyOption_Release( &o );

            oyProfile_Release( &p );
          }

          if(data && size)
          {
            prof = oyProfile_FromMem( size, data, 0, 0 );
            free( data );
            if(has == 0)
            {
              const char * key = OYX1_MONITOR_REGISTRATION OY_SLASH "icc_profile";
              if(oyOptions_FindString(options, "net_color_region_target", 0))
                key = OYX1_MONITOR_REGISTRATION OY_SLASH "icc_profile.net_color_region_target";
              o = oyOption_New( key, 0 );
            }
          } else if(oyOptions_FindString( options, "icc_profile.fallback", 0 ))
          {
            icHeader * header = 0;
            /* fallback: try to get EDID to build a profile */
            o_tmp = oyConfig_Find( device, "colour_matrix."
                     "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma");
            if(!o_tmp)
            {
              oyOptions_SetFromText( &options,
                                     OYX1_MONITOR_REGISTRATION OY_SLASH
                                     "edid",
                                     "yes", OY_CREATE_NEW );
              error = oyX1DeviceFromName_( device_name, options, &device );
              o_tmp = oyConfig_Find( device, "colour_matrix."
                     "redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma");
            }

            if(o_tmp)
            {
              oyOptions_s * opts = oyOptions_New(0),
                          * result = 0;
              error = oyOptions_MoveIn( opts, &o_tmp, -1 );
              oyOptions_Handle( "///create_profile.icc",
                                opts,"create_profile.icc_profile.colour_matrix",
                                &result );
              prof = (oyProfile_s*)oyOptions_GetType( result, -1, "icc_profile",
                                        oyOBJECT_PROFILE_S );
              oyOptions_Release( &result );
            }

            if(prof)
            {
              const char * t = 0;
              t = oyConfig_FindString( device, "model", 0 );
              if(!t)
                message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                "Could not obtain \"manufacturer\" from monitor device for %s",
                     OY_DBG_ARGS_, device_name );
              else
                STRING_ADD( text, oyConfig_FindString( device, "model", 0 ) );
              STRING_ADD( text, "_edid" );
              error = oyProfile_AddTagText( prof,
                                            icSigProfileDescriptionTag, text);
              oyDeAllocateFunc_( text ); text = 0;
              t = oyConfig_FindString( device, "manufacturer", 0);
              if(!t)
                message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                "Could not obtain \"manufacturer\" from monitor device for %s",
                     OY_DBG_ARGS_, device_name );
              else
                error = oyProfile_AddTagText( prof, icSigDeviceMfgDescTag, t);
              t = oyConfig_FindString( device, "model", 0 );
              if(!t)
                message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                     "Could not obtain \"model\" from monitor device for %s",
                     OY_DBG_ARGS_, device_name );
              else
                error = oyProfile_AddTagText( prof, icSigDeviceModelDescTag, t);
              data = oyProfile_GetMem( prof, &size, 0, oyAllocateFunc_ );
              header = (icHeader*) data;
              o_tmp = oyConfig_Find( device, "mnft" );
              t = oyConfig_FindString( device, "mnft", 0 );
              if(!t)
                message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                     "Could not obtain \"mnft\" from monitor device for %s",
                     OY_DBG_ARGS_, device_name );
              else
                sprintf( (char*)&header->manufacturer, "%s", t );
            }
            oyOption_Release( &o_tmp );
            oyProfile_Release( &prof );
            if(data && size)
            {
              prof = oyProfile_FromMem( size, data, 0, 0 );
              oyDeAllocateFunc_( data ); data = NULL; size = 0;
            }
            if(has == 0)
              o = oyOption_New( OYX1_MONITOR_REGISTRATION OY_SLASH
                                "icc_profile.fallback", 0 );
            error = -1;
          }

          if(!o)
              o = oyOption_New( OYX1_MONITOR_REGISTRATION OY_SLASH
                                "icc_profile", 0 );

          if(prof)
          {
            t_err = oyOption_StructMoveIn( o, (oyStruct_s**) &prof );
            if(t_err > 0)
              error = t_err;
          }
          else
          /** Warn and return issue on not found profile. */
          {
            message( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n  "
                     "Could not obtain %s(_xxx) information for %s",
                     OY_DBG_ARGS_,
                     oyOptions_FindString(options, "net_color_region_target", 0) ? 
                     OY_ICC_COLOUR_SERVER_TARGET_PROFILE_IN_X_BASE :
                     OY_ICC_V0_3_TARGET_PROFILE_IN_X_BASE,
                     device_name );

            /* Show the "icc_profile" option is understood. */
            prof = 0;
            error = oyOption_StructMoveIn( o, (oyStruct_s**) &prof );
            error = -1;
          }

          if(!has)
            oyOptions_Set( device->data, o, -1, 0 );

          oyOption_Release( &o );
        }

        /** 3.1.5 construct a oyNAME_NAME string */
        if(oyOptions_FindString( options, "oyNAME_NAME", 0 ))
        {
          o = oyOptions_Find( device->data, "device_rectangle" );
          r = (oyRectangle_s*) o->value->oy_struct;

          text = 0; tmp = 0;
      
          tmp = oyRectangle_Show( (oyRectangle_s*)r );
          STRING_ADD( text, tmp );
          oyOption_Release( &o );

          o = oyOptions_Find( device->data, "icc_profile" );

          if( o && o->value && o->value->oy_struct && 
              o->value->oy_struct->type_ == oyOBJECT_PROFILE_S)
          {
            prof = oyProfile_Copy( (oyProfile_s*) o->value->oy_struct, 0 );
            tmp = oyProfile_GetFileName( prof, 0 );

            STRING_ADD( text, "  " );
            if(tmp)
            {
              if(oyStrrchr_( tmp, OY_SLASH_C ))
                STRING_ADD( text, oyStrrchr_( tmp, OY_SLASH_C ) + 1 );
              else
                STRING_ADD( text, tmp );
            } else
            {
              tmp = oyProfile_GetText( prof, oyNAME_DESCRIPTION );
              if(tmp)
                STRING_ADD( text, tmp );
            }

            oyProfile_Release( &prof );
          }

          if(error <= 0)
          {
            t_err = oyOptions_SetFromText( &device->data,
                                         OYX1_MONITOR_REGISTRATION OY_SLASH
                                         "oyNAME_NAME",
                                         text, OY_CREATE_NEW );
            if(t_err > 0)
              error = t_err;
          }
          oyFree_m_( text );
        }

        if(!oyOptions_FindString( options, "icc_profile.fallback", 0 ) &&
           (oyOptions_FindString( options, "command", "properties" ) ||
            oyOptions_FindString( options, "edid", "refresh" )))
          error = oyX1DeviceFromName_( device_name, options, &device );

        /** 3.1.6 add the rank scheme to combine properties */
        if(error <= 0 && !device->rank_map)
          device->rank_map = oyRankMapCopy( oyX1_rank_map,
                                            device->oy_->allocateFunc_ );

        oyConfig_Release( &device );
      }

      goto cleanup;

    } else

    /** 3.3 "setup" call; bring a profile to the device */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "setup" ))
    {
      n = oyConfigs_Count( devices );

      /** 3.1.1 iterate over all provided devices */
      for( i = 0; i < n; ++i )
      {
        device = oyConfigs_Get( devices, i );
        rank = oyFilterRegistrationMatch( oyX1_api8.registration,
                                          device->registration,
                                          oyOBJECT_CMM_API8_S );
        if(!rank)
        {
          oyConfig_Release( &device );
          continue;
        }

        /** 3.1.2 get the "device_name" */
        if(error <= 0)
        device_name = oyConfig_FindString( device, "device_name", 0 );
        oprofile_name = oyOptions_FindString( options, "profile_name", 0 );

        error = !device_name || !oprofile_name;
        if(error >= 1)
          message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                  "The device_name/profile_name option is missed. Options:\n%s",
                  OY_DBG_ARGS_,
                  oyOptions_GetText( options, oyNAME_NICK )
                  );
        else
          error = oyX1MonitorProfileSetup( device_name, oprofile_name );

        oyConfig_Release( &device );
      }

      goto cleanup;

    } else

    /** 3.4 "unset" call; clear a profile from a device */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "unset" ))
    {
      n = oyConfigs_Count( devices );

      /** 3.1.1 iterate over all provided devices */
      for( i = 0; i < n; ++i )
      {
        device = oyConfigs_Get( devices, i );
        rank = oyFilterRegistrationMatch( oyX1_api8.registration,
                                          device->registration,
                                          oyOBJECT_CMM_API8_S );
        if(!rank)
        {
          oyConfig_Release( &device );
          continue;
        }

        /** 3.1.2 get the "device_name" */
        if(error <= 0)
        device_name = oyConfig_FindString( device, "device_name", 0 );
        oprofile_name = oyOptions_FindString( options, "profile_name", 0 );

        error = !device_name || !oprofile_name;
        if(error >= 1)
          message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                  "The device_name option is missed. Options:\n%s",
                  OY_DBG_ARGS_,
                  oyOptions_GetText( options, oyNAME_NICK )
                  );
        else
          error = oyX1MonitorProfileUnset( device_name );

        oyConfig_Release( &device );
      }

      goto cleanup;
    }

    /** 3.5 "help" call; display a help text */
    if(error <= 0 &&
       oyOptions_FindString( options, "command", "help" ))
    {
      oyX1ConfigsUsage( (oyStruct_s*)options );

      goto cleanup;
    }
  }


  message(oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ "\n "
                "This point should not be reached. Options:\n%s", OY_DBG_ARGS_,
                oyOptions_GetText( options, oyNAME_NICK )
                );

  oyX1ConfigsUsage( (oyStruct_s*)options );


  cleanup:

  return error;
}

/** Function oyX1Config_Rank
 *  @brief   oyX1 oyCMMapi8_s Xorg monitor check
 *
 *  @param[in]     config              the monitor device configuration
 *  @return                            rank value
 *
 *  @version Oyranos: 0.1.10
 *  @date    2009/01/26
 *  @since   2009/01/26 (Oyranos: 0.1.10)
 */
int            oyX1Config_Rank       ( oyConfig_s        * config )
{
  int error = !config,
      rank = 1;

  if(!config)
  {
    message(oyMSG_DBG, (oyStruct_s*)config, OY_DBG_FORMAT_ "\n "
                "No config argument provided.\n", OY_DBG_ARGS_ );
    return 0;
  }

  if(error <= 0)
  {
    /* evaluate a driver specific part of the options */
  }

  return rank;
}

/** @instance oyX1_rank_map
 *  @brief    oyRankPad map for mapping device to configuration informations
 *
 *  @version Oyranos: 0.1.10
 *  @date    2009/01/27
 *  @since   2009/01/27 (Oyranos: 0.1.10)
 */
oyRankPad oyX1_rank_map[] = {
  {"device_name", 2, -1, 0},           /**< is good */
  {"profile_name", 0, 0, 0},           /**< non relevant for device properties*/
  {"manufacturer", 0, 0, 0},           /**< is nice, covered by mnft_id */
  {"model", 0, 0, 0},                  /**< important, covered by model_id */
  {"serial", 10, -2, 0},               /**< important, could slightly fail */
  {"host", 1, 0, 0},                   /**< nice to match */
  {"system_port", 2, 0, 0},            /**< good to match */
  {"week", 2, 0, 0},                   /**< good to match */
  {"year", 2, 0, 0},                   /**< good to match */
  {"mnft", 0, 0, 0},                   /**< is nice, covered by mnft_id */
  {"mnft_id", 1, -1, 0},               /**< is nice */
  {"model_id", 5, -5, 0},              /**< important, should not fail */
  {"display_geometry", 3, -1, 0},      /**< important to match, as fallback */
  {0,0,0,0}                            /**< end of list */
};

const char * oyX1Api8UiGetText       ( const char        * select,
                                       oyNAME_e            type )
{
  static char * category = 0;
  if(strcmp(select,"name") == 0 ||
     strcmp(select,"help") == 0)
  {
    /* The "help" and "name" texts are identical, as the module contains only
     * one filter to provide help for. */
    return oyX1GetText(select,type);
  }
  else if(strcmp(select, "device_class")==0)
  {
        if(type == oyNAME_NICK)
            return _("Monitor");
        else if(type == oyNAME_NAME)
            return _("Monitor");
        else
            return _("Monitors, which can be detected through the video card driver and windowing system.");
  }
  else if(strcmp(select,"category") == 0)
  {
    if(!category)
    {
      STRING_ADD( category, _("Colour") );
      STRING_ADD( category, _("/") );
      /* CMM: abbreviation for Colour Matching Module */
      STRING_ADD( category, _("Device") );
      STRING_ADD( category, _("/") );
      STRING_ADD( category, _("Monitor") );
    }
         if(type == oyNAME_NICK)
      return "category";
    else if(type == oyNAME_NAME)
      return category;
    else
      return category;
  } 
  return 0;
}
const char * oyX1_api8_ui_texts[] = {"name", "help", "device_class", "category", 0};

/** @instance oyX1_api8_ui
 *  @brief    oyX1 oyCMMapi8_s::ui implementation
 *
 *  The UI for oyX1 devices.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/14 (Oyranos: 0.1.10)
 *  @date    2009/12/16
 */
oyCMMui_s oyX1_api8_ui = {
  oyOBJECT_CMM_DATA_TYPES_S,           /**< oyOBJECT_e       type; */
  0,0,0,                            /* unused oyStruct_s fields; keep to zero */

  CMM_VERSION,                         /**< int32_t version[3] */
  {0,1,10},                            /**< int32_t module_api[3] */

  0, /* oyCMMFilter_ValidateOptions_f */
  0, /* oyWidgetEvent_f */

  "Colour/Device/Monitor", /* category */
  0,   /* const char * options */
  0,   /* oyCMMuiGet_f oyCMMuiGet */

  oyX1Api8UiGetText,  /* oyCMMGetText_f getText */
  oyX1_api8_ui_texts  /* (const char**)texts */
};

oyIcon_s oyX1_api8_icon = {
  oyOBJECT_ICON_S, 0,0,0, 0,0,0, "oyranos_logo.png"
};

/** @instance oyX1_api8
 *  @brief    oyX1 oyCMMapi8_s implementations
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/08/21
 */
oyCMMapi8_s oyX1_api8 = {
  oyOBJECT_CMM_API8_S,
  0,0,0,
  (oyCMMapi_s*) 0, /**< next */

  oyX1CMMInit,               /**< oyCMMInit_f      oyCMMInit */
  oyX1CMMMessageFuncSet,     /**< oyCMMMessageFuncSet_f oyCMMMessageFuncSet */

  OYX1_MONITOR_REGISTRATION, /**< registration */
  {0,3,0},                   /**< int32_t version[3] */
  {0,1,10},                  /**< int32_t module_api[3] */
  0,                         /**< char * id_ */

  0,                         /**< oyCMMapi5_s * api5_ */
  oyX1Configs_FromPattern,   /**<oyConfigs_FromPattern_f oyConfigs_FromPattern*/
  oyX1Configs_Modify,        /**< oyConfigs_Modify_f oyConfigs_Modify */
  oyX1Config_Rank,           /**< oyConfig_Rank_f oyConfig_Rank */

  &oyX1_api8_ui,             /**< device class UI name and help */
  &oyX1_api8_icon,           /**< device icon */

  oyX1_rank_map              /**< oyRankPad ** rank_map */
};

/* OYX1_MONITOR_REGISTRATION -------------------------------------------------*/


/**
 *  This function implements oyCMMInfoGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/23 (Oyranos: 0.1.10)
 *  @date    2008/12/30
 */
const char * oyX1GetText             ( const char        * select,
                                       oyNAME_e            type )
{
         if(strcmp(select, "name")==0)
  {
         if(type == oyNAME_NICK)
      return _(CMM_NICK);
    else if(type == oyNAME_NAME)
      return _("Oyranos X11");
    else
      return _("The window support module of Oyranos.");
  } else if(strcmp(select, "help")==0)
  {
    static char * t = 0;
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("The oyX1 module supports the generic device protocol.");
    else
    {
      if(!t)
      {
        t = malloc( strlen(oyX1_help) + strlen(oyX1_help_list)
                    + strlen(oyX1_help_properties) + strlen(oyX1_help_setup)
                    + strlen(oyX1_help_unset)
                    + strlen(oyX1_help_add_edid_to_icc) + 1);
        sprintf( t, "%s\n%s%s%s%s%s", oyX1_help, oyX1_help_list,
                 oyX1_help_properties, oyX1_help_setup, oyX1_help_unset,
                 oyX1_help_add_edid_to_icc );
      }
      return t;
    }
  } else if(strcmp(select, "manufacturer")==0)
  {
         if(type == oyNAME_NICK)
      return _("Kai-Uwe");
    else if(type == oyNAME_NAME)
      return _("Kai-Uwe Behrmann");
    else
      return _("Oyranos project; www: http://www.oyranos.com; support/email: ku.b@gmx.de; sources: http://www.oyranos.com/wiki/index.php?title=Oyranos/Download");
  } else if(strcmp(select, "copyright")==0)
  {
         if(type == oyNAME_NICK)
      return _("newBSD");
    else if(type == oyNAME_NAME)
      return _("Copyright (c) 2005-2009 Kai-Uwe Behrmann; newBSD");
    else
      return _("new BSD license: http://www.opensource.org/licenses/bsd-license.php");
  }
  return 0;
}
const char *oyX1_texts[5] = {"name","copyright","manufacturer","help",0};

/** @instance oyX1_cmm_module
 *  @brief    oyX1 module infos
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/12 (Oyranos: 0.1.8)
 *  @date    2008/12/30
 */
oyCMMInfo_s oyX1_cmm_module = {

  oyOBJECT_CMM_INFO_S,
  0,0,0,
  CMM_NICK,
  "0.3",
  oyX1GetText, /* oyCMMInfoGetText_f get Text */
  (char**)oyX1_texts, /* texts; list of arguments to getText */
  OYRANOS_VERSION,

  (oyCMMapi_s*) & oyX1_api8,

  {oyOBJECT_ICON_S, 0,0,0, 0,0,0, "oyranos_logo.png"},
};

