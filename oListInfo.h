#pragma once
/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2015 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

#include <set>
#include <vector>
#include "oBase.h"
#include "gdifonts.h"

typedef oEvent *pEvent;

enum EPostType
{
  lAlignNext,
  lNone,
  lString,
  lResultDescription,
  lTimingFromName,
  lTimingToName,
  lCmpName,
  lCmpDate,
  lCurrentTime,
  lClubName,
  lClassName,
  lClassStartName,
  lClassStartTime,
  lClassLength,
  lClassResultFraction,
  lCourseLength,
  lCourseName,
  lCourseClimb,
  lRunnerName,
  lRunnerGivenName,
  lRunnerFamilyName,
  lRunnerCompleteName,
  lPatrolNameNames, // Single runner's name or both names in a patrol
  lPatrolClubNameNames, // Single runner's club or combination of patrol clubs
  lRunnerFinish,
  lRunnerTime,
  lRunnerTimeStatus,
  lRunnerTotalTime,
  lRunnerTimePerKM,
  lRunnerTotalTimeStatus,
  lRunnerTotalPlace,
  lRunnerPlaceDiff,
  lRunnerClassCoursePlace,
  lRunnerTotalTimeAfter,
  lRunnerClassCourseTimeAfter,
  lRunnerTimeAfterDiff,
  lRunnerTempTimeStatus,
  lRunnerTempTimeAfter,
  lRunnerTimeAfter,
  lRunnerMissedTime,
  lRunnerPlace,
  lRunnerStart,
  lRunnerClub,
  lRunnerCard,
  lRunnerBib,
  lRunnerStartNo,
  lRunnerRank,
  lRunnerCourse,
  lRunnerRogainingPoint,
  lRunnerRogainingPointTotal,
  lRunnerRogainingPointReduction,
  lRunnerRogainingPointOvertime,
  lRunnerTimeAdjustment,
  lRunnerPointAdjustment,

  lRunnerUMMasterPoint,
  lRunnerTimePlaceFixed,
  lRunnerLegNumberAlpha,

  lRunnerBirthYear,
  lRunnerAge,
  lRunnerSex,
  lRunnerNationality,
  lRunnerPhone,
  lRunnerFee,

  lTeamName,
  lTeamStart,
  lTeamTimeStatus,
  lTeamTimeAfter,
  lTeamPlace,
  lTeamLegTimeStatus,
  lTeamLegTimeAfter,
  lTeamRogainingPoint,
  lTeamRogainingPointTotal,
  lTeamRogainingPointReduction,
  lTeamRogainingPointOvertime,
  lTeamTimeAdjustment,
  lTeamPointAdjustment,

  lTeamTime,
  lTeamStatus,
  lTeamClub,
  lTeamRunner,
  lTeamRunnerCard,
  lTeamBib,
  lTeamStartNo,
  lTeamFee,

  lTeamTotalTime,
  lTeamTotalTimeStatus,
  lTeamTotalPlace,
  lTeamTotalTimeAfter,
  lTeamTotalTimeDiff,
  lTeamPlaceDiff,

  lPunchNamedTime,
  lPunchTime,
  lPunchControlNumber,
  lPunchControlCode,
  lPunchLostTime,
  lPunchControlPlace,
  lPunchControlPlaceAcc,

  lResultModuleTime,
  lResultModuleNumber,
  lResultModuleTimeTeam,
  lResultModuleNumberTeam,
  
  lRogainingPunch,
  lTotalCounter,
  lSubCounter,
  lSubSubCounter,
  lLastItem
};

enum EStdListType
{
  EStdNone=-1,
  EStdStartList=1,
  EStdResultList,
  EGeneralResultList,
  ERogainingInd,
  EStdTeamResultListAll,
  EStdTeamResultListLeg,
  EStdTeamResultList,
  EStdTeamStartList,
  EStdTeamStartListLeg,
  EStdIndMultiStartListLeg,
  EStdIndMultiResultListLeg,
  EStdIndMultiResultListAll,
  EStdPatrolStartList,
  EStdPatrolResultList,
  EStdRentedCard,
  EStdResultListLARGE,
  EStdTeamResultListLegLARGE,
  EStdPatrolResultListLARGE,
  EStdIndMultiResultListLegLARGE,
  EStdRaidResultListLARGE, //Obsolete
  ETeamCourseList,
  EIndCourseList,
  EStdClubStartList,
  EStdClubResultList,

  EIndPriceList,
  EStdUM_Master,

  EFixedPreReport,
  EFixedReport,
  EFixedInForest,
  EFixedInvoices,
  EFixedEconomy,
  EFixedResultFinishPerClass,
  EFixedResultFinish,
  EFixedMinuteStartlist,
  EFixedTimeLine,
  EFirstLoadedList = 1000
};

enum EFilterList
{
  EFilterHasResult,
  EFilterHasPrelResult,
  EFilterRentCard,
  EFilterHasCard,
  EFilterHasNoCard,
  EFilterExcludeDNS,
  EFilterVacant,
  EFilterOnlyVacant,
  _EFilterMax
};

enum ESubFilterList
{
  ESubFilterHasResult,
  ESubFilterHasPrelResult,
  ESubFilterExcludeDNS,
  ESubFilterVacant,
  _ESubFilterMax
};

enum gdiFonts;

struct oPrintPost {
  oPrintPost();
  oPrintPost(EPostType type_, const string &format_,
                       int style_, int dx_, int dy_, int index_=0);

  static string encodeFont(const string &face, int factor);

