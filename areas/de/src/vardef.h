/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _VARDEF_H_

#define VAR_TEXTEDIT_NAME   "TextEdit"  // name of external desc editor
#ifdef __UNIX__
#  define DEF_TEXTEDIT_VAL    "vi"
#else
#  define DEF_TEXTEDIT_VAL    "notepad"  // windows gets notepad
#endif

#define VAR_SCREENWIDTH_NAME "ScreenWidth"
#define DEF_SCREENWIDTH_VAL  DEFAULT_SCREEN_WIDTH

#define VAR_SCREENHEIGHT_NAME "ScreenHeight"
#define DEF_SCREENHEIGHT_VAL  DEFAULT_SCREEN_HEIGHT

#define VAR_ISMAPZONE_NAME "IsMapZone"
#define DEF_ISMAPZONE_VAL  false

#define VAR_VNUMCHECK_NAME  "VnumCheck"  // check all vnums for validity?
#define DEF_VNUMCHECK_VAL   false

#define VAR_OBJAFFECT_NAME  "ObjAffect"  // if TRUE, user can edit obj affects
#define DEF_OBJAFFECT_VAL   false

#define VAR_CHECKLIMITS_NAME  "CheckLimits"  // check zone limits when loading?
#define DEF_CHECKLIMITS_VAL   true

#define VAR_CHECKZONEFLAGS_NAME  "CheckZoneFlags"  // check zone flags of rooms
#define DEF_CHECKZONEFLAGS_VAL  true               // against zone's?

#define VAR_SHOWCONTENTS_NAME "ShowObjContents"  // show objects inside containers
#define DEF_SHOWCONTENTS_VAL  true               // in inventory, mob eq, etc?

#define VAR_SHOWOBJFLAGS_NAME "ShowObjFlags"  // show obj flags next to name?
#define DEF_SHOWOBJFLAGS_VAL  true

#define VAR_SHOWMOBFLAGS_NAME "ShowMobFlags"  // show mob flags next to name?
#define DEF_SHOWMOBFLAGS_VAL  true

#define VAR_SHOWMOBPOS_NAME "ShowMobPos"  // show mob pos after name?
#define DEF_SHOWMOBPOS_VAL  true

#define VAR_SHOWOBJVNUM_NAME "ShowObjVnum"  // show obj vnum after name?
#define DEF_SHOWOBJVNUM_VAL  true

#define VAR_SHOWMOBVNUM_NAME "ShowMobVnum"  // show mob vnum after name?
#define DEF_SHOWMOBVNUM_VAL  true

#define VAR_SHOWMOBRIDEFOLLOW_NAME "ShowMobRideFollow"  // show ride/follow
#define DEF_SHOWMOBRIDEFOLLOW_VAL  true

#define VAR_SHOWROOMEXTRA_NAME "ShowRoomExtra"  // show sector/flag info?
#define DEF_SHOWROOMEXTRA_VAL  true

#define VAR_SHOWROOMVNUM_NAME "ShowRoomVnum"  // show room vnum?
#define DEF_SHOWROOMVNUM_VAL  true

#define VAR_SHOWPRICESADJUSTED_NAME "ShowPricesAdjusted"
#define DEF_SHOWPRICESADJUSTED_VAL  true

#define VAR_INTERPCOLOR_NAME "InterpColor"   // interpret Duris color codes?
#define DEF_INTERPCOLOR_VAL  true

#define VAR_SHOWCOLOR_NAME "ShowColor"       // show Duris color codes?
#define DEF_SHOWCOLOR_VAL  false

#define VAR_MENUPROMPT_NAME "MenuPrompt"   // stores menu prompt
#define DEF_MENUPROMPT_VAL  "&+CYour choice, m'lady? &n"

#define VAR_MAINPROMPT_NAME "MainPrompt"   // if == default, normal prompt is
#define DEF_MAINPROMPT_VAL  "default"      // shown

#define VAR_WALKCREATE_NAME "WalkCreate"       // create rooms while walking
#define DEF_WALKCREATE_VAL  false              // around?

#define VAR_SHOWEXITDEST_NAME "ShowExitDest"  // show vnum of dest room next to
#define DEF_SHOWEXITDEST_VAL  false           // exit name?

#define VAR_SHOWEXITFLAGS_NAME "ShowExitFlags"  // show exit flags next to dir?
#define DEF_SHOWEXITFLAGS_VAL  true

#define VAR_STARTROOM_NAME "StartRoom"  // room user starts in
#define DEF_STARTROOM_VAL  0

#define VAR_SROOMACTIVE_NAME "SaveStartRoom"  // save and use start room?
#define DEF_SROOMACTIVE_VAL  true

#define VAR_SHOWMENUINFO_NAME "ShowMenuInfo"  // show numb extra descs etc
#define DEF_SHOWMENUINFO_VAL  true

#define VAR_MAINZONENAME_NAME "MainZoneName"  // main name of zone
#define DEF_MAINZONENAME_VAL  ""

#define VAR_LASTCOMMAND_NAME "LastCommand"  // last command typed
#define DEF_LASTCOMMAND_VAL  ""

