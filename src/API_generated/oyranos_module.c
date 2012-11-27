/** @file oyranos_module.c

   [Template file inheritance graph]
   +-- oyranos_module.template.c

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2012 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2012/10/25
 */


#include "oyranos_debug.h"
#include "oyranos_i18n.h"
#include "oyranos_io.h"
#include "oyranos_internal.h"
#include "oyranos_object_internal.h"
#include "oyranos_types.h"

#include "oyranos_generic_internal.h"
#include "oyranos_module_internal.h"
#include "oyObject_s.h"
#include "oyCMMapi3_s.h"
#include "oyCMMapi4_s_.h"
#include "oyCMMapi5_s.h"
#include "oyCMMapi6_s.h"
#include "oyCMMapi9_s.h"
#include "oyCMMapi10_s_.h"
#include "oyCMMapiFilter_s_.h"
#include "oyCMMapiFilters_s.h"
#include "oyCMMapiFilters_s_.h"
#include "oyCMMapis_s.h"
#include "oyOptions_s_.h"
#include "oyStructList_s_.h"

#ifdef HAVE_POSIX
#include <dlfcn.h>
#else
#include <ltdl.h>
#define dlopen  lt_dlopen
#define dlsym   lt_dlsym
#define dlerror lt_dlerror
#define dlclose lt_dlclose
#endif

oyStructList_s * oy_cmm_handles_ = 0;
/* defined in sources/Struct.public_methods_definitions.c */
extern const char * (*oyStruct_GetTextFromModule_p) (
                                       oyStruct_s        * obj,
                                       oyNAME_e            name_type,
                                       uint32_t            flags );

/** Function  oyCMMsGetFilterApis_
 *  @brief    Let a oyCMMapi5_s meta module open a set of modules
 *  @internal
 *
 *  The oyCMMapiLoadxxx_ function family loads a API from a external module.\n
 *  The module system shall support:
 *    - use of the desired CMM for the task at hand
 *    - provide fallbacks for incapabilities
 *    - process in different ways and by different modules through the same API
 *    - select complementing API's
 *
 *  We have modules with well defined capabilities and some with fuzzy ones.\n
 *  For instance the X11 API's is well defined and we can use it, once it is
 *  loaded.\n
 *  A CMM for colour conversion has often limitations or certain features,
 *  which makes the search for alternatives desireable. So we search
 *  for match to our automatic criteria.\n
 *
 *
 *  This function allowes to obtain API's for matching modules/CMM's.
 *  The modules are filtered for double occurencies. The highest rank or
 *  version is selected.
 *
 *  @param[in]   cmm_meta              the selected meta API will be used,
 *                                     can be Zero or at least 4 bytes long,
 *                                     e.g. "oyIM" for the library module loader
 *  @param[in]   cmm_required          if present take this or fail, the arg
 *                                     simplifies and speeds up the search
 *  @param[in]   registration          point'.' separated list of identifiers
 *  @param[in]   type                  CMM API
 *  @param[in]   flags                 supported is
 *                                     - oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR
 *                                       for omitting implementation attributes
 *  @param[out]  rank_list             the ranks matching the returned list;
 *                                     without that only the most matching API
 *                                     is returned at position 0
 *  @param[out]  count                 count of returned modules
 *  @return                            a zero terminated list of modules
 *
 *  @version  Oyranos: 0.9.0
 *  @date     2012/10/25
 *  @since    2008/12/19 (Oyranos: 0.1.10)
 */
oyCMMapiFilters_s * oyCMMsGetFilterApis_(const char        * cmm_meta,
                                         const char        * cmm_required,
                                         const char        * registration,
                                         oyOBJECT_e          type,
                                         uint32_t            flags,
                                         uint32_t         ** rank_list,
                                         uint32_t          * count )
{
  int error = !oyIsOfTypeCMMapiFilter( type );
  oyCMMapiFilters_s * apis = 0, * apis2 = 0;
  oyCMMapiFilter_s * api = 0,
                   * api2 = 0;
  oyCMMapiFilter_s_ ** api_ = (oyCMMapiFilter_s_**)&api;
  oyCMMapiFilter_s_ ** api2_ = (oyCMMapiFilter_s_**)&api2;
  uint32_t * rank_list_ = 0, * rank_list2_ = 0;
  int rank_list_n = 5, count_ = 0;
  oyObject_s object = oyObject_New();
  oyHash_s * entry = 0;

  if(!rank_list)
  {
    char * hash_text = 0;

    /* query cache */
    oyStringAddPrintf_( &hash_text, oyAllocateFunc_,oyDeAllocateFunc_,
                "oyCMMapiFilters_s:meta:%s;required:%s;reg:%s;type:%u;flags:%u",
                        oyNoEmptyString_m_( cmm_meta ),
                        oyNoEmptyString_m_( cmm_required ),
                        oyNoEmptyString_m_( registration ),
                        type,
                        flags );
    entry = oyCMMCacheListGetEntry_( hash_text );
    /* release hash string */
    oyDeAllocateFunc_( hash_text ); hash_text = 0;
    apis = (oyCMMapiFilters_s*) oyHash_GetPointer( entry,
                                                   oyOBJECT_CMM_API_FILTERS_S );
    if(apis)
    {
      int i,n = oyCMMapiFilters_Count( apis );

      if(count)
        *count = n;

      /* copy the result into a new container */
      if(n)
        apis2 = oyCMMapiFilters_New(0);
      for( i = 0; i < n; ++i)
      {
        api = oyCMMapiFilters_Get( apis, i );
        oyCMMapiFilters_MoveIn( apis2, &api, -1 );
      }

      oyHash_Release( &entry );
      oyCMMapiFilters_Release( &apis );

      /* return cached result */
      return apis2;
    }
  }

  if(error <= 0)
  {
    oyCMMapi5_s_ * api5 = 0;
    oyCMMapis_s * meta_apis = oyCMMGetMetaApis_( cmm_meta );
    int meta_apis_n = 0;
    char ** files = 0;
    uint32_t  files_n = 0;
    int i, j, k = 0, match_j = -1, ret, match_i = -1, rank = 0, old_rank = 0,
        n, accept;
    char * match = 0, * reg = 0;
    oyCMMinfo_s * info = 0;

    meta_apis_n = oyCMMapis_Count( meta_apis );
    for(k = 0; k < meta_apis_n; ++k)
    {
      api5 = (oyCMMapi5_s_*) oyCMMapis_Get( meta_apis, k );

      files_n = 0;

      if(!api5 && error <= 0)
        error = 1;

      if(error <= 0)
        files = oyCMMsGetNames_(&files_n, api5->sub_paths, api5->ext,
                                cmm_required,
                                api5->data_type == 0 ? oyPATH_MODULE :
                                                       oyPATH_SCRIPT);
      else
        WARNc2_S("%s: %s", _("Could not open meta module API"),
                 oyNoEmptyString_m_( registration ));

      if(files)
      for( i = 0; i < files_n; ++i)
      {
        ret = 0; j = 0;
        ret = api5->oyCMMFilterScan( 0,0, files[i], type, j,
                                     0, 0, oyAllocateFunc_, (oyCMMinfo_s**)&info, object );
        while(!ret)
        {
          ret = api5->oyCMMFilterScan( 0,0, files[i], type, j,
                                       &reg, 0, oyAllocateFunc_, 0, 0 );
          if(!ret && reg)
          {
            rank = oyFilterRegistrationMatch( reg, registration, type );
            if((rank && OYRANOS_VERSION == oyCMMinfo_GetCompatibility( info )) ||
               !registration)
              ++rank;

            if(rank && rank_list)
            {

              if(!rank_list_ && !apis)
              {
                *rank_list = 0;
                oyAllocHelper_m_( *rank_list, uint32_t, rank_list_n+1, 0,
                                  goto clean );
                rank_list_ = *rank_list;
                apis = oyCMMapiFilters_New(0);
              } else
              if(count_ >= rank_list_n)
              {
                rank_list_n *= 2;
                rank_list_ = 0;
                oyAllocHelper_m_( rank_list_, uint32_t, rank_list_n+1, 0,
                                  goto clean );
                error = !memcpy( rank_list_, *rank_list,
                                 sizeof(uint32_t) * rank_list_n/2 );
                oyFree_m_(*rank_list);
                *rank_list = rank_list_;
              }

              rank_list_[k++] = rank;
              api = api5->oyCMMFilterLoad( 0,0, files[i], type, j);
              if(!api)
                continue;

              if(!(*api_)->id_)
                (*api_)->id_ = oyStringCopy_( files[i], oyAllocateFunc_ );
              (*api_)->api5_ = api5;
              if(!apis)
                apis = oyCMMapiFilters_New( 0 );
              oyCMMapiFilters_MoveIn( apis, &api, -1 );
              ++ count_;
              if(count)
                *count = count_;

            } else
            if(rank > old_rank)
            {
              match = reg;
              match_j = j;
              match_i = i;
              old_rank = rank;
            }
          }
          ++j;
          if(reg) oyFree_m_( reg );
        }
      }

      oyCMMinfo_Release( (oyCMMinfo_s**)&info );
#if 0
      if(api5->release)
        api5->release( (oyStruct_s**)&api5 );
#endif
    }

    oyCMMapis_Release( &meta_apis );

    if(match && !rank_list)
    {
      apis2 = oyCMMapiFilters_New( 0 );
      api = api5->oyCMMFilterLoad( 0,0, files[match_i], type, match_j );
      if(!(*api_)->id_)
        (*api_)->id_ = oyStringCopy_( files[match_i], oyAllocateFunc_ );
      (*api_)->api5_ = api5;
      oyCMMapiFilters_MoveIn( apis2, &api, -1 );
      if(count)
        *count = 1;
    }

    if(rank_list_)
    {
      /* filter doubled entries */
      n = oyCMMapiFilters_Count( apis );
      oyAllocHelper_m_( rank_list2_, uint32_t, rank_list_n+1, 0, goto clean );
      k = 0;
      for(i = 0 ; i < n; ++i)
      {
        char * apir = 0;

        api = oyCMMapiFilters_Get( apis, i );
        if(flags | oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR)
          oyFilterRegistrationModify( (*api_)->registration,
                                    oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR,
                                      &apir, 0 );
        accept = 1;

        for(j = i+1; j < n; ++j)
        {
          char * api2r = 0;

          api2 = oyCMMapiFilters_Get( apis, j );
          if(flags | oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR)
            oyFilterRegistrationModify( (*api2_)->registration,
                                    oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR,
                                        &api2r, 0 );

          /* for equal registration compare rank and version */
          if(oyStrcmp_( apir,  api2r ) == 0 &&
             rank_list_[i] <= rank_list_[j])
            accept = 0;

          oyFree_m_(api2r);

          if(api2->release)
            api2->release( (oyStruct_s**)&api2 );
        }

        if(!apis2)
          apis2 = oyCMMapiFilters_New( 0 );

        if(accept)
        {
          oyCMMapiFilters_MoveIn( apis2, &api, -1 );
          rank_list2_[k++] = rank_list_[i];
        }

        oyFree_m_(apir);

        if(api && api->release)
          api->release( (oyStruct_s**)&api );
      }

      if(rank_list)
        *rank_list = rank_list2_;
      if(rank_list_)
        oyFree_m_(rank_list_);
      if(count)
        *count = k;
    }

    oyStringListRelease_( &files, files_n, oyDeAllocateFunc_ );
  }

  if(error <= 0 && apis2 && entry)
  {
    error = oyHash_SetPointer( entry, (oyStruct_s*) apis2 );
  }

  clean:
    oyObject_Release( &object );
    oyHash_Release( &entry );

  return apis2;
}

