#ifndef _TOKE_DICTIONARY_H
#define _TOKE_DICTIONARY_H

/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  dictionary.h - tokens for control commands.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 Stefan Reinauer <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

/* **************************************************************************
 *         Modifications made in 2005 by IBM Corporation
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Modifications Author:  David L. Paktor    dlpaktor@us.ibm.com
 **************************************************************************** */

/* **************************************************************************
 *
 *      Numeric values for FWord-type vocabulary entries.  Used by the
 *          big "switch" statement in handle_internal(); a subset are 
 *          also used as "definer-type" values associated with various
 *          types of definitions.
 *
 **************************************************************************** */

typedef enum fword_token {
      UNSPECIFIED  = 0xBAADD00D ,  /*  Default (absence-of) "definer"        */
      COMMON_FWORD = 0xC0EDC0DE ,  /*  Definer indicating a "shared" FWord   */
      BI_FWRD_DEFN = 0xB1F4409D ,  /*  Definer indicating a "built-in FWord" */
      COLON = 1 ,
      SEMICOLON ,
      TICK ,
      AGAIN ,
      ALIAS ,
      BRACK_TICK ,
      F_BRACK_TICK ,
      ASCII ,
      BEGIN ,
      BUFFER ,
      CASE ,
      CONST ,
      CONTROL ,
      CREATE ,
      DECIMAL ,
      DEFER ,
      DEFINED ,
      CDO ,
      DO ,
      ELSE ,
      ENDCASE ,
      ENDOF ,
      EXTERNAL ,
      INSTANCE ,
      FIELD ,
      NEW_DEVICE ,
      FINISH_DEVICE ,
      FLITERAL ,
      HEADERLESS ,
      HEADERS ,
      HEX ,
      IF ,
      UNLOOP ,
      LEAVE ,
      LOOP_I ,
      LOOP_J ,
      LOOP ,
      PLUS_LOOP ,
      OCTAL ,
      OF ,
      REPEAT ,
      THEN ,
      TO ,
      IS ,
      UNTIL ,
      VALUE ,
      VARIABLE ,
      WHILE ,
      OFFSET16 ,
      ESCAPETOK ,
      EMITBYTE ,
      FLOAD ,
      STRING ,
      PSTRING ,
      PBSTRING ,
      SSTRING ,
      RECURSIVE ,
      RECURSE ,
      RET_STK_FETCH ,
      RET_STK_FROM ,
      RET_STK_TO ,
      HEXVAL ,
      DECVAL ,
      OCTVAL ,

       ret_stk_from ,
     ASC_NUM ,          /*  Convert char seq to number  */
      ASC_LEFT_NUM ,     /*  same, only left-justified.  */

      CONDL_ENDER ,      /*  Conditional "[THEN] / [ENDIF]" variants  */
      CONDL_ELSE ,       /*  Conditional "[ELSE]" directive variants  */

      PUSH_FCODE ,	/*  Save the FCode Assignment number  */
      POP_FCODE ,	/*  Retrieve the FCode Assignment number  */
      RESET_FCODE ,	/*  Reset FCode Ass't nr and overlap checking  */

      CURLY_BRACE ,      /*  Support for IBM-style Locals  */
      DASH_ARROW ,
      LOCAL_VAL ,
      EXIT ,

      FUNC_NAME ,
      IFILE_NAME ,
      ILINE_NUM ,

      CL_FLAG ,
      SHOW_CL_FLAGS ,

      OVERLOAD ,
      ALLOW_MULTI_LINE ,
      MACRO_DEF ,
      GLOB_SCOPE ,
      DEV_SCOPE ,

      /*  This value has to be adjusted
       *      so that  FCODE_END  comes
       *      out to be  0xff
       */
      END0 = 0xd7 ,      /*   0xd7   */
      END1 ,             /*   0xd8   */
      CHAR ,             /*   0xd9   */
      CCHAR ,            /*   0xda   */
      ABORTTXT ,         /*   0xdb   */

      NEXTFCODE ,        /*   0xdc   */

      ENCODEFILE ,       /*   0xdd   */

      FCODE_V1 ,         /*   0xde   */
      FCODE_V3 ,         /*   0xdf   */
      NOTLAST ,          /*   0xef   */
      ISLAST ,           /*   0xf0   */
      SETLAST ,          /*   0xf1   */
      PCIREV ,           /*   0xf2   */
      PCIHDR ,           /*   0xf3   */
      PCIEND ,           /*   0xf4   */
      RESETSYMBS ,       /*   0xf5   */
      SAVEIMG ,          /*   0xf6   */
      START0 ,           /*   0xf7   */
      START1 ,           /*   0xf8   */
      START2 ,           /*   0xf9   */
      START4 ,           /*   0xfa   */
      VERSION1 ,         /*   0xfb   */
      FCODE_TIME ,       /*   0xfc   */
      FCODE_DATE ,       /*   0xfd   */
      FCODE_V2 ,         /*   0xfe   */
      FCODE_END = 0xff   /*   0xff   */
}  fwtoken ;

#endif   /*  _TOKE_DICTIONARY_H    */
