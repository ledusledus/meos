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

// csvparser.cpp: implementation of the csvparser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "meos.h"
#include "csvparser.h"
#include "oEvent.h"
#include "SportIdent.h"
#include "meos_util.h"
#include "localizer.h"
#include "meosexception.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

csvparser::csvparser()
{
  LineNumber=0;
}

csvparser::~csvparser()
{

}

int csvparser::iscsv(const char *file)
{
  fin.open(file);

  if (!fin.good())
    return false;

  char bf[2048];
  fin.getline(bf, 2048);

  while(fin.good() && strlen(bf)<3)
    fin.getline(bf, 2048);

  fin.close();

  vector<char *> sp;

  split(bf, sp);

  if (sp.size()==1 && strcmp(sp[0], "RAIDDATA")==0)
    return 3;

  if (sp.size()<5)//No csv
    return 0;

  if (_stricmp(sp[1], "Descr")==0 || _stricmp(sp[1], "Namn")==0) //OS-fil (SWE/ENG)??
    return 2;
  else return 1; //OE?!
}

RunnerStatus ConvertOEStatus(int i)
{
  switch(i)
  {
      case 0:
      return StatusOK;
      case 1:  // Ej start
      return StatusDNS;
      case 2:  // Utg.
      return StatusDNF;
      case 3:  // Felst.
      return StatusMP;
      case 4: //Disk
      return StatusDQ;
      case 5: //Maxtid
      return StatusMAX;
  }
  return StatusUnknown;
}


//Stno;Descr;Block;nc;Start;Time;Classifier;Club no.;Cl.name;City;Nat;Cl. no.;Short;Long;Legs;Num1;Num2;Num3;Text1;Text2;Text3;Start fee;Paid;Surname;First name;YB;S;Start;Finish;Time;Classifier;Chip;Rented;Database Id;Surname;First name;YB;S;Start;Finish;Time;Classifier;Chip;Rented;Database Id;Surname;First name;YB;S;Start;Finish;Time;Classifier;Chip;Rented;Database Id;(may be more) ...

bool csvparser::ImportOS_CSV(oEvent &event, const char *file)
{
  enum {OSstno=0, OSdesc=1, OSstart=4, OStime=5, OSstatus=6, OSclubno=7, OSclub=9,
    OSnat=10, OSclassno=11, OSclass=12, OSlegs=14, OSfee=21, OSpaid=22};

  const int Offset=23;
  const int PostSize=11;

  enum {OSRsname=0, OSRfname=1, OSRyb=2, OSRsex=3, OSRstart=4,
    OSRfinish=5, OSRstatus=7, OSRcard=8, OSRrentcard=9};

  fin.open(file);

  if (!fin.good())
    return false;

  char bf[1024];
  fin.getline(bf, 1024);

  nimport=0;
  while(!fin.eof())
  {
    fin.getline(bf, 1024);

    vector<char *> sp;

    split(bf, sp);

    if (sp.size()>20 && strlen(sp[OSclub])>0)
    {
      nimport++;

      //Create club with this club number...
      int ClubId=atoi(sp[OSclubno]);
      pClub pclub=event.getClubCreate(ClubId, sp[OSclub]);

      if (pclub){
        pclub->getDI().setString("Nationality", sp[OSnat]);
        pclub->synchronize(true);
      }

      //Create class with this class number...
      int ClassId=atoi(sp[OSclassno]);
      event.getClassCreate(ClassId, sp[OSclass]);

      //Club is autocreated...
      pTeam team=event.addTeam(string(sp[OSclub])+" "+string(sp[OSdesc]), ClubId,  ClassId);

      team->setStartNo(atoi(sp[OSstno]), false);

      if (strlen(sp[12])>0)
        team->setStatus( ConvertOEStatus( atoi(sp[OSstatus]) ), true, false);

      team->setStartTime(event.convertAbsoluteTime(sp[OSstart]), true, false);

      if (strlen(sp[OStime])>0)
        team->setFinishTime( event.convertAbsoluteTime(sp[OSstart])+event.convertAbsoluteTime(sp[OStime])-event.getZeroTimeNum() );

      if (team->getStatus()==StatusOK && team->getFinishTime()==0)
        team->setStatus(StatusUnknown, true, false);

      unsigned rindex=Offset;

      oDataInterface teamDI=team->getDI();

      teamDI.setInt("Fee", atoi(sp[OSfee]));
      teamDI.setInt("Paid", atoi(sp[OSpaid]));
      teamDI.setString("Nationality", sp[OSnat]);

      //Import runners!
      int runner=0;
      while( (rindex+OSRrentcard)<sp.size() && strlen(sp[rindex+OSRfname])>0 ){
        int year = extendYear(atoi(sp[rindex+OSRyb]));
        int cardNo = atoi(sp[rindex+OSRcard]);
        pRunner r = event.addRunner(string(sp[rindex+OSRfname])+" "+string(sp[rindex+OSRsname]), ClubId,
                                    ClassId, cardNo, year, false);

        oDataInterface DI=r->getDI();
        //DI.setInt("BirthYear", extendYear(atoi(sp[rindex+OSRyb])));
        DI.setString("Sex", sp[rindex+OSRsex]);
        DI.setString("Nationality", sp[OSnat]);

        if (strlen(sp[rindex+OSRrentcard])>0)
          DI.setInt("CardFee", event.getDCI().getInt("CardFee"));

        //r->setCardNo(atoi(sp[rindex+OSRcard]), false);
        r->setStartTime(event.convertAbsoluteTime(sp[rindex+OSRstart]), true, false);
        r->setFinishTime( event.convertAbsoluteTime(sp[rindex+OSRfinish]) );

        if (strlen(sp[rindex+OSRstatus])>0)
          r->setStatus( ConvertOEStatus( atoi(sp[rindex+OSRstatus]) ), true, false);

        if (r->getStatus()==StatusOK && r->getRunningTime()==0)
          r->setStatus(StatusUnknown, true, false);

        r->addClassDefaultFee(false);

        team->setRunner(runner++, r, true);

        rindex+=PostSize;
      }
      //int nrunners=team->GetNumRunners();
      pClass pc=event.getClass(ClassId);

      if (pc && runner>(int)pc->getNumStages())
        pc->setNumStages(runner);

      team->apply(true, 0, false);
    }
  }
  fin.close();

  return true;
}