/** @internal
 *  Function oyCMMsGetFilterApi_
 *  @brief let a oyCMMapi5_s meta module open a module
 *
 *  The oyCMMapiLoadxxx_ function family loads a API from a external module.\n
 *  The module system shall support:
 *    - use of the desired CMM for the task at hand
 *    - provide fallbacks for incapabilities
 *    - process in different ways and by different modules through the same API
 *
 *  We have modules with well defined capabilities and some with fuzzy ones.\n
 *  For instance the X11 API's is well defined and we can use it, once it is
 *  loaded.\n
 *  A CMM for colour conversion has often limitations or certain features,
 *  which makes the search for alternatives desireable. So we search
 *  for match to our automatic criteria.\n
 *
 *
 *  This function allowes to obtain a API for a matching modul/CMM.
 *
 *  @param[in]   cmm_required          if present take this or fail, the arg
 *                                     simplifies and speeds up the search
 *  @param[in]   registration          point'.' separated list of identifiers
 *  @param[in]   type                  CMM API
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/15 (Oyranos: 0.1.9)
 *  @date    2008/06/02
 */
oyCMMapiFilter_s *oyCMMsGetFilterApi_( const char        * cmm_required,
                                       const char        * registration,
                                       oyOBJECT_e          type )
{
  oyCMMapiFilter_s * api = 0;
  oyCMMapiFilters_s * apis = 0;
  oyHash_s * entry = 0;
#ifndef NO_OPT
  int len = oyStrlen_(registration);
  char * hash_text = oyAllocateFunc_( len + 10 );
  char api_char = oyCMMapiNumberToChar( type );

  oySprintf_( hash_text, "%s.%c_", registration, api_char ? api_char:' ');
  entry = oyCMMCacheListGetEntry_( hash_text );
  oyDeAllocateFunc_( hash_text ); hash_text = 0;
  api = (oyCMMapiFilter_s*) oyHash_GetPointer( entry, type );
  if(api)
  {
    oyHash_Release( &entry );
    return api;
  }
#endif

  apis = oyCMMsGetFilterApis_( 0,cmm_required, registration, type, 
                               oyFILTER_REG_MODE_NONE, 0,0 );

  if(apis)
  {
    api = oyCMMapiFilters_Get( apis, 0 );
    oyCMMapiFilters_Release( &apis );
#ifndef NO_OPT
#ifdef NOT_YET
    if(api->copy)
      api = api->copy
#endif
    oyHash_SetPointer( entry, (oyStruct_s*) api );
#endif
  }

  oyHash_Release( &entry );

  return api;
}

/** @internal
 *  Function oyCMMsGetApi__
 *  @brief get a specified module
 *
 *  The oyCMMapiLoadxxx_ function family loads a API from a external module.\n
 *
 *  This function allowes to obtain a desired API from a certain library.
 *
 *  @param[in]   type                  the API to return
 *  @param[in]   lib_name              if present take this or fail, the arg
 *                                     simplifies and speeds up the search
 *  @param[in]   apiCheck              custom api selector
 *  @param[in]   check_pointer         data to pass to apiCheck
 *  @param[in]   num                   position in api chain matching to type
 *                                     and apiCheck/check_pointer starting from
 *                                     zero,
 *                                     -1 means: pick the first match, useful
 *                                     in case the API position is known or to
 *                                     iterate through all matching API's
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/08 (Oyranos: 0.1.9)
 *  @date    2008/12/23
 */
oyCMMapi_s *     oyCMMsGetApi__      ( oyOBJECT_e          type,
                                       const char        * lib_name,
                                       oyCMMapi_Check_f    apiCheck,
                                       oyPointer           check_pointer,
                                       int                 num )
{
  int error = !type,
      i = 0;
  oyCMMapi_s * api = 0;
  uint32_t rank = 0;

  if(error <= 0 &&
     !(oyOBJECT_CMM_API1_S <= type && type < oyOBJECT_CMM_API_MAX))
    error = 1;

  if(error <= 0)
  {
    /* open the module */
    oyCMMinfo_s * cmm_info = oyCMMinfoFromLibName_( lib_name );

    if(cmm_info)
    {
      oyCMMapi_s * tmp = oyCMMinfo_GetApi( cmm_info );

      if(!apiCheck)
        apiCheck = oyCMMapi_CheckWrap_;

      while(tmp)
      {
        if(apiCheck(cmm_info, tmp, check_pointer, &rank) == type)
        {
          if((num >= 0 && num == i) ||
             num < 0 )
            api = tmp;

          ++i;
        }
        tmp = oyCMMapi_GetNext(tmp);
      }
    }

    oyCMMinfo_Release( &cmm_info );
  }

  return api;
}

/** @internal
 *  Function oyCMMsGetApi_
 *  @brief get a oyranos module
 *
 *  The oyCMMapiLoadxxx_ function family loads a API from a external module.\n
 *  The module system shall support:
 *    - use of the desired CMM for the task at hand
 *    - provide fallbacks for incapabilities
 *    - process in different ways and by different modules through the same API
 *
 *  We have modules with well defined capabilities and some with fuzzy ones.\n
 *  For instance the X11 API's is well defined and we can use it, once it is
 *  loaded.\n
 *  A CMM for colour conversion has often limitations or certain features,
 *  which make it desireable. So we have to search for match to our automatic
 *  criteria.\n
 *
 *
 *  This function allowes to obtain a API for a certain modul/CMM.
 *  oyCMMapi4_s is excluded.
 *
 *  @param[in]   type                  the API type to return
 *  @param[in]   cmm_required          if present take this or fail, the arg
 *                                     simplifies and speeds up the search
 *  @param[out]  lib_used              inform about the selected CMM
 *  @param[in]   apiCheck              custom API selector
 *  @param[in]   check_pointer         data to pass to apiCheck
 *
 *  @version Oyranos: 0.1.9
 *  @since   2007/12/12 (Oyranos: 0.1.9)
 *  @date    2008/12/16
 */
