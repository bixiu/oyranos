/** @file oyranos_forms_fltk.cxx
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    forms handling for the FLTK toolkit
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2009/10/04
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "oyranos_widgets_fltk.h"
#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
//#include "../fl_i18n/fl_i18n.H"
#include <FL/Flmm_Tabs.H>

#include "config.h"
#include "oyranos.h"
#include "oyranos_debug.h"
#include "oyranos_elektra.h"
#include "oyranos_helper.h"
#include "oyranos_internal.h"
#include "oyranos_forms.h"


using namespace oyranos;
using namespace oyranos::forms;

typedef struct {
  char * label;
  char * value;
  char * key;
  oyOptions_s ** callback_data;
} fltk_cb_data;

void fltkCallback                    ( Fl_Widget         * widget,
                                       void              * user_data )
{
  fltk_cb_data * cd = (fltk_cb_data*) user_data;
  Fl_Choice * ch = dynamic_cast<Fl_Choice*> (widget);
  oyOptions_s ** opts = cd->callback_data;
  char num[24];

  if(ch)
  {
    sprintf(num, "%d", ch->value() );
    oyOptions_SetFromText( opts, cd->key, cd->value, 0 );
    printf("Fl_Choice %s(%s)=%s/%s\n", cd->key, cd->label, cd->value, num );

  } else
    printf("no Fl_Choice %s(%s)=%s/%s\n", cd->key, cd->label, cd->value, num );
}

void fltkHelpViewCallback            ( Fl_Widget         * widget,
                                       void              * user_data )
{
  fltk_cb_data * cd = (fltk_cb_data*) user_data;
  Fl_Choice * ch = dynamic_cast<Fl_Choice*> (widget);
  oyOptions_s ** opts = cd->callback_data;
  char num[24];

  if(ch)
  {
    sprintf(num, "%d", ch->value() );
    oyOptions_SetFromText( opts, cd->key, cd->value, 0 );
    printf("Fl_Choice %s(%s)=%s/%s\n", cd->key, cd->label, cd->value, num );

  } else
    printf("no Fl_Choice %s(%s)=%s/%s\n", cd->key, cd->label, cd->value, num );
}

class formsFltkChoice : public Fl_Choice
{
  public:
  formsFltkChoice(int X,int Y,int W,int H) : Fl_Choice(X,Y,W,H) 
  { hint_callback= 0; };
  oyFormsCallback_s * hint_callback;
  int handle(int event)
  {
    int result = Fl_Choice::handle(event);
    switch (event)
    {
      case FL_ENTER:
           printf("Enter\n");
           if(hint_callback)
           {
             if(hint_callback)
             {
               oyFormsFltkHelpViewCallback_f userCallback = 0;
               userCallback =(oyFormsFltkHelpViewCallback_f)
                                                        hint_callback->callback;
               userCallback( (oyFormsArgs_s*)hint_callback->data,
                             (const char*)user_data() );
             }
           }
           redraw();
           break;

      case FL_LEAVE:
           printf("Leave\n");
           redraw();
           break;
    }
    return result;
  }
};

/** @internal
 *  Function oyXML2XFORMsFLTKSelect1Handler
 *  @brief   build a UI for a xf:select1 XFORMS sequence
 *
 *  This function is a simple demonstration.
 *
 *  @param[in]     cur                 libxml2 node
 *  @param[in]     collected_elements  parsed and requested elements
 *  @param[in]     user_data           toolkit context
 *  @return                            error
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/08/29 (Oyranos: 0.1.10)
 *  @date    2009/10/11
 */