#define VAR_SAVEEVERYXCOMMANDS_NAME "SaveEveryXCommands"
#define DEF_SAVEEVERYXCOMMANDS_VAL  false

#define VAR_SAVEHOWOFTEN_NAME "SaveHowOften"
#define DEF_SAVEHOWOFTEN_VAL  10

#define VAR_EDITUNEDITABLEFLAGS_NAME "EditUneditableFlags"  // allow user to
#define DEF_EDITUNEDITABLEFLAGS_VAL  false                  // edit any flag?

#define VAR_GETALLMOBS_NAME "GetAllMobs"  // when typing "get/drop all",
#define DEF_GETALLMOBS_VAL  false         // include mobs?  (not imped)

#define VAR_IGNOREZONES_NAME "IgnoreZoneS"  // ignore the "S" in a zone
#define DEF_IGNOREZONES_VAL  false          // file?  (stop on EOF instead)

#define VAR_NEGDESTOUTOFZONE_NAME "NegDestOutOfZone"  // is a negative dest.
#define DEF_NEGDESTOUTOFZONE_VAL  true                // considered out of zone?

// check-related variables

#define VAR_CHECKMISSINGKEYS_NAME "CheckMissingKeys"  // check to see if keys
#define DEF_CHECKMISSINGKEYS_VAL  true                // for doors/objs are
                                                      // loaded?

#define VAR_CHECKLOADED_NAME "CheckLoaded"  // check all obj/mob types to
#define DEF_CHECKLOADED_VAL  false          // make sure at least one is loaded?

#define VAR_CHECKSAVE_NAME "CheckSave"  // run "check" with each save?
#define DEF_CHECKSAVE_VAL  false

#define VAR_NOSAVEONCHECKERR_NAME "NoSaveOnCheckErr"  // if TRUE, abort
#define DEF_NOSAVEONCHECKERR_VAL  true                // saving if err found

#define VAR_SAVECHECKLOG_NAME "SaveCheckLog"  // write errors to CHECK.LOG?
#define DEF_SAVECHECKLOG_VAL  true

#define VAR_PAUSECHECKSCREENFUL_NAME "PauseCheckScreenful" // if TRUE, pause
#define DEF_PAUSECHECKSCREENFUL_VAL  true

#define VAR_CHECKLONEROOM_NAME "CheckLoneRoom"  // check for rooms with no
#define DEF_CHECKLONEROOM_VAL true              // exits to/from them?

#define VAR_CHECKROOM_NAME "CheckRoom"  // check various room thingies?
#define DEF_CHECKROOM_VAL  true

#define VAR_CHECKEXIT_NAME "CheckExit"  // check various exit thingies?
#define DEF_CHECKEXIT_VAL  true

#define VAR_CHECKEXITDESC_NAME "CheckExitDesc"  // check for exit descs?
#define DEF_CHECKEXITDESC_VAL  false

#define VAR_CHECKOBJ_NAME "CheckObj"  // check various obj thingies?
#define DEF_CHECKOBJ_VAL  true

#define VAR_CHECKOBJVAL_NAME "CheckObjVal"  // check obj values based on obj
#define DEF_CHECKOBJVAL_VAL  true           // type?

#define VAR_CHECKOBJDESC_NAME "CheckObjDesc"  // check for obj edescs?
#define DEF_CHECKOBJDESC_VAL  true

#define VAR_CHECKMOBDESC_NAME "CheckMobDesc"  // check for mob descs?
#define DEF_CHECKMOBDESC_VAL  true

#define VAR_CHECKMOB_NAME "CheckMob"  // check various mob thingies?
#define DEF_CHECKMOB_VAL  true

#define VAR_CHECKEDESC_NAME "CheckEDesc"  // check for edescs with no
#define DEF_CHECKEDESC_VAL  true          // keywords or actual descs?

#define VAR_CHECKFLAGS_NAME "CheckFlags"  // check for uneditable flags
#define DEF_CHECKFLAGS_VAL  true          // set to non-defaults?

#define VAR_CHECKMOBCLASSEQ_NAME "CheckMobClassEq"  // check mob class vs.
#define DEF_CHECKMOBCLASSEQ_VAL  false              // eq flags?

#define VAR_CHECKMOBRACEEQ_NAME "CheckMobRaceEq"  // check mob race vs.
#define DEF_CHECKMOBRACEEQ_VAL  false             // eq flags?

#define VAR_CHECKMOBSEXEQ_NAME "CheckMobSexEq"  // check mob sex vs.
#define DEF_CHECKMOBSEXEQ_VAL  false            // eq flags?

#define VAR_CHECKOBJMATERIAL_NAME "CheckObjMaterial"  // check for out-of-
#define DEF_CHECKOBJMATERIAL_VAL  true                // range obj materials?

#define VAR_CHECKGUILDSTUFF_NAME "CheckGuildStuff"  // check "guild stuff"?
#define DEF_CHECKGUILDSTUFF_VAL  false

#define VAR_CHECKZONE_NAME "CheckZone"  // check zone name, difficulty, reset mode
#define DEF_CHECKZONE_VAL  true

#define _VARDEF_H_
#endif