oyCMMapi_s *     oyCMMsGetApi_       ( oyOBJECT_e          type,
                                       const char        * cmm_required,
                                       char             ** lib_used,
                                       oyCMMapi_Check_f    apiCheck,
                                       oyPointer           check_pointer )
{
  int error = !type;
  oyCMMapi_s * api = 0;

  if(!apiCheck)
    apiCheck = oyCMMapi_CheckWrap_;

  if(error <= 0 &&
     !(oyOBJECT_CMM_API1_S <= type && type < oyOBJECT_CMM_API_MAX))
    error = 1;

  if(error <= 0)
  {
    char ** files = 0;
    uint32_t  files_n = 0;
    int i, oy_compatibility = 0;
    uint32_t rank = 0,
             max_rank = 0;
    int max_pos = -1;

    files = oyCMMsGetLibNames_(&files_n, cmm_required);

    /* open the modules */
    for( i = 0; i < files_n; ++i)
    {
      oyCMMinfo_s * cmm_info = oyCMMinfoFromLibName_(files[i]);

      if(cmm_info)
      {
        oyCMMapi_s * tmp = oyCMMinfo_GetApi( cmm_info );

        while(tmp)
        {

          if(apiCheck(cmm_info, tmp, check_pointer, &rank) == type)
          {

              if( /* if we found already a matching version, do not exchange*/
                  oy_compatibility != OYRANOS_VERSION &&
                    /* possibly newly found */
                  ( oy_compatibility = 0 ||
                    /* or a bigger version but not greater than current oy_ */
                    ( oyCMMinfo_GetCompatibility( cmm_info ) <= OYRANOS_VERSION &&
                      oy_compatibility < oyCMMinfo_GetCompatibility( cmm_info ) ) ||
                    /* or we select a less greater in case we are above oy_ */
                    ( oyCMMinfo_GetCompatibility( cmm_info ) > OYRANOS_VERSION &&
                      oy_compatibility > oyCMMinfo_GetCompatibility( cmm_info ) )
                  )
                )
              {
                if(oyCMMinfo_GetCompatibility( cmm_info ) == OYRANOS_VERSION)
                  ++rank;
                if(rank > max_rank)
                {
                  api = tmp;
                  max_rank = rank;
                  max_pos = i;

                }
                oy_compatibility = oyCMMinfo_GetCompatibility( cmm_info );
              }
          }
          tmp = oyCMMapi_GetNext(tmp);
        }
      }

      oyCMMinfo_Release( &cmm_info );
    }

    if(max_rank >= 0 && lib_used)
    {
      if(*lib_used)
        oyFree_m_(*lib_used);
      if(files && files_n)
        *lib_used = oyStringCopy_( files[max_pos], oyAllocateFunc_ );
    }

    oyStringListRelease_( &files, files_n, oyDeAllocateFunc_ );
  }

  return api;
}

/** @internal
 *  The lists are allocated one time and live until the application quits
 *  It contains the various caches for faster access of CPU intentsive data.\n
 *  We'd need a 3 dimensional table to map\n
 *    A: a function or resource type\n
 *    B: a CMM\n
 *    C: a hash value specifying under which conditions the resource was build\n
 *  The resulting cache entry is the result from the above 3 arguments. With the
 *  much implementation work and expectedly small to no speed advantage it is
 *  not a good strategy. Even though argument C would be extensible.\n
 *  \n
 *  A different approach would use two values to map the search request to the
 *  cache entry. The above hash map and the hash or combination of the two other *  values.\
 *  \n
 *  One point to consider is a readable string to end not with a anonymous list
 *  full of anonymous entries, where a user can pretty much nothing know.
 *  A transparent approach has to allow for easy identifying each entry.\n
 *  A help would be a function to compute both a md5 digest and a message from
 *  the 3 arguments outlined above. Probably it would allow much more arguments
 *  to add as we can not know how many optins and other influential parameters
 *  the cache entry depends on.\n
 *  \n
 *  A final implementation would consist of a
 *  - function to convert arbitrary (string) arguments to a single string and a
 *    hash sum
 *  - a function to create a cache entry struct from above hash and a the
 *    according description string plus the oyPointer_s struct. For simplicity
 *    the cache struct can be identical to the oyPointer_s, with the disadvantage
 *    of containing additional data not understandable for a CMM. We need to
 *    mark these data (hash + description) as internal to Oyranos.
 *  - a list that hold above cache entry stucts
 *  - several functions to reference, release, maps a hash value to the
 *    cached resource
 *
 *  @since Oyranos: version 0.1.8
 *  @date  23 november 2007 (API 0.1.8)
 */
oyStructList_s * oy_cmm_cache_ = 0;

/** @internal
 *  @brief get always a Oyranos cache entry from the CMM's cache
 *
 *
 *  @since Oyranos: version 0.1.8
 *  @date  november 2007 (API 0.1.8)
 */
oyHash_s *   oyCMMCacheListGetEntry_ ( const char        * hash_text)
{
  if(!oy_cmm_cache_)
    oy_cmm_cache_ = oyStructList_New( 0 );

  if(oy_debug > 3)
    DBGs1_S(oy_cmm_cache_,"%s", hash_text)

  return oyCacheListGetEntry_(oy_cmm_cache_, 0, hash_text);
}

/** @internal
 *  @brief get the Oyranos CMM cache
 *
 *
 *  @since Oyranos: version 0.1.8
 *  @date  17 december 2007 (API 0.1.8)
 */
oyStructList_s** oyCMMCacheList_()
{
  return &oy_cmm_cache_;
}

/** @internal
 *  @brief get the Oyranos CMM cache
 *
 *
 *  @since Oyranos: version 0.1.8
 *  @date  17 december 2007 (API 0.1.8)
 */
oyChar* oyCMMCacheListPrint_()
{
  oyStructList_s ** cache_list = oyCMMCacheList_();
  int n = oyStructList_Count( *cache_list ), i;
  oyChar * text = 0;
  oyChar refs[80];

  oySprintf_( refs,"%s:%d Oyranos CMM cache with %d entries:\n", 
              __FILE__,__LINE__, n);
  STRING_ADD( text, refs );

  for(i = 0; i < n ; ++i)
  {
    oyHash_s * compare = (oyHash_s*) oyStructList_GetType_((oyStructList_s_*)*cache_list, i,
                                                         oyOBJECT_HASH_S );

    if(compare)
    {
      oySprintf_(refs,"%d: ", compare->oy_->ref_);
      STRING_ADD( text, refs );
      STRING_ADD( text, oyObject_GetName(compare->oy_, oyNAME_NAME) );
      STRING_ADD( text, "\n" );
    }
  }

  return text;
}



/** Function oyCMMGetMetaApis_
 *  @brief   Get module loaders
 *  @internal
 *
 *  This function searches for a meta modul API.
 *
 *  @param[in]   cmm                   the four byte module ID, optional
 *
 *  @version Oyranos: 0.1.11
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2010/09/14
 */
oyCMMapis_s * oyCMMGetMetaApis_     ( const char        * cmm )
{
  oyCMMapis_s * meta_apis = 0;
  meta_apis = oyCMMsGetMetaApis_( cmm );
  return meta_apis;
}

/** @internal
 *  Function  oyCMMsGetMetaApis_
 *  @brief    Get oyranos modules
 *
 *  @param[in]   cmm                  the selected module, optional
 *
 *  @version  Oyranos: 0.9.0
 *  @date     2012/10/25
 *  @since    2010/06/25 (Oyranos: 0.1.10)
 */
oyCMMapis_s *    oyCMMsGetMetaApis_  ( const char        * cmm )
{
  int error = 0;
  oyCMMapis_s * apis = 0;
  oyCMMapi_Check_f apiCheck = oyCMMapi_CheckWrap_;

  oyHash_s * entry;
  char * hash_text = 0;

  /* query cache */
  STRING_ADD( hash_text, "oyCMMapis_s:" );
  STRING_ADD( hash_text, oyNoEmptyString_m_( cmm ) );
  entry = oyCMMCacheListGetEntry_( hash_text );
  /* release hash string */
  oyDeAllocateFunc_( hash_text ); hash_text = 0;
  apis = (oyCMMapis_s*) oyHash_GetPointer( entry, oyOBJECT_CMM_APIS_S );
  if(apis)
  {
    oyHash_Release( &entry );
    /* return cached result */
    return apis;
  }

  if(error <= 0)
  {
    char ** files = 0;
    uint32_t  files_n = 0;
    int i = 0;

    files = oyCMMsGetLibNames_(&files_n, cmm);

    if(!files_n)
      WARNc_S(_("Could not find any meta module. "
                "Did you set the OY_MODULE_PATH variable,"
                " to point to a Oyranos module loader library?"));

    /* open the modules */
    for( i = 0; i < files_n; ++i)
    {
      oyCMMinfo_s * cmm_info = oyCMMinfoFromLibName_(files[i]);

      if(cmm_info)
      {
        oyCMMapi_s * tmp = oyCMMinfo_GetApi( cmm_info ),
                   * copy = 0;

        while(tmp)
        {

          if(apiCheck(cmm_info, tmp, 0, 0) == oyOBJECT_CMM_API5_S)
          {
            if(!apis)
              apis = oyCMMapis_New(0);

            copy = tmp;

            oyCMMapis_MoveIn( apis, &copy, -1 );
          }

          tmp = oyCMMapi_GetNext(tmp);
        }
      }

      oyCMMinfo_Release( &cmm_info );
    }

    oyStringListRelease_( &files, files_n, oyDeAllocateFunc_ );
  }

  if(error <= 0 && apis)
    error = oyHash_SetPointer( entry, (oyStruct_s*) apis );
  oyHash_Release( &entry );

  return apis;
}