int        oyXML2XFORMsFLTKSelect1Handler (
                                       xmlNodePtr          cur,
                                       oyOptions_s       * collected_elements,
                                       oyPointer           user_data )
{
  oyOption_s * o  = 0, * o2, *o3;
  int n = oyOptions_Count( collected_elements ),
      i,j,j_n,k,k_n,
      is_default, default_pos = -1,
      choices_n = 0;
  oyOptions_s * opts = 0, * opts2;
  const char * default_value = 0,
             * tmp,
             * label,
             * value,
             * search;
  char * default_key = 0, * key = 0, * t = 0;
  char * choices = 0;
  oyFormsArgs_s * forms_args = (oyFormsArgs_s *)user_data;
  int print = forms_args ? !forms_args->silent : 1;
  int error = 0;

  default_value = oyOptions_FindString( collected_elements, "xf:select1", 0 );
  o = oyOptions_Find( collected_elements, "xf:select1" );
  key = oyStringCopy_( o->registration, oyAllocateFunc_ );
  t = oyStrrchr_( key, '/' );
  t = oyStrchr_( t, '.' );
  t[0] = 0;

  if(oy_debug && default_value && print)
    printf( "found default: \"%s\"\n", default_value );

  Fl_Group *parent = Fl_Group::current();

  if(!parent)
    return 1;

  int x = parent->x(),
            y = parent->y(),
            w = parent->w()/*,
            h = parent->h()*/;
  Fl_Pack * pack = new Fl_Pack(x,y,w,BUTTON_HEIGHT);
  pack->type( FL_HORIZONTAL );
  pack->spacing(H_SPACING);

    new Fl_Box( 0,0,H_SPACING,BUTTON_HEIGHT);
    OyFl_Box_c * box = new OyFl_Box_c( 2*H_SPACING,0,w-BOX_WIDTH-4*H_SPACING,BUTTON_HEIGHT);
    box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );

    formsFltkChoice * c = new formsFltkChoice( w-BOX_WIDTH-H_SPACING,0,BOX_WIDTH,BUTTON_HEIGHT );

  search = oyOptions_FindString( collected_elements, "search", 0 );

  for(i = 0; i < n; ++i)
  {
    o = oyOptions_Get( collected_elements, i );
    opts = (oyOptions_s*) oyOption_StructGet( o, oyOBJECT_OPTIONS_S );

    if(!opts && oyFilterRegistrationMatch( o->registration,"xf:label",
                                           oyOBJECT_NONE ) &&
       print)
      box->copy_label( o->value->string );

    if(!opts && oyFilterRegistrationMatch( o->registration,"xf:help",
                                           oyOBJECT_NONE ) &&
       print)
    {
      oyFormsCallback_s * cb = 0;
      error = oyOptions_FindData( (oyOptions_s*)forms_args->data_,
                                  OYFORMS_FLTK_HELP_VIEW_REG,
                                  (oyPointer*)&cb, 0, 0);
      if(cb)
        c->hint_callback = cb;
      c->user_data( o->value->string );
    }

    if(opts && oyFilterRegistrationMatch( o->registration,"xf:choices",
                                          oyOBJECT_NONE ))
    {
      j_n = oyOptions_Count( opts);
      for(j = 0; j < j_n; ++j)
      {
        o2 = oyOptions_Get( opts, j );
        opts2 = (oyOptions_s*) oyOption_StructGet( o2, oyOBJECT_OPTIONS_S );

        if(!opts2 && oyFilterRegistrationMatch(o2->registration,"xf:label",
                                               oyOBJECT_NONE )
           && print)
          box->copy_label( o2->value->string );

        if(opts2 && oyFilterRegistrationMatch( o2->registration,"xf:item",
                                               oyOBJECT_NONE ))
        {
          label = tmp = value = 0;
          is_default = 0;

          if(oy_debug && print)
          {
            k_n = oyOptions_Count( opts2);
            for(k = 0; k < k_n; ++k)
            {
              o3 = oyOptions_Get( opts2, k );
              if(oy_debug && print)
                printf( "    found option: 0x%x  \"%s\" %s\n",
                  (int)o3, oyOption_GetText(o3, oyNAME_NICK),
                  oyStruct_TypeToText((oyStruct_s*)o3) );

              oyOption_Release( &o3 );
            }
          }

          /* collect the understood elements */
          tmp = oyOptions_FindString( opts2, "xf:label", 0 );
          if(tmp)
            label = tmp;
          tmp = oyOptions_FindString( opts2, "xf:value", 0 );
          if(tmp)
            value = tmp;

          if(!value && !label)
            continue;

          /* detect default */
          if(value && default_value &&
             oyStrcmp_(default_value,value) == 0)
          {
            is_default = 1;
            default_pos = choices_n;
          }

          if(!value) value = label;
          if(!label) label = value;

          /* append the choice
           * store the label and value in user_data() for evaluating results */
          if(print)
          {
            fltk_cb_data *cb_data = (fltk_cb_data*)malloc(sizeof(fltk_cb_data));
            int len = strlen(label), pos = 0;
            memset(cb_data, 0, sizeof(fltk_cb_data) );
            cb_data->label = (char*) malloc(strlen(label)*2);
            cb_data->value = strdup(value);
            cb_data->key = strdup(key);
            cb_data->callback_data = (oyOptions_s**)
                                                &forms_args->xforms_data_model_;
            for(k = 0; k <= len; ++k)
            {
              if(label[k] == '/')
                cb_data->label[pos++] = '\\';
              cb_data->label[pos++] = label[k];
            }
            c->add( (const char *) cb_data->label, 0,
                    fltkCallback,
                    (void*)cb_data, 0 );
          }

          ++choices_n;
        }
        else if(oy_debug && print)
          printf( "  found option: 0x%x  \"%s\" %s\n",
                (int)o2, oyOption_GetText(o2, oyNAME_NICK),
                oyStruct_TypeToText((oyStruct_s*)o2) );

        oyOptions_Release( &opts2 );
        oyOption_Release( &o2 );
      }
    }
    else if(oy_debug && print)
      printf( "found option: 0x%x  \"%s\" %s\n",
              (int)o, oyOption_GetText(o, oyNAME_NICK),
              oyStruct_TypeToText((oyStruct_s*)o) );

    oyOptions_Release( &opts );
    oyOption_Release( &o );
  }

  pack->end();
  pack->resizable( box );
  if(default_pos >= 0)
    c->value( default_pos );

  /* collect results */
  o = oyOptions_Find( collected_elements, "xf:select1" );
  if(o)
  {
    STRING_ADD( default_key, o->registration );
    t = oyStrstr_( default_key, ".xf:select1" );
    t[0] = 0;

    if(forms_args)
      oyOptions_SetFromText( (oyOptions_s**)&forms_args->xforms_data_model_,
                             key, default_value, OY_CREATE_NEW );

    oyOption_Release( &o );
  }

  if(choices)
    oyFree_m_( choices );
  oyFree_m_( default_key );
  if(key)
    oyFree_m_( key );

  /*printf("collected:\n%s", oyOptions_GetText( collected_elements, oyNAME_NICK));*/
  return 0;
}