bool csvparser::ImportOE_CSV(oEvent &event, const char *file)
{
  enum {OEstno=0, OEcard=1, OEid=2, OEsurname=3, OEfirstname=4,
      OEbirth=5, OEsex=6, OEstart=9,  OEfinish=10, OEstatus=12,
      OEclubno=13, OEclub=15, OEnat=16, OEclassno=17, OEclass=18, OEbib=23,
      OErent=35, OEfee=36, OEpaid=37, OEcourseno=38, OEcourse=39,
      OElength=40};

  fin.open(file);

  if (!fin.good())
    return false;

  char bf[1024];
  fin.getline(bf, 1024);

  nimport=0;
  while (!fin.eof()) {
    fin.getline(bf, 1024);

    vector<char *> sp;

    split(bf, sp);

    if (sp.size()>20) {
      nimport++;

      int clubId = atoi(sp[OEclubno]);
      pClub pclub = event.getClubCreate(clubId, sp[OEclub]);

      if (pclub) {
        if (strlen(sp[OEnat])>0)
          pclub->getDI().setString("Nationality", sp[OEnat]);

        pclub->synchronize(true);
      }

      int id = atoi(sp[OEid]);
      pRunner pr = 0;

      if (id>0)
        pr = event.getRunner(id, 0);

      if (pr == 0) {
        if (id==0) {
          oRunner r(&event);
          pr = event.addRunner(r, true);
        }
        else {
          oRunner r(&event, id);
          pr = event.addRunner(r, true);
        }
      }

      if (pr==0)
        continue;

      if (id>0)
        pr->setExtIdentifier(id);

      string name = string(sp[OEfirstname])+" "+string(sp[OEsurname]);
      pr->setName(name);
      pr->setClubId(pclub ? pclub->getId():0);
      pr->setCardNo( atoi(sp[OEcard]), false );

      pr->setStartTime(event.convertAbsoluteTime(sp[OEstart]), true, false);
      pr->setFinishTime(event.convertAbsoluteTime(sp[OEfinish]));

      if (strlen(sp[OEstatus])>0)
        pr->setStatus( ConvertOEStatus( atoi(sp[OEstatus]) ), true, false);

      if (pr->getStatus()==StatusOK && pr->getRunningTime()==0)
        pr->setStatus(StatusUnknown, true, false);

      //Autocreate class if it does not exist...
      int classId=atoi(sp[OEclassno]);
      if (classId>0) {
        pClass pc=event.getClassCreate(classId, sp[OEclass]);

        if (pc) {
          pc->synchronize();
          pr->setClassId(pc->getId());
        }
      }
      int stno=atoi(sp[OEstno]);

      if (stno>0)
        pr->setStartNo(stno, false);
      else
        pr->setStartNo(nimport, false);

      oDataInterface DI=pr->getDI();

      DI.setString("Sex", sp[OEsex]);
      DI.setInt("BirthYear", extendYear(atoi(sp[OEbirth])));
      DI.setString("Nationality", sp[OEnat]);

      if (sp.size()>OEbib)
        pr->setBib(sp[OEbib], false, false);

      if (sp.size()>=38) {//ECO
        DI.setInt("Fee", atoi(sp[OEfee]));
        DI.setInt("CardFee", atoi(sp[OErent]));
        DI.setInt("Paid", atoi(sp[OEpaid]));
      }

      if (sp.size()>=40) {//Course
        if (pr->getCourse(false) == 0){
          const char *cid=sp[OEcourseno];
          const int courseid=atoi(cid);
          if (courseid>0) {
            pCourse course=event.getCourse(courseid);

            if (!course) {
              oCourse oc(&event, courseid);
              oc.setLength(int(atof(sp[OElength])*1000));
              oc.setName(sp[OEcourse]);
              course = event.addCourse(oc);
              if (course)
                course->synchronize();
            }
            if (course) {
              if (pr->getClassId() != 0)
                event.getClass(pr->getClassId())->setCourse(course);
              else
                pr->setCourseId(course->getId());
            }
          }
        }
      }
      if (pr)
        pr->synchronize();
    }
  }
  fin.close();

  return true;
}