/** @internal
 *  @brief release Oyranos CMM dlopen handle
 *
 *  use in pair in oyCMMdsoReference_
 *
 *  @version Oyranos: 0.1.8
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 *  @date    2008/12/08
 */
int          oyCMMdsoRelease_      ( const char        * lib_name )
{
  int error = 0;

  /* inactive */
#if 0
  int found = -1;

  if(!oy_cmm_handles_)
    return 1;

  if(oy_cmm_handles_->type_ != oyOBJECT_STRUCT_LIST_S)
    error = 1;

  if(error <= 0)
    found = oyCMMdsoSearch_(cmm);

  if(found >= 0)
    oyStructList_ReleaseAt(oy_cmm_handles_, found);
#endif
  return error;
}

/** @internal
 *  @brief get Oyranos CMM dlopen handle
 *
 *  Search the cache for a handle and return it. Or dlopen the library and 
 *  store and reference the handle in the cache.
 *
 *  @since Oyranos: version 0.1.8
 *  @date  23 november 2007 (API 0.1.8)
 */
oyPointer    oyCMMdsoGet_            ( const char        * cmm,
                                       const char        * lib_name )
{
  int found = -1;
  oyPointer dso_handle = 0;

if(!lib_name)
    return 0;

#ifdef HAVE_POSIX
  found = oyCMMdsoSearch_(lib_name);

  if(found >= 0)
  {
    oyPointer_s * s = (oyPointer_s*)oyStructList_GetType_( (oyStructList_s_*)oy_cmm_handles_, found,
                                                  oyOBJECT_POINTER_S );

    if(s)
      dso_handle = oyPointer_GetPointer(s);
  }

  if(!dso_handle)
  {
    dso_handle = dlopen( lib_name, RTLD_LAZY );

    if(!dso_handle)
    {
      WARNc2_S( "\n  dlopen( %s, RTLD_LAZY):\n  \"%s\"", lib_name, dlerror() );
      system("  echo $LD_LIBRARY_PATH");
    }

    /* initialise module type lookup */
    if(!oyStruct_GetTextFromModule_p)
      oyStruct_GetTextFromModule_p = oyStruct_GetTextFromModule;
  }

  if(dso_handle)
    oyCMMdsoReference_( lib_name, dso_handle );

  return dso_handle;
#endif
}

/** @internal
 *  @brief internal Oyranos module handle list
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
oyStructList_s * oy_cmm_infos_ = 0;

/** @internal
 *  @brief search a Oyranos module handle in a internal list
 *
 *  a intermediate step
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
oyCMMhandle_s *  oyCMMFromCache_     ( const char        * lib_name )
{
  int error = !lib_name;
  int n, i;
  oyCMMhandle_s * cmm_handle = 0;

  if(error <= 0 && !oy_cmm_infos_)
  {
    oy_cmm_infos_ = oyStructList_New( 0 );
    error = !oy_cmm_infos_;
  }

  if(error <= 0 && oy_cmm_infos_->type_ != oyOBJECT_STRUCT_LIST_S)
    error = 1;

  n = oyStructList_Count(oy_cmm_infos_);
  if(error <= 0)
  for(i = 0; i < n; ++i)
  {
    oyCMMhandle_s * cmmh = (oyCMMhandle_s*) oyStructList_GetType_((oyStructList_s_*)oy_cmm_infos_,
                                                i, oyOBJECT_CMM_HANDLE_S );
    oyCMMinfo_s * s = 0;

    if(cmmh)
      s = (oyCMMinfo_s*) cmmh->info;

    if( s && s->type_ == oyOBJECT_CMM_INFO_S &&
        !oyStrcmp_( cmmh->lib_name, lib_name ) )
    {
      cmm_handle = oyCMMhandle_Copy_( cmmh, 0 );
      error = oyStructList_ReferenceAt_( (oyStructList_s_*)oy_cmm_infos_, i );
      if(!error)
        break;
    }
  }

  return cmm_handle;
}

/** @internal
 *
 *  @since Oyranos: version 0.1.8
 *  @date  11 december 2007 (API 0.1.8)
 */
char *           oyCMMnameFromLibName_(const char        * lib_name)
{
  char * cmm = 0;

  if(lib_name && lib_name[0])
  {
    const char * tmp = oyStrstr_( lib_name, OY_MODULE_NAME );

    if(!tmp && oyStrlen_( lib_name ) == 4)
      return oyStringCopy_( lib_name, oyAllocateFunc_ );

    if(tmp)
    {
      cmm = oyAllocateFunc_(5);
      memcpy(cmm, tmp-4, 4);
      cmm[4] = 0;
    }
  }

  return cmm;
}



/** @internal
 *  @brief compare a library name with a Oyranos CMM
 *
 *  @version Oyranos: 0.1.9
 *  @since   2007/12/08 (Oyranos: 0.1.9)
 *  @date    2008/12/08
 */
int              oyCMMlibMatchesCMM  ( const char        * lib_name,
                                       const char        * cmm )
{
  int matches = 0;
  char * tmp = oyCMMnameFromLibName_( lib_name );

  if(oyStrcmp_(tmp, cmm) == 0)
    matches = 1;

  oyFree_m_( tmp );

  return matches;
}

/** @internal
 *  @brief get all CMM/module/script names
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/16 (Oyranos: 0.1.9)
 *  @date    2010/06/28
 */
char **          oyCMMsGetNames_     ( uint32_t          * n,
                                       const char        * sub_path,
                                       const char        * ext,
                                       const char        * required_cmm,
                                       oyPATH_TYPE_e       path_type )
{
  int error = !n;
  char ** files = 0,
       ** sub_paths = 0;
  int sub_paths_n = 0;

  if(error <= 0)
  {
    int  files_n = 0, i;
    char * lib_string = oyAllocateFunc_(24);
    const char * cmm = 0;

    lib_string[0] = 0;
    if(required_cmm)
    {
      /* assuming a library file can not be smaller than the signature + 
         OY_MODULE_NAME + file extension */
      if(oyStrlen_(required_cmm) < 5)
        cmm = required_cmm;

      if(cmm)
        oySprintf_( lib_string, "%s%s", cmm, OY_MODULE_NAME );
      else
        oyStringListAddStaticString_( &files, &files_n, required_cmm,
                                       oyAllocateFunc_, oyDeAllocateFunc_ );

    } else
      oySprintf_( lib_string, "%s", OY_MODULE_NAME );

    sub_paths = oyStringSplit_( sub_path, ':', &sub_paths_n, 0 );

    /* search for a matching module file */
    for(i = 0; i < sub_paths_n; ++i)
    {
      if(!files)
      {
        if(path_type == oyPATH_MODULE)
          files = oyLibFilesGet_( &files_n, sub_paths[i], oyUSER_SYS,
                                  0, lib_string, 0, oyAllocateFunc_ );
        else if(path_type == oyPATH_SCRIPT)
          files = oyDataFilesGet_( &files_n, sub_paths[i], oyYES, oyUSER_SYS,
                                   0, lib_string, ext, oyAllocateFunc_ );
      }
    }
    error = !files;

    if(!error)
      *n = files_n;

    if(sub_paths_n && sub_paths)
      oyStringListRelease_( &sub_paths, sub_paths_n, oyDeAllocateFunc_ );

    if( lib_string )
      oyFree_m_(lib_string);
  }

  return files;
}

/** @internal
 *  @brief get all module names
 *
 *  @version Oyranos: 0.1.9
 *  @since   2007/12/12 (Oyranos: 0.1.8)
 *  @date    2008/12/16
 */
char **          oyCMMsGetLibNames_  ( uint32_t          * n,
                                       const char        * required_cmm )
{
#ifdef HAVE_POSIX
  return oyCMMsGetNames_(n, OY_METASUBPATH, 0, required_cmm, oyPATH_MODULE);
#else
  *n = 0;
  return 0;
#endif
}