const char * oy_ui_fltk_handler_xf_select1_element_searches_[] = {
 "xf:choices/xf:item/xf:label.xf:value",
 "xf:help",
 0
};

oyUiHandler_s oy_ui_fltk_handler_xf_select1_ =
  {oyOBJECT_UI_HANDLER_S,0,0,0,        /**< oyStruct_s members */
   (char*)"oyFORMS",                   /**< dialect */
   (char*)"libxml2",                   /**< parser_type */
   (char*)"xf:select1",                /**< element_type; Wanted XML element. */
   (oyUiHandler_f)oyXML2XFORMsFLTKSelect1Handler, /**<oyUiHandler_f handler*/
   (char*)"dummy",                     /**< handler_type */
   (char**)oy_ui_fltk_handler_xf_select1_element_searches_ /**< element_searches */
  };

/** @internal
 *  Function oyXML2XFORMsFLTKHtmlHeadlineHandler
 *  @brief   build a UI for a html:h3 element
 *
 *  This function is a simple demonstration.
 *
 *  @param[in]     cur                 libxml2 node
 *  @param[in]     collected_elements  parsed and requested elements
 *  @param[in]     user_data           toolkit context
 *  @return                            error
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/08/29 (Oyranos: 0.1.10)
 *  @date    2009/08/31
 */