bool csvparser::openOutput(const char *filename)
{
  //Startnr;Bricka;Databas nr.;Efternamn;Förnamn;År;K;Block;ut;Start;Mål;Tid;Status;Klubb nr.;Namn;Ort;Land;Klass nr.;Kort;Lång;Num1;Num2;Num3;Text1;Text2;Text3;Adr. namn;Gata;Rad 2;Post nr.;Ort;Tel;Fax;E-post;Id/Club;Hyrd;Startavgift;Betalt;Bana nr.;Bana;km;Hm;Bana kontroller
  fout.open(filename);

  if (fout.bad())
    return false;
  return true;
}

bool csvparser::OutputRow(const string &row)
{
  fout << row << endl;
  return true;
}
bool csvparser::OutputRow(vector<string> &out)
{
  int size=out.size();

  for(int i=0;i<size;i++){
    string p=out[i];

    //Replace " with '
    int found=p.find_first_of("\"");
    while (found!=string::npos){
      p[found]='\'';
      found=p.find_first_of("\"",found+1);
    }

    if (i>0) fout << ";";

    if (p.find_first_of("; ,\t.")!=string::npos)
      fout << "\"" << p << "\"";
    else fout << p;
  }
  fout << endl;
  fout.flush();
  return true;
}

bool csvparser::closeOutput()
{
  fout.close();
  return true;
}


int csvparser::split(char *line, vector<char *> &split_vector)
{

  int len=strlen(line);
  bool cite=false;

  for(int m=0;m<len;m++)
  {
    char *ptr=&line[m];

    if (*ptr=='"')
      ptr++;

    while(line[m])
    {
      if (!cite && line[m]==';')
        line[m]=0;
      else
      {
        if (line[m]=='"')
        {
          cite=!cite;
          line[m]=0;
          if (cite)
            ptr=&line[m+1];
        }
        m++;
      }
    }
    line[m]=0;

    split_vector.push_back(ptr);

    //m++;
  }

  return 0;
}

