/** @file oyCMMapi3_s_.c

   [Template file inheritance graph]
   +-> oyCMMapi3_s_.template.c
   |
   +-> oyCMMapi_s_.template.c
   |
   +-- Base_s_.c

 *  Oyranos is an open source Color Management System
 *
 *  @par Copyright:
 *            2004-2015 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/BSD-3-Clause
 */



  
#include "oyCMMapi3_s.h"
#include "oyCMMapi3_s_.h"


#include "oyCMMapi_s_.h"




#include "oyObject_s.h"
#include "oyranos_object_internal.h"
#include "oyranos_module_internal.h"



#include "oyCMMapiFilter_s_.h"
  


/* Include "CMMapi3.private_custom_definitions.c" { */
/** Function    oyCMMapi3_Release__Members
 *  @memberof   oyCMMapi3_s
 *  @brief      Custom CMMapi3 destructor
 *  @internal
 *
 *  This function will free up all memmory allocated by the
 *  input object. First all object members witch have their
 *  own release method are deallocated. Then the deallocateFunc_
 *  of the oy_ object is used to release the rest of the members
 *  that were allocated with oy_->allocateFunc_.
 *
 *  @param[in]  cmmapi3  the CMMapi3 object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
void oyCMMapi3_Release__Members( oyCMMapi3_s_ * cmmapi3 )
{
  /* Deallocate members here
   * E.g: oyXXX_Release( &cmmapi3->member );
   */

  if(cmmapi3->oy_->deallocateFunc_)
  {
#if 0
    oyDeAlloc_f deallocateFunc = cmmapi3->oy_->deallocateFunc_;
#endif

    /* Deallocate members of basic type here
     * E.g.: deallocateFunc( cmmapi3->member );
     */
  }
}

/** Function    oyCMMapi3_Init__Members
 *  @memberof   oyCMMapi3_s
 *  @brief      Custom CMMapi3 constructor 
 *  @internal
 *
 *  This function will allocate all memmory for the input object.
 *  For the basic member types this is done using the allocateFunc_
 *  of the attatced (oyObject_s)oy_ object.
 *
 *  @param[in]  cmmapi3  the CMMapi3 object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyCMMapi3_Init__Members( oyCMMapi3_s_ * cmmapi3 )
{
  return 0;
}

/** Function    oyCMMapi3_Copy__Members
 *  @memberof   oyCMMapi3_s
 *  @brief      Custom CMMapi3 copy constructor
 *  @internal
 *
 *  This function makes a copy of all values from the input
 *  to the output object. The destination object and all of its
 *  members should already be allocated.
 *
 *  @param[in]   src  the oyCMMapi3_s_ input object
 *  @param[out]  dst  the output oyCMMapi3_s_ object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyCMMapi3_Copy__Members( oyCMMapi3_s_ * dst, oyCMMapi3_s_ * src)
{
  if(!dst || !src)
    return 1;

  /* Copy each value of src to dst here */

  return 0;
}

/* } Include "CMMapi3.private_custom_definitions.c" */



/** @internal
 *  Function oyCMMapi3_New_
 *  @memberof oyCMMapi3_s_
 *  @brief   allocate a new oyCMMapi3_s_  object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi3_s_ * oyCMMapi3_New_ ( oyObject_s object )
{
  /* ---- start of common object constructor ----- */
  oyOBJECT_e type = oyOBJECT_CMM_API3_S;
  int error = 0;
  oyObject_s    s_obj = oyObject_NewFrom( object );
  oyCMMapi3_s_ * s = 0;

  if(s_obj)
    s = (oyCMMapi3_s_*)s_obj->allocateFunc_(sizeof(oyCMMapi3_s_));

  if(!s || !s_obj)
  {
    WARNc_S(_("MEM Error."));
    return NULL;
  }

  error = !memset( s, 0, sizeof(oyCMMapi3_s_) );
  if(error)
    WARNc_S( "memset failed" );

  memcpy( s, &type, sizeof(oyOBJECT_e) );
  s->copy = (oyStruct_Copy_f) oyCMMapi3_Copy;
  s->release = (oyStruct_Release_f) oyCMMapi3_Release;

  s->oy_ = s_obj;

  
  
  /* ---- start of custom CMMapi constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_CMM_API_S, (oyPointer)s );
  /* ---- end of custom CMMapi constructor ------- */
  /* ---- start of custom CMMapi3 constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_CMM_API3_S, (oyPointer)s );
  /* ---- end of custom CMMapi3 constructor ------- */
  
  
  
  /* ---- end of common object constructor ------- */
  if(error)
    WARNc_S( "oyObject_SetParent failed" );


  
  

  
  
  /* ---- start of custom CMMapi constructor ----- */
  error += oyCMMapi_Init__Members( (oyCMMapi_s_*)s );
  /* ---- end of custom CMMapi constructor ------- */
  /* ---- start of custom CMMapi3 constructor ----- */
  error += oyCMMapi3_Init__Members( s );
  /* ---- end of custom CMMapi3 constructor ------- */
  
  
  

  if(error)
    WARNc1_S("%d", error);

  return s;
}