int        oyXML2XFORMsFLTKHtmlHeadlineHandler (
                                       xmlNodePtr          cur,
                                       oyOptions_s       * collected_elements,
                                       oyPointer           user_data )
{
  const char * tmp = 0;
  int size = 0;
  oyFormsArgs_s * forms_args = (oyFormsArgs_s *)user_data;
  int print = forms_args ? !forms_args->silent : 1;
  OyFl_Box_c * box = 0;

  if(!tmp)
  {
    tmp = oyOptions_FindString( collected_elements, "h3", 0 );
    if(tmp)
      size = 3;
  }

  if(tmp && tmp[0] && print)
  {
    Fl_Group *parent = Fl_Group::current(); // parent tab

    if( !parent )
    {
      WARNc_S( "wrong widget" );
      return 1;
    }

    {
        Fl_Widget *wid = (Fl_Widget*)0; //parent->user_data();
        if( !wid ) wid = parent;

        int x = parent->x(),
            y = parent->y(),
            w = parent->w();
        box = new OyFl_Box_c( x,y,w,BUTTON_HEIGHT );
        box->copy_label( tmp );
        box->labelfont( FL_BOLD );
        box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
    }
  }

  return 0;
}

/** @internal
 *  Function oyXML2XFORMsFLTKHtmlHeadline4Handler
 *  @brief   build a UI for a html:h4 element
 *
 *  This function is a simple demonstration.
 *
 *  @param[in]     cur                 libxml2 node
 *  @param[in]     collected_elements  parsed and requested elements
 *  @param[in]     user_data           toolkit context
 *  @return                            error
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/10/04 (Oyranos: 0.1.10)
 *  @date    2009/10/04
 */
int        oyXML2XFORMsFLTKHtmlHeadline4Handler (
                                       xmlNodePtr          cur,
                                       oyOptions_s       * collected_elements,
                                       oyPointer           user_data )
{
  const char * tmp = 0;
  int size = 0;
  oyFormsArgs_s * forms_args = (oyFormsArgs_s *)user_data;
  int print = forms_args ? !forms_args->silent : 1;
  OyFl_Box_c * box = 0;

  if(!tmp)
  {
    tmp = oyOptions_FindString( collected_elements, "h4", 0 );
    if(tmp)
      size = 3;
  }

  if(tmp && print)
  {
    Fl_Group *parent = Fl_Group::current(); // parent tab

    if( !parent )
    {
      WARNc_S( "wrong widget" );
      return 1;
    }

    {
        int x = parent->x(),
            y = parent->y(),
            w = parent->w();
        box = new OyFl_Box_c( x,y,w,BUTTON_HEIGHT );
        box->copy_label( tmp );
        box->labelfont( FL_ITALIC );
        box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
    }
  }

  return 0;
}

const char * oy_ui_fltk_handler_html_headline4_element_searches_[] = {"h4",0};

oyUiHandler_s oy_ui_fltk_handler_html_headline4_ =
  {oyOBJECT_UI_HANDLER_S,0,0,0,        /**< oyStruct_s members */
   (char*)"oyFORMS",                   /**< dialect */
   (char*)"libxml2",                   /**< parser_type */
   (char*)"h4",                        /**< element_type; Wanted XML elements.*/
   (oyUiHandler_f)oyXML2XFORMsFLTKHtmlHeadline4Handler, /**<oyUiHandler_f handler*/
   (char*)"dummy",                     /**< handler_type */
   (char**)oy_ui_fltk_handler_html_headline4_element_searches_ /**< element_searches */
  };

const char * oy_ui_fltk_handler_html_headline_element_searches_[] = {"h3",0};
oyUiHandler_s oy_ui_fltk_handler_html_headline_ =
  {oyOBJECT_UI_HANDLER_S,0,0,0,        /**< oyStruct_s members */
   (char*)"oyFORMS",                   /**< dialect */
   (char*)"libxml2",                   /**< parser_type */
   (char*)"h3",                        /**< element_type; Wanted XML elements.*/
   (oyUiHandler_f)oyXML2XFORMsFLTKHtmlHeadlineHandler, /**<oyUiHandler_f handler*/
   (char*)"dummy",                     /**< handler_type */
   (char**)oy_ui_fltk_handler_html_headline_element_searches_ /**< element_searches */
  };

oyUiHandler_s * oy_ui_fltk_handlers[4] = {
  &oy_ui_fltk_handler_xf_select1_,
  &oy_ui_fltk_handler_html_headline_,
  &oy_ui_fltk_handler_html_headline4_,
  0
};