bool csvparser::ImportOCAD_CSV(oEvent &event, const char *file, bool addClasses) {
  fin.open(file);

  if (!fin.good())
    return false;

  char bf[1024];

  while(!fin.eof()) {
    fin.getline(bf, 1024);
    vector<char *> sp;
    split(bf, sp);
    if (sp.size()>7) {
      size_t firstIndex = 7;
      bool hasLengths = true;
      int offset = 0;
      if (atoi(sp[firstIndex]) < 30) {
        firstIndex = 6;
        offset = -1;
      }

      if (atoi(sp[firstIndex])<30 || atoi(sp[firstIndex])>1000) {
        string str = "Ogiltig banfil. Kontroll förväntad på position X, men hittade 'Y'.#"
                      + itos(firstIndex+1) + "#" + sp[firstIndex];
        throw std::exception(str.c_str());
      }

      while (firstIndex > 0 && atoi(sp[firstIndex])>30 && atoi(sp[firstIndex])>1000) {
        firstIndex--;
      }
      string Start = lang.tl("Start ") + "1";
      double Length = 0;
      string Course = event.getAutoCourseName();
      string Class = "";

      if (firstIndex>=6) {
        Class = sp[0];
        Course = sp[1+offset];
        Start = sp[5+offset];
        Length = atof(sp[3+offset]);

        if (Start[0]=='S') {
          int num = atoi(Start.substr(1).c_str());
          if (num>0)
            Start = lang.tl("Start ") + Start.substr(1);
        }
      }
      else
        hasLengths = false;

      //Get controls
      pCourse pc=event.getCourse(Course);

      if (!pc) {
        pc = event.addCourse(Course, int(Length*1000));
      }
      else {
        // Reset control
        pc->importControls("");
        pc->setLength(int(Length*1000));
      }


      vector<int> legLengths;

      if (hasLengths) {
        double legLen = atof(sp[firstIndex-1]); // Length in km
        if (legLen > 0.001 && legLen < 30)
          legLengths.push_back(int(legLen*1000));
      }

      if (pc) {
        for (size_t k=firstIndex; k<sp.size()-1;k+=(hasLengths ? 2 : 1)) {
          string ctrlStr = trim(sp[k]);
          if (!ctrlStr.empty()) {
            int ctrl = atoi(ctrlStr.c_str());

            if (ctrl == 0 && trim(sp[k+1]).length() == 0)
              break; // Done

            if (ctrl >= 30 && ctrl < 1000)
              pc->addControl(atoi(sp[k]));
            else {
              string str = "Oväntad kontroll 'X' i bana Y.#" + ctrlStr + "#" + pc->getName();
              throw std::exception(str.c_str());
            }
          }
          if (hasLengths) {
            double legLen = atof(sp[k+1]); // Length in km
            if (legLen > 0.001 && legLen < 30)
              legLengths.push_back(int(legLen*1000));
          }
        }
        pc->setLength(int(Length*1000));
        pc->setStart(Start, true);

        if (legLengths.size() == pc->getNumControls()+1)
          pc->setLegLengths(legLengths);

        if (!Class.empty() && addClasses) {
          pClass cls = event.getBestClassMatch(Class);
          if (!cls)
            cls = event.addClass(Class);

          if (cls->getNumStages()==0) {
            cls->setCourse(pc);
          }
          else {
            for (size_t i = 0; i<cls->getNumStages(); i++)
              cls->addStageCourse(i, pc->getId());
          }

          cls->synchronize();
        }

        pc->synchronize();
      }
    }
  }
  return true;
}


bool csvparser::ImportRAID(oEvent &event, const char *file)
{
  enum {RAIDid=0, RAIDteam=1, RAIDcity=2, RAIDedate=3, RAIDclass=4,
        RAIDclassid=5, RAIDrunner1=6, RAIDrunner2=7, RAIDcanoe=8};

  fin.open(file);

  if (!fin.good())
    return false;

  char bf[1024];
  fin.getline(bf, 1024);

  nimport=0;
  while (!fin.eof()) {
    fin.getline(bf, 1024);

    vector<char *> sp;
    split(bf, sp);

    if (sp.size()>8) {
      nimport++;

      int ClubId=0;
      //Create class with this class number...
      int ClassId=atoi(sp[RAIDclassid]);
      pClass pc = event.getClassCreate(ClassId, sp[RAIDclass]);
      ClassId = pc->getId();

      //Club is autocreated...
      pTeam team=event.addTeam(sp[RAIDteam], ClubId,  ClassId);

      team->setStartNo(atoi(sp[RAIDid]), false);
      team->getDI().setInt("SortIndex", atoi(sp[RAIDcanoe]));
      oDataInterface teamDI=team->getDI();
      teamDI.setDate("EntryDate", sp[RAIDedate]);

      //Import runners!
      pRunner r1=event.addRunner(sp[RAIDrunner1], ClubId, ClassId, 0, 0, false);
      team->setRunner(0, r1, false);

      pRunner r2=event.addRunner(sp[RAIDrunner2], ClubId, ClassId, 0, 0, false);
      team->setRunner(1, r2, false);

      if (pc) {
        if (pc->getNumStages()<2)
          pc->setNumStages(2);

        pc->setLegType(0, LTNormal);
        pc->setLegType(1, LTIgnore);
      }
      team->apply(true, 0, false);
    }
  }
  fin.close();

  return true;
}