/** @internal
 *  Function oyCMMapi3_Copy__
 *  @memberof oyCMMapi3_s_
 *  @brief   real copy a CMMapi3 object
 *
 *  @param[in]     cmmapi3                 CMMapi3 struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi3_s_ * oyCMMapi3_Copy__ ( oyCMMapi3_s_ *cmmapi3, oyObject_s object )
{
  oyCMMapi3_s_ *s = 0;
  int error = 0;

  if(!cmmapi3 || !object)
    return s;

  s = (oyCMMapi3_s_*) oyCMMapi3_New( object );
  error = !s;

  if(!error) {
    
    
    /* ---- start of custom CMMapi copy constructor ----- */
    error = oyCMMapi_Copy__Members( (oyCMMapi_s_*)s, (oyCMMapi_s_*)cmmapi3 );
    /* ---- end of custom CMMapi copy constructor ------- */
    /* ---- start of custom CMMapi3 copy constructor ----- */
    error = oyCMMapi3_Copy__Members( s, cmmapi3 );
    /* ---- end of custom CMMapi3 copy constructor ------- */
    
    
    
    
    
  }

  if(error)
    oyCMMapi3_Release_( &s );

  return s;
}

/** @internal
 *  Function oyCMMapi3_Copy_
 *  @memberof oyCMMapi3_s_
 *  @brief   copy or reference a CMMapi3 object
 *
 *  @param[in]     cmmapi3                 CMMapi3 struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi3_s_ * oyCMMapi3_Copy_ ( oyCMMapi3_s_ *cmmapi3, oyObject_s object )
{
  oyCMMapi3_s_ *s = cmmapi3;

  if(!cmmapi3)
    return 0;

  if(cmmapi3 && !object)
  {
    s = cmmapi3;
    
    oyObject_Copy( s->oy_ );
    return s;
  }

  s = oyCMMapi3_Copy__( cmmapi3, object );

  return s;
}
 
/** @internal
 *  Function oyCMMapi3_Release_
 *  @memberof oyCMMapi3_s_
 *  @brief   release and possibly deallocate a CMMapi3 object
 *
 *  @param[in,out] cmmapi3                 CMMapi3 struct object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
int oyCMMapi3_Release_( oyCMMapi3_s_ **cmmapi3 )
{
  /* ---- start of common object destructor ----- */
  oyCMMapi3_s_ *s = 0;

  if(!cmmapi3 || !*cmmapi3)
    return 0;

  s = *cmmapi3;

  *cmmapi3 = 0;

  if(oyObject_UnRef(s->oy_))
    return 0;
  /* ---- end of common object destructor ------- */

  
  
  /* ---- start of custom CMMapi destructor ----- */
  oyCMMapi_Release__Members( (oyCMMapi_s_*)s );
  /* ---- end of custom CMMapi destructor ------- */
  /* ---- start of custom CMMapi3 destructor ----- */
  oyCMMapi3_Release__Members( s );
  /* ---- end of custom CMMapi3 destructor ------- */
  
  
  



  if(s->oy_->deallocateFunc_)
  {
    oyDeAlloc_f deallocateFunc = s->oy_->deallocateFunc_;

    oyObject_Release( &s->oy_ );

    deallocateFunc( s );
  }

  return 0;
}



/* Include "CMMapi3.private_methods_definitions.c" { */
/** Function    oyCMMapi3_Query_
 *  @memberof   oyCMMapi3_s
 *  @brief      Implements oyCMMapi_Check_f
 *  @internal
 *
 *  The data argument is expected to be oyCMMapiQueries_s.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/09/02 (Oyranos: 0.1.10)
 *  @date    2009/09/02
 */
oyOBJECT_e   oyCMMapi3_Query_        ( oyCMMinfo_s_      * cmm_info,
                                       oyCMMapi_s_       * api,
                                       oyPointer           data,
                                       uint32_t          * rank )
{
  oyCMMapiQueries_s * queries = data;
  uint32_t rank_ = 0;
  int prefered = 1;
  oyCMMapi3_s_ * api3 = 0;

  if(api->type_ == oyOBJECT_CMM_API3_S)
    api3 = (oyCMMapi3_s_*) api;

  if(memcmp( queries->prefered_cmm, cmm_info->cmm, 4 ) == 0)
    prefered = 10;

  rank_ = oyCMMCanHandle_( (oyCMMapi3_s*)api3, queries );

  if(rank)
    *rank = rank_ * prefered;

  if(rank_)
    return api->type_;
  else
    return oyOBJECT_NONE;
}

/* } Include "CMMapi3.private_methods_definitions.c" */