/** @internal
 *  @brief pick from cache or ldopen a CMM and get the Oyranos module infos
 *
 *  @since Oyranos: version 0.1.8
 *  @date  5 december 2007 (API 0.1.8)
 */
oyCMMinfo_s *    oyCMMOpen_          ( const char        * lib_name )
{
  oyCMMinfo_s * cmm_info = 0;
  oyCMMapi_s * api = 0;
  oyCMMhandle_s * cmm_handle = 0;
  int error = !lib_name;
  char * cmm = oyCMMnameFromLibName_(lib_name);
  const char * error_text = 0;

  if(error <= 0)
  {
    oyPointer dso_handle = 0;

    if(error <= 0)
    {
      if(lib_name)
        dso_handle = oyCMMdsoGet_(cmm, lib_name);

      error = !dso_handle;

      if(error)
      {
        error_text = dlerror();
        WARNc2_S( "\n  error while dlopen'ing lib:\n    %s\n  dlerror(): %s",
                  lib_name, error_text ? error_text : "no text" );
      }
    }

    /* open the module */
    if(error <= 0)
    {
      char * info_sym = oyAllocateFunc_(24);
      int api_found = 0;

      oySprintf_( info_sym, "%s%s", cmm, OY_MODULE_NAME );

      cmm_info = (oyCMMinfo_s*) dlsym (dso_handle, info_sym);

      if(info_sym)
        oyFree_m_(info_sym);

      error = !cmm_info;

      if(error)
        WARNc_S(dlerror());

      if(error <= 0)
        if(oyCMMapi_Check_( oyCMMinfo_GetApi( cmm_info ) ))
          api = oyCMMinfo_GetApi( cmm_info );

      if(error <= 0 && api)
      {
        error = oyCMMapi_GetMessageFuncSetF(api)( oyMessageFunc_p );

        cmm_handle = oyCMMhandle_New_(0);

        /* init */
        if(error <= 0)
        error = oyCMMapi_GetInitF(api)( (oyStruct_s*) api );
        if(error <= 0)
          error = oyCMMhandle_Set_( cmm_handle, cmm_info, dso_handle, lib_name);

        api_found = 1;
      }

      /* store */
      if(error <= 0 && api_found)
        oyStructList_MoveIn(oy_cmm_infos_, (oyStruct_s**)&cmm_handle, -1, 0);
    }

    oyCMMdsoRelease_( lib_name );
  }

  if(cmm)
    oyDeAllocateFunc_(cmm);
  cmm = 0;

  return cmm_info;
}

/** @internal
 *  @brief pick from cache or ldopen a CMM and get the Oyranos module infos
 *
 *  @since Oyranos: version 0.1.8
 *  @date  5 december 2007 (API 0.1.8)
 */
oyCMMinfo_s *    oyCMMinfoFromLibName_(const char        * lib_name )
{
  oyCMMinfo_s * cmm_info = 0;
  oyCMMhandle_s * cmm_handle = 0;
  int error = !lib_name;
  int found = 0;

  if(error <= 0)
  {
    cmm_handle = oyCMMFromCache_( lib_name );
    if(cmm_handle && oyCMMinfo_GetApi( cmm_handle->info ))
    {
      cmm_info = cmm_handle->info;
      found = 1;
    }
  }

  if(error <= 0 && !found)
  {
    cmm_info = oyCMMOpen_(lib_name);
  }

  return cmm_info;
}

/** @internal
 *  @brief Oyranos wrapper for dlopen
 *
 *  @since Oyranos: version 0.1.8
 *  @date  28 november 2007 (API 0.1.8)
 */
int oyDlclose(oyPointer* handle)
{
  if(handle && *handle)
  {
    dlclose(*handle);
    *handle = 0;
    return 0;
  }
  return 1;
}

/** @internal
 *  @brief register Oyranos CMM dlopen handle
 *
 *  use in pair with oyCMMdsoRelease_
 *
 *  @since Oyranos: version 0.1.8
 *  @date  november 2007 (API 0.1.8)
 */
int          oyCMMdsoReference_    ( const char        * lib_name,
                                     oyPointer           ptr )
{
  int i, n;
  int found = 0;
  int error = 0;

  if(!oy_cmm_handles_)
  {
    oy_cmm_handles_ = oyStructList_New( 0 );
    error = !oy_cmm_handles_;
  }

  if(error <= 0 && oy_cmm_handles_->type_ != oyOBJECT_STRUCT_LIST_S)
    error = 1;

  n = oyStructList_Count(oy_cmm_handles_);
  if(error <= 0)
  for(i = 0; i < n; ++i)
  {
    oyStruct_s * obj = oyStructList_Get_((oyStructList_s_*)oy_cmm_handles_, i);
    oyPointer_s * s;

    if(obj && obj->type_ == oyOBJECT_POINTER_S)
      s = (oyPointer_s*) obj;
    else
      s = 0;

    if( s && oyPointer_GetLibName( s ) && lib_name &&
        !oyStrcmp_( oyPointer_GetLibName( s ), lib_name ) )
    {
      found = 1;
      oyStructList_ReferenceAt_((oyStructList_s_*)oy_cmm_handles_, i);
      if(ptr)
      {
        if(!oyPointer_GetPointer( s ))
          oyPointer_Set( s, 0, 0, ptr, 0, 0 );
        /*else
          WARNc_S(("Attempt to register dso handle multiple times."));*/
      }
    }
  }

  if(!found)
  {
    oyPointer_s * s = oyPointer_New(0);
    oyStruct_s * oy_cmm_struct = 0;

    error = !s;

    if(error <= 0)
      error = oyPointer_Set( s, lib_name, 0, ptr, "oyDlclose", oyDlclose );

    if(error <= 0)
      oy_cmm_struct = (oyStruct_s*) s;

    if(error <= 0)
      oyStructList_MoveIn(oy_cmm_handles_, &oy_cmm_struct, -1, 0);
  }

  return error;
}

/** @internal
 *  @brief search a dlopen handle for a Oyranos CMM
 *
 *  @since Oyranos: version 0.1.8
 *  @date  23 november 2007 (API 0.1.8)
 */
int          oyCMMdsoSearch_         ( const char        * lib_name )
{
  int i, n;
  int pos = -1;
  int error = 0;

  if(!oy_cmm_handles_)
    return 1;

  if(oy_cmm_handles_->type_ != oyOBJECT_STRUCT_LIST_S)
    error = 1;

  n = oyStructList_Count(oy_cmm_handles_);
  if(error <= 0)
  for(i = 0; i < n; ++i)
  {
    oyStruct_s * obj = oyStructList_Get_((oyStructList_s_*)oy_cmm_handles_, i);
    oyPointer_s * s = 0;

    if(obj && obj->type_ == oyOBJECT_POINTER_S)
      s = (oyPointer_s*) obj;

    error = !s;

    if(error <= 0)
    if( oyPointer_GetLibName(s) && lib_name &&
        !oyStrcmp_( oyPointer_GetLibName(s), lib_name ) )
      pos = i;
  }

  return pos;
}

/** @internal
 */
char *           oyCMMinfoPrint_     ( oyCMMinfo_s       * cmm_info,
                                       int                 simple )
{
  char * text = 0, num[48];
  oyCMMapi_s * tmp = 0;
  oyOBJECT_e type = 0;

  if(!cmm_info || cmm_info->type_ != oyOBJECT_CMM_INFO_S)
    return oyStringCopy_("---\n", oyAllocateFunc_);

  oySprintf_(num,"%d", oyCMMinfo_GetCompatibility( cmm_info ) );

  STRING_ADD( text, oyCMMinfo_GetCMM(cmm_info) );
  STRING_ADD( text, " " );
  STRING_ADD( text, oyCMMinfo_GetVersion(cmm_info) );
  STRING_ADD( text, "/" );
  STRING_ADD( text, num );
  if(simple) { STRING_ADD( text, "\n" ); return text; }
  STRING_ADD( text, ":" );

#define CMMINFO_ADD_NAME_TO_TEXT( name_, select ) \
  STRING_ADD( text, "\n  " ); \
  STRING_ADD( text, name_ ); \
  STRING_ADD( text, ":\n    " ); \
  STRING_ADD( text, oyCMMinfo_GetTextF(cmm_info)( select, oyNAME_NICK, (oyStruct_s*) cmm_info ) ); \
  STRING_ADD( text, "\n    " ); \
  STRING_ADD( text, oyCMMinfo_GetTextF(cmm_info)( select, oyNAME_NAME, (oyStruct_s*) cmm_info ) ); \
  STRING_ADD( text, "\n    " ); \
  STRING_ADD( text, oyCMMinfo_GetTextF(cmm_info)( select, oyNAME_DESCRIPTION, (oyStruct_s*) cmm_info)); \
  STRING_ADD( text, "\n" );

  CMMINFO_ADD_NAME_TO_TEXT( _("Name"), "name" )
  CMMINFO_ADD_NAME_TO_TEXT( _("Manufacturer"), "manufacturer" )
  CMMINFO_ADD_NAME_TO_TEXT( _("Copyright"), "copyright" )

      if(cmm_info)
      {
        tmp = oyCMMinfo_GetApi( cmm_info );
        STRING_ADD( text, "\n  API(s):\n" );

        while(tmp)
        {
          type = oyCMMapi_Check_(tmp);

          oySprintf_(num,"    %d:", type );
          STRING_ADD( text, num );
          STRING_ADD( text, oyStructTypeToText( tmp->type_ ) );

          if(type == oyOBJECT_CMM_API4_S)
          {
            STRING_ADD( text, "\n    Registration: " );
            STRING_ADD( text, oyCMMapi_GetRegistration( tmp ) );
            CMMINFO_ADD_NAME_TO_TEXT( "Name", "name" )
          }
          STRING_ADD( text, "\n" );

          tmp = oyCMMapi_GetNext(tmp);
        }
      }
  STRING_ADD( text, "\n" );

  return text;
}