bool csvparser::importPunches(const oEvent &oe, const char *file, vector<PunchInfo> &punches)
{
  punches.clear();
  fin.clear();
  fin.open(file);
  if (!fin.good())
    return false;

  char bf[1024];
  fin.getline(bf, 1024);

  nimport=0;
  while (!fin.eof()) {
    fin.getline(bf, 1024);

    vector<char *> sp;
    split(bf, sp);

    if (sp.size()==4) {
      int no = atoi(sp[0]);
      int card = atoi(sp[1]);
      int time = oe.getRelativeTime(sp[3]);

      if (no>0 && card>0) {
        PunchInfo pi;
        pi.card = card;
        pi.time = time;
        strncpy_s(pi.date, sizeof(pi.date), sp[2], 26);
        pi.date[26] = 0;
        punches.push_back(pi);
        nimport++;
      }
    }
  }
  fin.close();

  return true;
}

int analyseSITime(const oEvent &oe, const char *dow, const char *time)
{
  int t=-1;
  if (trim(dow).length()>0)
    t = oe.getRelativeTime(time);
  else
    t = oe.getRelativeTimeFrom12Hour(time);

  if (t<0)
    t=0;

  return t;
}


bool csvparser::checkSimanLine(const oEvent &oe, const vector<char *> &sp, SICard &card) {
  if (sp.size() <= 11)
    return false;

  int cardNo = atoi(sp[1]);

  if (cardNo < 1000 || cardNo > 9999999)
    return false;

  int start = convertAbsoluteTimeMS(sp[5]);
  int finish = convertAbsoluteTimeMS(sp[6]);
  vector< pair<int, int> > punches;
  for (size_t k=10; k + 1<sp.size(); k+=2) {
    int code = atoi(sp[k]);
    int time = convertAbsoluteTimeMS(sp[k+1]);
    if (code > 0) {
      punches.push_back(make_pair(code, time));
    }
    else {
      return false;
    }
  }

  if (punches.size() > 2) {
    card.clear(0);
    card.CardNumber = cardNo;
    if (start > 0) {
      card.StartPunch.Code = 0;
      card.StartPunch.Time = start;
    }
    else {
      card.StartPunch.Code = -1;
    }

    if (finish > 0) {
      card.FinishPunch.Code = 0;
      card.FinishPunch.Time = finish;
    }
    else {
      card.FinishPunch.Code = -1;
    }

    for (size_t k = 0; k<punches.size(); k++) {
      card.Punch[k].Code = punches[k].first;
      card.Punch[k].Time = punches[k].second;
    }
    card.nPunch = punches.size();
    card.convertedTime = true;
    return true;
  }

  return false;
}