  EPostType type;
  string text;
  string fontFace;
  int resultModuleIndex;
  int format;
  GDICOLOR color;
  int dx;
  int dy;
  int index;
  gdiFonts getFont() const {return gdiFonts(format & 0xFF);}
  oPrintPost &setFontFace(const string &font, int factor) {
    fontFace = encodeFont(font, factor);
    return *this;
  }
  int fixedWidth;
  bool doMergeNext;
  mutable const oPrintPost *mergeWithTmp; // Merge text with this output
};

class gdioutput;
enum gdiFonts;
typedef int (*GUICALLBACK)(gdioutput *gdi, int type, void *data);
class xmlparser;
class xmlobject;
class MetaListContainer;

struct oListParam {
  oListParam();
  EStdListType listCode;
  GUICALLBACK cb;
  set<int> selection;
  int legNumber;
  int useControlIdResultTo;
  int useControlIdResultFrom;
  int filterMaxPer;
  bool pageBreak;
  bool showInterTimes;
  bool showSplitTimes;
  bool splitAnalysis;
  string title;
  string name;
  int inputNumber;

  mutable string defaultName; // Initialized when generating list
  // Generate a large-size list (supported as input when supportLarge is true)
  bool useLargeSize;
  bool saved;

  void updateDefaultName(const string &name) const {defaultName = name;}
  void setCustomTitle(const string &t) {title = t;}
  void getCustomTitle(char *t) const; // 256 size buffer required. Get title if set
  const string &getCustomTitle(const string &t) const;
  const string &getDefaultName() const {return defaultName;}
  void setName(const string &n) {name = n;}
  const string &getName() const {return name;}

  int getInputNumber() const {return inputNumber;}
  void setInputNumber(int n) {inputNumber = n;}

  void serialize(xmlparser &xml, const MetaListContainer &container) const;
  void deserialize(const xmlobject &xml, const MetaListContainer &container);
};

class oListInfo {
public:
  enum EBaseType {EBaseTypeRunner,
                  EBaseTypeTeam,
                  EBaseTypeClub,
                  EBaseTypePunches,
                  EBaseTypeNone,
                  EBaseTypeRunnerGlobal,  // Used only in metalist (meaning global, not classwise)
                  EBaseTypeTeamGlobal, // Used only in metalist (meaning global, not classwise)
                  EBasedTypeLast_};

  
  enum ResultType {
    Global,
    Classwise,
  };

  static bool addRunners(EBaseType t) {return t == EBaseTypeRunner || t == EBaseTypeClub;}
  static bool addTeams(EBaseType t) {return t == EBaseTypeTeam || t == EBaseTypeClub;}
  static bool addPatrols(EBaseType t) {return t == EBaseTypeTeam || t == EBaseTypeClub;}

  const string &getName() const {return Name;}
protected:
  string Name;
  EBaseType listType;
  EBaseType listSubType;
  SortOrder sortOrder;
     
  bool calcResults;
  bool calcCourseClassResults;
  bool calcTotalResults;
  bool rogainingResults;

  oListParam lp;

  list<oPrintPost> Head;
  list<oPrintPost> subHead;
  list<oPrintPost> listPost;
  vector<char> listPostFilter;
  vector<char> listPostSubFilter;
  list<oPrintPost> subListPost;
  bool fixedType;
  bool needPunches;
  string resultModule;
  set<string> additionalModules;

  void setupLinks(const list<oPrintPost> &lst) const;
  void setupLinks() const;

  oListInfo *next;
public:
  ResultType getResultType() const;

  void addList(const oListInfo &lst);

  bool supportClasses;
  bool supportLegs;
  bool supportParameter;
  // True if large (and non-large) is supported
  bool supportLarge;
  // True if a large-size list only
  bool largeSize;

  // True if supports timing from control
  bool supportTo;
  // True if supports timing to control
  bool supportFrom;
  // Result type 
  ResultType resType;


  bool needPunchCheck() const {return needPunches;}
  void setCallback(GUICALLBACK cb) {lp.cb=cb;}
  int getLegNumber() const {return lp.legNumber;}
  EStdListType getListCode() const {return lp.listCode;}
  oPrintPost &addHead(const oPrintPost &pp) {
    Head.push_back(pp);
    return Head.back();
  }
  oPrintPost &addSubHead(const oPrintPost &pp) {
    subHead.push_back(pp);
    return subHead.back();
  }
  oPrintPost &addListPost(const oPrintPost &pp) {
    listPost.push_back(pp);
    return listPost.back();
  }
  oPrintPost &addSubListPost(const oPrintPost &pp) {
    subListPost.push_back(pp);
    return subListPost.back();
  }
  inline bool filter(EFilterList i) const {return listPostFilter[i]!=0;}
  inline bool subFilter(ESubFilterList i) const {return listPostSubFilter[i]!=0;}

  void setFilter(EFilterList i) {listPostFilter[i]=1;}
  void setSubFilter(ESubFilterList i) {listPostSubFilter[i]=1;}

  void setResultModule(const string &rm) {resultModule = rm;}
  void additionalResultModule(const string &rm) {additionalModules.insert(rm);}
  const string &getResultModule() const {return resultModule;}
  oListInfo(void);
  ~oListInfo(void);

  friend class oEvent;
  friend class MetaList;
  friend class MetaListContainer;

  int getMaxCharWidth(const oEvent *oe, EPostType type,
                      const string &format, gdiFonts font,
                      const char *fontFace = 0,
                      bool large = false, int minSize = 0);


  const oListParam &getParam() const {return lp;}
  oListParam &getParam() {return lp;}

  // Returns true if the list needs to be regenerated due to competition changes
  bool needRegenerate(const oEvent &oe) const;

};