/** Function  oyStruct_GetTextFromModule
 *  @memberof oyStruct_s
 *  @brief    get object infos from a module
 *
 *  @param[in,out] obj                 the objects structure
 *  @param[in]     name_type           the type
 *  @param[in]     flags               @see oyStruct_GetText
 *  @return                            the text
 *
 *  @version  Oyranos: 0.3.3
 *  @since    2009/09/15 (Oyranos: 0.3.3)
 *  @date     2011/10/31
 */
const char * oyStruct_GetTextFromModule (
                                       oyStruct_s        * obj,
                                       oyNAME_e            name_type,
                                       uint32_t            flags )
{
  int error = !obj;
  const char * text = 0;

  if(!error)
    text = oyObject_GetName( obj->oy_, oyNAME_NICK );

  if(!error && !text)
  {
    if(obj->type_)
    {
      oyCMMapiFilters_s * apis;
      int apis_n = 0, i,j,n;
      oyCMMapi9_s * cmm_api9 = 0;
      char * api_reg = 0;

      apis = oyCMMsGetFilterApis_( 0,0, api_reg, oyOBJECT_CMM_API9_S,
                                   oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR,
                                   0, 0);
      apis_n = oyCMMapiFilters_Count( apis );
      for(i = 0; i < apis_n; ++i)
      {
        cmm_api9 = (oyCMMapi9_s*) oyCMMapiFilters_Get( apis, i );

        n = 0;
        if(oyCMMapi9_GetObjectTypes( cmm_api9 ) != NULL)
        while( oyCMMapi9_GetObjectTypes(cmm_api9)[n] )
          ++n;
        for(j = 0; j < n; ++j)
          if( oyCMMobjectType_GetTextF( oyCMMapi9_GetObjectTypes(cmm_api9)[j] ) &&
              oyCMMobjectType_GetId( oyCMMapi9_GetObjectTypes(cmm_api9)[j] ) == obj->type_ )
          {
            text = oyCMMobjectType_GetTextF( oyCMMapi9_GetObjectTypes(cmm_api9)[j] )(
                                      flags?0:obj, name_type, 0 );
            if(text)
              break;
          }

        if(cmm_api9->release)
          cmm_api9->release( (oyStruct_s**)&cmm_api9 );

        if(text)
          break;
      }
      oyCMMapiFilters_Release( &apis );
    }
  }

  if(!error && !text)
    text = oyStructTypeToText( obj->type_ );

  return text;
}

/** @internal
 *  Function oyCMMCanHandle_
 *  @brief query a module for certain capabilities
 *
 *  @return                           sum of asked capabilities
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/01/03 (Oyranos: 0.1.8)
 *  @date    2009/09/02
 */
int              oyCMMCanHandle_    ( oyCMMapi3_s        * api,
                                      oyCMMapiQueries_s  * queries )
{
  int capable = 0;
  int error = !api;
  int i, ret = 0;

  if(error <= 0)
  {
    if(queries)
    {
      for(i = 0; i < queries->n; ++i)
      {
        oyCMMapiQuery_s * query = queries->queries[i];
        ret = oyCMMapi3_GetCanHandleF( api )( query->query, query->value );

        if(ret)
          capable += query->request;
        else if(query->request == oyREQUEST_HARD)
          return 0;
      }

    } else
      capable = 1;
  }

  return capable;
}



/** @internal
 *  @brief new module handle
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
oyCMMhandle_s *    oyCMMhandle_New_    ( oyObject_s        object )
{
  /* ---- start of common object constructor ----- */
  oyOBJECT_e type = oyOBJECT_CMM_HANDLE_S;
# define STRUCT_TYPE oyCMMhandle_s
  int error = 0;
  oyObject_s    s_obj = oyObject_NewFrom( object );
  STRUCT_TYPE * s = 0;
  
  if(s_obj)
    s = (STRUCT_TYPE*)s_obj->allocateFunc_(sizeof(STRUCT_TYPE));

  if(!s || !s_obj)
  {
    WARNc_S(_("MEM Error."));
    return NULL;
  }

  error = !memset( s, 0, sizeof(STRUCT_TYPE) );
  if(error)
    WARNc_S("memset failed");

  s->type_ = type;
  s->copy = (oyStruct_Copy_f) oyCMMhandle_Copy_;
  s->release = (oyStruct_Release_f) oyCMMhandle_Release_;

  s->oy_ = s_obj;

  error = !oyObject_SetParent( s_obj, type, (oyPointer)s );
# undef STRUCT_TYPE
  /* ---- end of common object constructor ------- */

  return s;
}

/** @internal
 *  @brief copy module handle 
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
oyCMMhandle_s *  oyCMMhandle_Copy_     ( oyCMMhandle_s   * handle,
                                         oyObject_s        object )
{
  int error = !handle;
  oyCMMhandle_s * s = 0;

  if(error <= 0 && object)
  {
    s = oyCMMhandle_New_(object);
    error = !s;
    if(error <= 0)
    {
      if(handle)
        WARNc1_S("Dont know how to copy CMM[%s] handle.", handle->lib_name);

      error = oyCMMhandle_Set_( s, 0, 0, 0 );
      if(!error)
        oyCMMhandle_Release_( &s );

    } else
      WARNc_S("Could not create a new object.");

  } else if(handle)
  {
    oyObject_Copy( handle->oy_ );
    s = handle;
  }

  return s;
}

/** @internal
 *  @brief release module handle 
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
int              oyCMMhandle_Release_( oyCMMhandle_s    ** obj )
{
  int error = 0;
  /* ---- start of common object destructor ----- */
  oyCMMhandle_s * s = 0;

  if(!obj || !*obj)
    return 0;

  s = *obj;

  oyCheckType__m( oyOBJECT_CMM_HANDLE_S, return 1 )

  *obj = 0;

  if(oyObject_UnRef(s->oy_))
    return 0;
  /* ---- end of common object destructor ------- */

  error = oyCMMdsoRelease_( s->lib_name );

  s->dso_handle = 0;
  s->info = 0;
  oyFree_m_( s->lib_name );


  if(s->oy_->deallocateFunc_)
  {
    oyDeAlloc_f deallocateFunc = s->oy_->deallocateFunc_;

    oyObject_Release( &s->oy_ );

    deallocateFunc( s );
  }

  return error;
}

/** @internal
 */
int              oyCMMhandle_Set_    ( oyCMMhandle_s     * handle,
                                       oyCMMinfo_s       * info,
                                       oyPointer           dso_handle,
                                       const char        * lib_name )
{
  int error = 0;

  if(!handle || !info || !dso_handle)
    error = 1;

  if(error <= 0)
  {
    handle->info = info;
    handle->dso_handle = dso_handle;
    handle->lib_name = oyStringCopy_( lib_name, handle->oy_->allocateFunc_ );
    oyCMMdsoReference_( handle->lib_name, dso_handle );
  }

  return error;
}

oyStructList_s * oy_meta_module_cache_ = 0;
oyStructList_s * oy_cmm_filter_cache_ = 0;



/** @internal
 *  @brief release Oyranos module infos
 *
 *  @since Oyranos: version 0.1.8
 *  @date  6 december 2007 (API 0.1.8)
 */
int              oyCMMRelease_       ( const char        * cmm )
{
  int error = !cmm;
  int n = oyStructList_Count( oy_cmm_infos_ );
  int i;

  n = oyStructList_Count(oy_cmm_infos_);
  if(error <= 0)
  for(i = 0; i < n; ++i)
  {
    oyCMMinfo_s * s = 0;
    oyCMMhandle_s * cmmh = (oyCMMhandle_s *) oyStructList_GetType_(
                                               (oyStructList_s_*)oy_cmm_infos_,
                                               i,
                                               oyOBJECT_CMM_HANDLE_S );

    if(cmmh)
      s = (oyCMMinfo_s*) cmmh->info;

    if( s && s->type_ == oyOBJECT_CMM_INFO_S &&
        *((uint32_t*)oyCMMinfo_GetCMM(s)) && cmm &&
        !memcmp( oyCMMinfo_GetCMM(s), cmm, 4 ) )
    {
      oyCMMhandle_Release_( &cmmh );
      oyStructList_ReleaseAt( oy_cmm_infos_, 0 );
    }
  }

  return error;
}