bool csvparser::importCards(const oEvent &oe, const char *file, vector<SICard> &cards)
{
  cards.clear();
  fin.clear();
  fin.open(file);

  if (!fin.good())
    return false;

  //[1024*16];
  int s = 1024*256;
  vector<char> bbf(s);
  char *bf = &bbf[0];
  fin.getline(bf, s);

  nimport=0;
  while (!fin.eof()) {
    fin.getline(bf, s);

    vector<char *> sp;
    split(bf, sp);

    SICard card;

    if (checkSimanLine(oe, sp, card)) {
      cards.push_back(card);
      nimport++;
    }
    else if (sp.size()>28) {
      int no = atoi(sp[0]);
      card.CardNumber = atoi(sp[2]);
      strncpy_s(card.FirstName, sp[5], 20);
      strncpy_s(card.LastName, sp[6], 20);
      strncpy_s(card.Club, sp[7], 40);

      if (trim(sp[21]).length()>1) {
        card.CheckPunch.Code = atoi(sp[19]);
        card.CheckPunch.Time = analyseSITime(oe, sp[20], sp[21]);
      }
      else {
        card.CheckPunch.Code = -1;
        card.CheckPunch.Time = 0;
      }

      if (trim(sp[24]).length()>1) {
        card.StartPunch.Code = atoi(sp[22]);
        card.StartPunch.Time = analyseSITime(oe, sp[23], sp[24]);
      }
      else {
        card.StartPunch.Code = -1;
        card.StartPunch.Time = 0;
      }

      if (trim(sp[27]).length()>1) {
        card.FinishPunch.Code = atoi(sp[25]);
        card.FinishPunch.Time = analyseSITime(oe, sp[26], sp[27]);
      }
      else  {
        card.FinishPunch.Code = -1;
        card.FinishPunch.Time = 0;
      }

      card.nPunch = atoi(sp[28]);
      if (no>0 && card.CardNumber>0 && card.nPunch>0) {
        if (sp.size()>28+3*card.nPunch) {
          for (unsigned k=0;k<card.nPunch;k++) {
            card.Punch[k].Code = atoi(sp[29+k*3]);
            card.Punch[k].Time = analyseSITime(oe, sp[30+k*3], sp[31+k*3]);
          }
          card.PunchOnly = false;
          nimport++;
          card.convertedTime = true;
          cards.push_back(card);
        }
      }
    }
  }
  fin.close();

  return true;
}


void csvparser::parse(const string &file, list< vector<string> > &data) {
  data.clear();

  fin.open(file.c_str());
  char bf[1024];
  if (!fin.good())
    throw meosException("Failed to read file");

  while(!fin.eof()) {
    fin.getline(bf, 1024);
    vector<char *> sp;
    split(bf, sp);

    if (!sp.empty()) {
      data.push_back(vector<string>());
      data.back().resize(sp.size());
      for (size_t k = 0; k < sp.size(); k++) {
        data.back()[k] = sp[k];
      }
    }
  }
  fin.close();
}

void csvparser::importTeamLineup(const string &file,
                                 const map<string, int> &classNameToNumber,
                                 vector<TeamLineup> &teams) {
  list< vector<string> > data;
  parse(file, data);
  teams.clear();
  teams.reserve(data.size()/3);
  int membersToRead = 0;
  int lineNo = 1;
  while (!data.empty()) {
    vector<string> &line = data.front();
    if (!line.empty()) {
      if (membersToRead == 0) {
        if (line.size() < 2 || line.size() > 3)
          throw meosException("Ogiltigt lag på rad X.#" + itos(lineNo) + ": " + line[0]);
        const string cls = trim(line[0]);
        map<string, int>::const_iterator res = classNameToNumber.find(cls);
        if (res == classNameToNumber.end())
          throw meosException("Okänd klass på rad X.#" + itos(lineNo) + ": " + cls);
        if (res->second <= 1)
          throw meosException("Klassen X är individuell.#" + cls);

        membersToRead = res->second;
        teams.push_back(TeamLineup());
        teams.back().teamClass = cls;
        teams.back().teamName = trim(line[1]);
        if (line.size() >= 3)
          teams.back().teamClub = trim(line[2]);
      }
      else {
        membersToRead--;
        teams.back().members.push_back(TeamLineup::TeamMember());
        TeamLineup::TeamMember &member = teams.back().members.back();
        member.name = trim(line[0]);
        if (line.size()>1)
          member.cardNo = atoi(line[1].c_str());
        else
          member.cardNo = 0;

        if (line.size()>2)
          member.club = trim(line[2]);

        if (line.size() > 3)
          member.course = trim(line[3]);

        if (line.size() > 4)
          member.cls = trim(line[4]);
      }
    }
    else if (membersToRead>0) {
      membersToRead--;
      teams.back().members.push_back(TeamLineup::TeamMember());
    }
    lineNo++;
    data.pop_front();
  }
}