/** @internal
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/30 (Oyranos: 0.1.10)
 *  @date    2009/01/30
 */
int    oyIsOfTypeCMMapiFilter        ( oyOBJECT_e          type )
{
  return        type == oyOBJECT_CMM_API4_S ||
                type == oyOBJECT_CMM_API6_S ||
                type == oyOBJECT_CMM_API7_S ||
                type == oyOBJECT_CMM_API8_S ||
                type == oyOBJECT_CMM_API9_S ||
                type == oyOBJECT_CMM_API10_S;
}

/**
 *  @brief oyDATATYPE_e to byte mapping
 *
 *  @version Oyranos: 0.9.0
 *  @date    2012/10/22
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 */
size_t             oyDataTypeGetSize ( oyDATATYPE_e        data_type )
{
  size_t n = 0;
  switch(data_type)
  {
    case oyUINT8:
         return 1;
    case oyUINT16:
    case oyHALF:
         return 2;
    case oyUINT32:
    case oyFLOAT:
         return 4;
    case oyDOUBLE:
         return 8;
  }
  return n;
}

/**
 *  @brief oyDATATYPE_e to string mapping
 *
 *  @version Oyranos: 0.9.0
 *  @date    2012/10/22
 *  @since   2007/11/26 (Oyranos: 0.1.8)
 */

const char *       oyDataTypeToText  ( oyDATATYPE_e        data_type )
{
  const char * text = 0;
  switch(data_type)
  {
    case oyUINT8:
         text = "oyUINT8"; break;
    case oyUINT16:
         text = "oyUINT16"; break;
    case oyHALF:
         text = "oyHALF"; break;
    case oyUINT32:
         text = "oyUINT32"; break;
    case oyFLOAT:
         text = "oyFLOAT"; break;
    case oyDOUBLE:
         text = "oyDOUBLE"; break;
  }
  return text;
}


uint32_t     oyCMMtoId               ( const char        * cmm )
{
  if(cmm)
    return oyValueUInt32( *(uint32_t*)cmm );
  else
    return 0;
}
int          oyIdToCMM               ( uint32_t            cmmId,
                                       char              * cmm )
{
  cmmId = oyValueUInt32( cmmId );

  if(cmm)
    return !memcpy( cmm, &cmmId, 4 );
  else
    return 0;
}

/** Function  oyPointer_LookUpFromObject
 *  @brief    Get a module specific pointer from cache
 *  @memberof oyPointer_s
 *
 *  The returned oyPointer_s has to be released after using by the module with
 *  oyPointer_Release().
 *  In case the the oyPointer_s::ptr member is empty, it should be set by the
 *  requesting module.
 *
 *  @see oyPointer_LookUpFromText()
 *
 *  @param[in]     data                 object to look up
 *  @param[in]     data_type            four byte module type for this object
 *                                      type; The data_type shall enshure the
 *                                      returned oyPointer_s is specific to the
 *                                      calling module.
 *  @return                             the CMM specific oyPointer_s; It is owned
 *                                      by the CMM.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2009/11/05
 */
oyPointer_s  * oyPointer_LookUpFromObject (
                                       oyStruct_s        * data,
                                       const char        * data_type )
{
  oyStruct_s * s = data;
  int error = !s;
  oyPointer_s * cmm_ptr = 0;

  if(error <= 0 && !data_type)
    error = !data_type;

  if(error <= 0)
  {
    const char * tmp = 0;
    tmp = oyObject_GetName( s->oy_, oyNAME_NICK );
    cmm_ptr = oyPointer_LookUpFromText( tmp, data_type );
  }

  return cmm_ptr;
}

/** Function  oyPointer_LookUpFromText
 *  @brief    Get a module specific pointer from cache
 *  @memberof oyPointer_s
 *
 *  The returned oyPointer_s has to be released after using by the module with
 *  oyPointer_Release().
 *  In case the the oyPointer_s::ptr member is empty, it should be set by the
 *  requesting module.
 *
 *  @see e.g. lcmsCMMData_Open()
 *
 *  @param[in]     text                 hash text to look up
 *  @param[in]     data_type            four byte module type for this object
 *                                      type; The data_type shall enshure the
 *                                      returned oyPointer_s is specific to the
 *                                      calling module.
 *  @return                             the CMM specific oyPointer_s; It is owned
 *                                      by the CMM.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/11/05 (Oyranos: 0.1.10)
 *  @date    2009/11/05
 */
oyPointer_s * oyPointer_LookUpFromText( const char        * text,
                                       const char        * data_type )
{
  int error = !text;
  oyPointer_s * cmm_ptr = 0;

  if(error <= 0 && !data_type)
    error = !data_type;

  if(error <= 0)
  {
    /*oyPointer_s *cmm_ptr = 0;*/
    const char * tmp = 0;

    oyHash_s * entry = 0;
    oyChar * hash_text = 0;

    /** Cache Search \n
     *  1.     hash from input \n
     *  2.     query for hash in cache \n
     *  3.     check \n
     *  3a.       eighter take cache entry or \n
     *  3b.       update cache entry
     */

    /* 1. create hash text */
    STRING_ADD( hash_text, data_type );
    STRING_ADD( hash_text, ":" );
    tmp = text;
    STRING_ADD( hash_text, tmp );

    /* 2. query in cache */
    entry = oyCMMCacheListGetEntry_( hash_text );

    if(error <= 0)
    {
      /* 3. check and 3.a take*/
      cmm_ptr = (oyPointer_s*) oyHash_GetPointer( entry,
                                                  oyOBJECT_POINTER_S);

      if(!cmm_ptr)
      {
        cmm_ptr = oyPointer_New( 0 );
        error = !cmm_ptr;

        if(error <= 0)
          error = oyPointer_Set( cmm_ptr, 0,
                                 data_type, 0, 0, 0 );

        error = !cmm_ptr;

        if(error <= 0 && cmm_ptr)
          /* 3b.1. update cache entry */
          error = oyHash_SetPointer( entry,
                                     (oyStruct_s*) cmm_ptr );
      }
    }

    oyHash_Release( &entry );
  }

  return cmm_ptr;
}


/** Function  oyOptions_Handle
 *  @brief    Handle a request by a module
 *
 *  @param[in]     registration        the module selector
 *  @param[in]     options             options
 *  @param[in]     command             the command to handle
 *  @param[out]    result              options to the policy module
 *  @return                            0 - indifferent, >= 1 - error,
 *                                     <= -1 - issue,
 *                                     + a message should be sent
 *
 *  @version  Oyranos: 0.9.0
 *  @date     2012/10/25
 *  @since    2009/12/11 (Oyranos: 0.1.10)
 */
int             oyOptions_Handle     ( const char        * registration,
                                       oyOptions_s       * options,
                                       const char        * command,
                                       oyOptions_s      ** result )
{
  int error = 0;
  oyOptions_s * s = options;

  if(!options && !command)
    return error;

  oyCheckType__m( oyOBJECT_OPTIONS_S, return 1 )

  if(!error)
  {
    oyCMMapiFilters_s * apis;
    int apis_n = 0, i, found = 0;
    oyCMMapi10_s_ * cmm_api10 = 0;
    char * test = 0;

    STRING_ADD( test, "can_handle." );
    if(command && command[0])
      STRING_ADD( test, command );

    apis = oyCMMsGetFilterApis_( 0,0, registration, oyOBJECT_CMM_API10_S,
                                 oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR,
                                 0,0 );

    apis_n = oyCMMapiFilters_Count( apis );
    if(test)
      for(i = 0; i < apis_n; ++i)
      {
        cmm_api10 = (oyCMMapi10_s_*) oyCMMapiFilters_Get( apis, i );

        if(oyFilterRegistrationMatch( cmm_api10->registration, registration, 0))
        {
          if(cmm_api10->oyMOptions_Handle)
          {
            error = cmm_api10->oyMOptions_Handle( s, test, result );
            if(error == 0)
            {
              found = 1;
              error = cmm_api10->oyMOptions_Handle( s, command, result );
            }

          } else
            error = 1;

          if(error > 0)
          {
            WARNc2_S( "%s %s",_("error in module:"), cmm_api10->registration );
          }
        }

        if(cmm_api10->release)
          cmm_api10->release( (oyStruct_s**)&cmm_api10 );
      }
    else
      WARNc2_S( "%s %s",_("Could not allocate memory for:"),
                cmm_api10->registration );

    oyFree_m_( test );
    oyCMMapiFilters_Release( &apis );
    if(!found && error == 0)
      error = -1;
  }

  return error;
}


/** @internal
 *  @brief   convert between oyPointer_s data
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */
int          oyPointer_ConvertData   ( oyPointer_s       * cmm_ptr,
                                       oyPointer_s       * cmm_ptr_out,
                                       oyFilterNode_s    * node )
{
  int error = !cmm_ptr || !cmm_ptr_out;
  oyCMMapi6_s * api6 = 0;
  char * reg = 0, * tmp = 0;

  if(error <= 0)
  {
    reg = oyStringCopy_( "//", oyAllocateFunc_ );
    tmp = oyFilterRegistrationToText( oyFilterNode_GetRegistration(node),
                                      oyFILTER_REG_TYPE,0);
    STRING_ADD( reg, tmp );
    oyFree_m_( tmp );
    STRING_ADD( reg, "/" );
    STRING_ADD( reg, oyPointer_GetResourceName( cmm_ptr ) );
    STRING_ADD( reg, "_" );
    STRING_ADD( reg, oyPointer_GetResourceName( cmm_ptr_out ) );

    api6 = (oyCMMapi6_s*) oyCMMsGetFilterApi_( 0, reg, oyOBJECT_CMM_API6_S );

    error = !api6;
  }

  if(error <= 0 && oyCMMapi6_GetConvertF(api6))
    error = oyCMMapi6_GetConvertF(api6)( cmm_ptr, cmm_ptr_out, node );
  else
    error = 1;

  if(error)
    WARNc_S("Could not convert context");

  return error;
}

/** @internal
 *  Function oyAdjacencyListAdd_
 *  @brief   get a graphs adjacency list
 *
 *  Try to add an edge, if not yet found in the scanned graph.
 *  If the edge/plug was added, return success.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/25 (Oyranos: 0.1.10)
 *  @date    2009/03/06
 */
int    oyAdjacencyListAdd_           ( oyFilterPlug_s    * plug,
                                       oyFilterNodes_s   * nodes,
                                       oyFilterPlugs_s   * edges,
                                       const char        * selector,
                                       int                 flags )
{
  int added = 0, found = 0,
      i,n;
  oyFilterPlug_s * p = 0;
  oyFilterSocket_s * remote_socket = oyFilterPlug_GetSocket( plug );
  oyFilterNode_s * node = 0,
                 * plug_node = oyFilterPlug_GetNode( plug ),
                 * remote_socket_node = oyFilterSocket_GetNode( remote_socket );
  oyOptions_s * plug_node_tags = oyFilterNode_GetTags( plug_node ),
              * remote_socket_node_tags = oyFilterNode_GetTags( remote_socket_node );

  if(selector &&
     (oyOptions_FindString( plug_node_tags, selector, 0 ) == 0 &&
      oyOptions_FindString( remote_socket_node_tags, selector,0) == 0))
    goto oyAdjacencyListAdd_Clean;

  n = oyFilterPlugs_Count( edges );
  for(i = 0; i < n; ++i)
  {
    p = oyFilterPlugs_Get( edges, i );
    if(oyObject_GetId( p->oy_ ) == oyObject_GetId( plug->oy_ ))
      found = 1;
    oyFilterPlug_Release( &p );
  }

  if(!found)
  {
    p = oyFilterPlug_Copy(plug, 0);
    oyFilterPlugs_MoveIn( edges, &p, -1 );
    added = !found;


    found = 0;
    {
      n = oyFilterNodes_Count( nodes );
      for(i = 0; i < n; ++i)
      {
        node = oyFilterNodes_Get( nodes, i );
        if(oyObject_GetId( plug_node->oy_ ) == oyObject_GetId( node->oy_))
          found = 1;
        oyFilterNode_Release( &node );
      }
      if(!found)
      {
        node = oyFilterNode_Copy( plug_node, 0 );
        oyFilterNodes_MoveIn( nodes, &node, -1 );
      }
    }

    {
      found = 0;
      n = oyFilterNodes_Count( nodes );
      for(i = 0; i < n; ++i)
      {
        node = oyFilterNodes_Get( nodes, i );
        if(remote_socket && remote_socket_node)
        {
          if(oyObject_GetId( remote_socket_node->oy_ ) ==
             oyObject_GetId( node->oy_ ))
            found = 1;

        } else
          found = 1;

        oyFilterNode_Release( &node );
      }
      if(!found)
      {
        node = oyFilterNode_Copy( remote_socket_node, 0 );
        oyFilterNodes_MoveIn( nodes, &node, -1 );
      }
    }
  }

  oyAdjacencyListAdd_Clean:
  oyFilterNode_Release( &remote_socket_node );
  oyFilterNode_Release( &plug_node );
  oyFilterSocket_Release( &remote_socket );
  oyOptions_Release( &plug_node_tags );
  oyOptions_Release( &remote_socket_node_tags );

  return added;
}

/** Function oyFilterRegistrationModify
 *  @brief   process a registration string
 *
 *  A semantical overview is given in @ref module_api.
 *
 *  @param[in]     registration        registration key
 *  @param[in]     mode                the processing rule
 *  @param[out]    result              allocated by allocateFunc
 *  @param[in]     allocateFunc        optional user allocator; defaults to 
 *                                     oyAllocateFunc_
 *  @return                            0 - good; >= 1 - error; < 0 issue
 *
 *  @version Oyranos: 0.1.11
 *  @since   2010/08/12 (Oyranos: 0.1.11)
 *  @date    2010/08/12
 */
char   oyFilterRegistrationModify    ( const char        * registration,
                                       oyFILTER_REG_MODE_e mode,
                                       char             ** result,
                                       oyAlloc_f           allocateFunc )
{
  if(!result) return 1;
  if(!registration) return -1;

  if(!allocateFunc)
    allocateFunc = oyAllocateFunc_;

  switch(mode)
  {
  case oyFILTER_REG_MODE_STRIP_IMPLEMENTATION_ATTR:
  {
    char ** reg_texts = 0;
    int     reg_texts_n = 0;
    char ** regc_texts = 0;
    int     regc_texts_n = 0;
    char  * reg_text = 0;
    int     i,j;
 
    reg_texts = oyStringSplit_( registration, OY_SLASH_C, &reg_texts_n,
                                oyAllocateFunc_);

    for( i = 0; i < reg_texts_n; ++i)
    {
      regc_texts_n = 0;

      /* level by level */
      regc_texts = oyStringSplit_( reg_texts[i],'.',&regc_texts_n,
                                   oyAllocateFunc_);

      if(i > 0)
        STRING_ADD( reg_text, OY_SLASH );

      for( j = 0; j < regc_texts_n; ++j)
      {
        /* '_' underbar is the feature char; omit such attributes */
        if(regc_texts[j][0] != '_')
        {
          if(j > 0)
            STRING_ADD( reg_text, "." );
          STRING_ADD( reg_text, regc_texts[j] );
        }
      }

      oyStringListRelease_( &regc_texts, regc_texts_n, oyDeAllocateFunc_ );
    }
    oyStringListRelease_( &reg_texts, reg_texts_n, oyDeAllocateFunc_ );


    if(allocateFunc != oyAllocateFunc_)
      *result = oyStringCopy_( reg_text, allocateFunc );
    else
      *result = reg_text;
  }
  break;

  default: return -1;
  }

  return 0;
}

char **     oyDataFilesGet_          ( int             * count,
                                       const char      * subdir,
                                       int               data,
                                       int               owner,
                                       const char      * dir_string,
                                       const char      * string,
                                       const char      * suffix,
                                       oyAlloc_f         allocateFunc )
{
  int l_n = 0;
  char** l = oyFileListGet_(subdir, &l_n, data, owner);
  int filt_n = 0;
  char ** filt = oyStringListFilter_( (const char**)l, l_n, 
                                      dir_string, string, suffix, &filt_n,
                                      allocateFunc );

  if(l && *l)
    oyStringListRelease_(&l, l_n, oyDeAllocateFunc_);

  if(count)
    *count = filt_n;
  return filt;
}

char **     oyLibFilesGet_           ( int             * count,
                                       const char      * subdir,
                                       int               owner,
                                       const char      * dir_string,
                                       const char      * string,
                                       const char      * suffix,
                                       oyAlloc_f         allocateFunc )
{
  int l_n = 0;
  char** l = oyLibListGet_(subdir, &l_n, owner);
  int filt_n = 0;
  char ** filt = oyStringListFilter_( (const char**)l, l_n, 
                                      dir_string, string, suffix, &filt_n,
                                      allocateFunc );

  if(l && *l)
    oyStringListRelease_(&l, l_n, oyDeAllocateFunc_);

  if(count)
    *count = filt_n;
  return filt;
}
