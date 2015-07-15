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

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h>

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include "classconfiginfo.h"
#include "onlineresults.h"
#include "onlineinput.h"

#include "TabAuto.h"
#include "TabSI.h"
#include "meos_util.h"
#include <shellapi.h>

#include "gdiconstants.h"
#include "meosexception.h"

static TabAuto *tabAuto = 0;

extern HWND hWndMain;
extern HWND hWndWorkspace;

TabAuto::TabAuto(oEvent *poe):TabBase(poe)
{
  synchronize=false;
  synchronizePunches=false;
}

AutoMachine* AutoMachine::construct(Machines ms) {
  switch(ms) {
  case mPrintResultsMachine:
    return new PrintResultMachine(0);
  case mSplitsMachine:
    return new SplitsMachine();
  case mPrewarningMachine:
    return new PrewarningMachine();
  case mPunchMachine:
    return new PunchMachine();
  case mOnlineInput:
    return new OnlineInput();
  case mOnlineResults:
    return new OnlineResults();
  }
  throw meosException("Invalid machine");
}



TabAuto::~TabAuto(void)
{
  list<AutoMachine *>::iterator it;
  for (it=machines.begin(); it!=machines.end(); ++it) {
    delete *it;
    *it=0;
  }
  tabAuto=0;
}


void tabAutoKillMachines()
{
  if (tabAuto)
    tabAuto->killMachines();
}

void tabAutoRegister(TabAuto *ta)
{
  tabAuto=ta;
}

void tabAutoAddMachinge(const AutoMachine &am)
{
  if (tabAuto) {
    tabAuto->addMachine(am);
  }
}

void tabForceSync(gdioutput &gdi, pEvent oe)
{
  if (tabAuto)
    tabAuto->syncCallback(gdi);
}

int AutomaticCB(gdioutput *gdi, int type, void *data)
{
  if (!tabAuto)
    throw std::exception("tabAuto undefined.");

  switch(type){
    case GUI_BUTTON: {
      //Make a copy
      ButtonInfo bu=*static_cast<ButtonInfo *>(data);
      return tabAuto->processButton(*gdi, bu);
             }
    case GUI_LISTBOX:{
      ListBoxInfo lbi=*static_cast<ListBoxInfo *>(data);
      return tabAuto->processListBox(*gdi, lbi);
             }
  }
  return 0;
}

void TabAuto::syncCallback(gdioutput &gdi)
{
  string msg;
  try {
    list<AutoMachine *>::iterator it;
    for (it=machines.begin(); it!=machines.end(); ++it) {
      AutoMachine *am=*it;
      if (am && am->synchronize && !am->isEditMode())
        am->process(gdi, oe, SyncDataUp);
    }
  }
  catch(std::exception &ex) {
    msg=ex.what();
  }
  catch(...) {
    msg="Ett okänt fel inträffade.";
  }
  if (!msg.empty()) {
    gdi.alert(msg);
    gdi.setWaitCursor(false);
  }

}

void TabAuto::updateSyncInfo()
{
  list<AutoMachine *>::iterator it;
  synchronize=false;
  synchronizePunches=false;

  for (it=machines.begin(); it!=machines.end(); ++it) {
    AutoMachine *am=*it;
    if (am){
      am->synchronize= am->synchronize || am->synchronizePunches;
      synchronize=synchronize || am->synchronize;
      synchronizePunches=synchronizePunches || am->synchronizePunches;
    }
  }
}

void TabAuto::timerCallback(gdioutput &gdi)
{
  DWORD tc=GetTickCount();

  list<AutoMachine *>::iterator it;
  bool reload=false;

  for (it=machines.begin(); it!=machines.end(); ++it) {
    AutoMachine *am=*it;
    if (am && am->interval && tc >= am->timeout && !am->isEditMode()) {
      am->process(gdi, oe, SyncTimer);
      setTimer(am);
      reload=true;
    }
  }

  DWORD d=0;
  if (reload && !editMode && gdi.getData("AutoPage", d) && d)
    loadPage(gdi);
  /*if (printResultIntervalSec>0) {

    if (tc>printResultTimeOut) {

      gdioutput gdi;
      //gdi.Init(hWndWorkspace, hWndMain, 0);
      gdi.ClearPage();

      oe->GenerateResultlistPrint(gdi);

      HDC hDC=GetDC(hWndWorkspace);
      RECT rc;
      rc.right=1000;
      rc.left=0;
      rc.bottom=1000;
      rc.top=0;
      gdi.Draw(hDC, rc);

      ReleaseDC(hWndWorkspace, hDC);

      setTimer(printResultTimeOut, printResultIntervalSec);
    }
  }*/
}

void TabAuto::setTimer(AutoMachine *am)
{
  DWORD tc=GetTickCount();

  if (am->interval>0) {
    DWORD to=am->interval*1000+tc;

    if (to<tc) { //Warp-around. No reasonable way to handle it
      to=DWORD(0);
      am->interval=0;
    }

    am->timeout=to;
  }
}

int TabAuto::processButton(gdioutput &gdi, const ButtonInfo &bu)
{

  if (bu.id=="GenerateCMP") {
#ifndef MEOSDB
    int nClass=gdi.getTextNo("nClass");
    int nRunner=gdi.getTextNo("nRunner");

    if (nRunner>0 &&
      gdi.ask("Vill du dumpa aktuellt tävling och skapa en testtävling?")) {
      oe->generateTestCompetition(nClass, nRunner,
        gdi.isChecked("UseRelay"));
    }
#endif
  }
  if (bu.id=="Result") {
    PrintResultMachine *sm=dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mPrintResultsMachine);
  }
  else if (bu.id == "BrowseFile") {
    static int index = 0;
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Webbdokument", "*.html;*.htm"));

    string file = gdi.browseForSave(ext, "html", index);
    if (!file.empty())
      gdi.setText("ExportFile", file);
  }
  else if (bu.id == "BrowseScript") {
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Skript", "*.bat;*.exe;*.js"));

    string file = gdi.browseForOpen(ext, "bat");
    if (!file.empty())
      gdi.setText("ExportScript", file);
  }
  else if (bu.id == "DoExport") {
    bool stat = gdi.isChecked(bu.id);
    gdi.setInputStatus("ExportFile", stat);
    gdi.setInputStatus("ExportScript", stat);
    gdi.setInputStatus("BrowseFile", stat);
    gdi.setInputStatus("BrowseScript", stat);
    gdi.setInputStatus("HTMLRefresh", stat);
    gdi.setInputStatus("StructuredExport", stat);
  }
  else if (bu.id == "DoPrint") {
    bool stat = gdi.isChecked(bu.id);
    gdi.setInputStatus("PrinterSetup", stat);
  }
  else if (bu.id=="Splits") {
    SplitsMachine *sm=dynamic_cast<SplitsMachine*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mSplitsMachine);
  }
  else if (bu.id=="Prewarning") {
    PrewarningMachine *sm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mPrewarningMachine);
  }
  else if (bu.id=="Punches") {
    PunchMachine *sm=dynamic_cast<PunchMachine*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mPunchMachine);
  }
  else if (bu.id=="OnlineResults") {
    OnlineResults *sm=dynamic_cast<OnlineResults*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mOnlineResults);
  }
  else if (bu.id=="OnlineInput") {
    OnlineInput *sm=dynamic_cast<OnlineInput*>((AutoMachine*)bu.getExtra());
    settings(gdi, sm, mOnlineInput);
  }
  else if (bu.id=="StartResult") {
#ifndef MEOSDB
    string minute=gdi.getText("Interval");
    int t=convertAbsoluteTimeMS(minute);

    if (t<2 || t>7200) {
      gdi.alert("Intervallet måste anges på formen MM:SS.");
    }
    else {
      PrintResultMachine *prm=dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());

      if (prm) {
        prm->interval = t;
        prm->doExport = gdi.isChecked("DoExport");
        prm->doPrint = gdi.isChecked("DoPrint");
        prm->exportFile = gdi.getText("ExportFile");
        prm->exportScript = gdi.getText("ExportScript");
        prm->structuredExport = gdi.isChecked("StructuredExport");
        prm->htmlRefresh = gdi.isChecked("HTMLRefresh") ? t : 0;
        if (!prm->readOnly) {
          gdi.getSelection("Classes", prm->classesToPrint);

          ListBoxInfo lbi;
          if (gdi.getSelectedItem("ListType", &lbi)) {
            oListParam par;
            par.selection=prm->classesToPrint;
            par.listCode = EStdListType(lbi.data);
            par.pageBreak = gdi.isChecked("PageBreak");
            par.showInterTimes = gdi.isChecked("ShowInterResults");
            par.splitAnalysis = gdi.isChecked("SplitAnalysis");
            gdi.getSelectedItem("LegNumber", &lbi);
            if (signed(lbi.data)>=0)
              par.legNumber = lbi.data == 1000 ? -1: lbi.data;
            else
              par.legNumber = 0;

            oe->generateListInfo(par, gdi.getLineHeight(), prm->listInfo);
          }
        }
        prm->po.onlyChanged = gdi.isChecked("OnlyChanged");
        prm->pageBreak = gdi.isChecked("PageBreak");
        prm->showInterResult = gdi.isChecked("ShowInterResults");
        prm->splitAnalysis = gdi.isChecked("SplitAnalysis");
        prm->synchronize=true; //To force continuos data sync.
        setTimer(prm);
      }
      updateSyncInfo();
      loadPage(gdi);
    }
#endif
  }
  else if (bu.id=="StartSplits") {
    int iv=gdi.getTextNo("Interval");
    const string &file=gdi.getText("FileName");

    if (file.empty()) {
      gdi.alert("Filnamnet får inte vara tomt");
      return false;
    }

    //Try exporting.
#ifndef MEOSDB
    oe->exportIOFSplits(oEvent::IOF20, file.c_str(), true, false, set<int>(), -1, false, true);
#endif

    SplitsMachine *sm=dynamic_cast<SplitsMachine*>((AutoMachine*)bu.getExtra());

    if (sm) {
      sm->interval=iv;

      sm->file=file;
      sm->synchronize=true;
      setTimer(sm);
    }
    updateSyncInfo();
    loadPage(gdi);
  }
  else if (bu.id=="Save") { // General save
    AutoMachine *sm=static_cast<AutoMachine*>(bu.getExtra());
    if (sm) {
      sm->save(*oe, gdi);
      setTimer(sm);
    }
    updateSyncInfo();
    loadPage(gdi);
  }
  else if (bu.id=="StartPrewarning") {
    PrewarningMachine *pwm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());

    if (pwm) {
      pwm->waveFolder=gdi.getText("WaveFolder");
      gdi.getSelection("Controls", pwm->controls);

      oe->synchronizeList(oLPunchId);
      oe->clearPrewarningSounds();

      pwm->synchronizePunches=true;
      pwm->controlsSI.clear();
      for (set<int>::iterator it=pwm->controls.begin();it!=pwm->controls.end();++it) {
        pControl pc=oe->getControl(*it, false);

        if (pc)
          pwm->controlsSI.insert(pc->Numbers, pc->Numbers+pc->nNumbers);
      }
    }
    updateSyncInfo();
    loadPage(gdi);
  }
  else if (bu.id=="StartPunch") {

    string minute=gdi.getText("Interval");
    int t=convertAbsoluteTimeMS(minute);

    if (t<1 || t>7200) {
      gdi.alert("Intervallet måste anges på formen MM:SS");
    }
    else {
      PunchMachine *pm=dynamic_cast<PunchMachine*>((AutoMachine*)bu.getExtra());

      if (pm) {
        pm->interval=t;
        pm->radio=gdi.getTextNo("Radio");
        setTimer(pm);
      }
    }
    updateSyncInfo();
    loadPage(gdi);
  }
  else if (bu.id == "Cancel") {
    loadPage(gdi);
  }
  else if (bu.id == "Stop") {
    if (bu.getExtra())
      stopMachine(static_cast<AutoMachine*>(bu.getExtra()));

    updateSyncInfo();
    loadPage(gdi);
  }
  else if (bu.id == "PrinterSetup") {
    PrintResultMachine *prm =
          dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());

    if (prm) {
      gdi.printSetup(prm->po);
    }
  }
  else if (bu.id == "PrintNow") {
     PrintResultMachine *prm =
          dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());

     if (prm) {
       prm->process(gdi, oe, SyncNone);
       setTimer(prm);
       loadPage(gdi);
     }
  }
  else if (bu.id == "SelectAll") {
    const char *ctrl=
      static_cast<const char *>(bu.getExtra());
    set<int> lst;
    lst.insert(-1);
    gdi.setSelection(ctrl, lst);
  }
  else if (bu.id == "SelectNone") {
    const char *ctrl=
      static_cast<const char *>(bu.getExtra());
    set<int> lst;
    gdi.setSelection(ctrl, lst);
  }
  else if (bu.id == "TestVoice") {
    PrewarningMachine *pwm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());

    if (pwm)
      oe->tryPrewarningSounds(pwm->waveFolder, rand()%400+1);
  }
  else if ( bu.id == "WaveBrowse") {
    string wf=gdi.browseForFolder(gdi.getText("WaveFolder"));

    if (wf.length()>0)
      gdi.setText("WaveFolder", wf);
  }
  else if ( bu.id == "BrowseSplits") {
    int index=0;
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Sträcktider", "*.xml"));

    string wf = gdi.browseForSave(ext, "xml", index);

    if (!wf.empty())
      gdi.setText("FileName", wf);
  }

  return 0;
}

int TabAuto::processListBox(gdioutput &gdi, const ListBoxInfo &bu)
{
  return 0;
}

bool TabAuto::stopMachine(AutoMachine *am)
{
  list<AutoMachine *>::iterator it;
  for (it=machines.begin(); it!=machines.end(); ++it)
    if (am==*it)  {
      if (am->stop()) {
        delete am;
        machines.erase(it);
        return true;
      }
    }
  return false;
}

void TabAuto::settings(gdioutput &gdi, AutoMachine *sm, Machines ms) {
    editMode=true;
    bool createNew = (sm==0);
    if (!sm) {
      sm = AutoMachine::construct(ms);
      machines.push_back(sm);
    }

    gdi.restore("", false);
    gdi.dropLine();
    int cx = gdi.getCX();
    int cy = gdi.getCY();
    int d = gdi.scaleLength(6);
    gdi.setCX(cx + d);
    sm->setEditMode(true);
    sm->settings(gdi, *oe, createNew);
    int w = gdi.getWidth();
    int h = gdi.getHeight();

    RECT rc;
    rc.top = cy - d;
    rc.bottom = h + d;
    rc.left = cx - d;
    rc.right = w + d;
    gdi.addRectangle(rc, colorLightBlue, true, true);
    gdi.refresh();
}

void TabAuto::killMachines()
{
  while(!machines.empty()) {
    machines.back()->stop();
    delete machines.back();
    machines.pop_back();
  }
}

bool TabAuto::loadPage(gdioutput &gdi)
{
  oe->checkDB();
  tabAuto=this;
  editMode=false;
  DWORD isAP = 0;
  gdi.getData("AutoPage", isAP);
  int storedOY = 0;
  int storedOX = 0;
  if (isAP) {
    storedOY = gdi.GetOffsetY();
    storedOX = gdi.GetOffsetX();
  }

  gdi.clearPage(false);
  gdi.setData("AutoPage", 1);
  gdi.addString("", boldLarge, "Automater");
  gdi.setRestorePoint();

  gdi.addString("", 10, "help:10000");

  gdi.dropLine();
  gdi.addString("", fontMediumPlus, "Tillgängliga automater").setColor(colorDarkBlue);
  gdi.dropLine();
  gdi.fillRight();
  gdi.pushX();
  gdi.addButton("Result", "Resultatutskrift / export", AutomaticCB, "tooltip:resultprint");
  gdi.addButton("OnlineResults", "Resultat online", AutomaticCB, "Publicera resultat direkt på nätet");
  gdi.addButton("OnlineInput", "Inmatning online", AutomaticCB, "Hämta stämplingar m.m. från nätet");
  gdi.popX();
  gdi.dropLine(2.5);
  gdi.addButton("Splits", "Sträcktider (WinSplits)", AutomaticCB, "Spara sträcktider till en fil för automatisk synkronisering med WinSplits");
  gdi.addButton("Prewarning", "Förvarningsröst", AutomaticCB, "tooltip:voice");
  gdi.addButton("Punches", "Stämplingstest", AutomaticCB, "Simulera inläsning av stämplar");

  gdi.fillDown();
  gdi.dropLine(3);
  gdi.popX();

  if (!machines.empty()) {
    gdi.addString("", fontMediumPlus, "Startade automater").setColor(colorDarkBlue);;
    list<AutoMachine *>::iterator it;

    int baseX = gdi.getCX();
    int dx = gdi.scaleLength(6);

    for (it=machines.begin(); it!=machines.end(); ++it) {
      AutoMachine *am=*it;
      if (am) {
        RECT rc;
        rc.left = baseX;
        rc.right = gdi.scaleLength(500);
        rc.top = gdi.getCY();
        gdi.dropLine(0.5);
        gdi.setCX(baseX+dx);
        am->setEditMode(false);
        am->status(gdi);
        gdi.setCX(baseX);
        gdi.dropLine(0.5);
        rc.bottom = gdi.getCY();
        gdi.addRectangle(rc, colorLightGreen, true, true);
        gdi.dropLine();
      }
    }
    gdi.dropLine();
  }

  if (isAP) {
    gdi.setOffset(storedOY, storedOY, true);
  }

  gdi.refresh();
  return true;
}

void AutoMachine::settingsTitle(gdioutput &gdi, char *title) {
  gdi.dropLine(0.5);
  gdi.addString("", fontMediumPlus, title).setColor(colorDarkBlue);
  gdi.dropLine(0.5);
}

void AutoMachine::startCancelInterval(gdioutput &gdi, char *startCommand, bool created, IntervalType type, const string &interval) {
  gdi.pushX();
  gdi.fillRight();
  if (type == IntervalMinute)
    gdi.addInput("Interval", interval, 7, 0, "Tidsintervall (MM:SS):");
  else if (type == IntervalSecond)
    gdi.addInput("Interval", interval, 7, 0, "Tidsintervall (sekunder):");
  gdi.dropLine(1);
  gdi.addButton(startCommand, "Starta automaten", AutomaticCB).setExtra(this);
  gdi.addButton(created ? "Stop":"Cancel", "Avbryt", AutomaticCB).setExtra(this);

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(2.5);
  int dx = gdi.scaleLength(3);
  RECT rc;
  rc.left = gdi.getCX() - dx;
  rc.right = rc.left + gdi.scaleLength(450);
  rc.top = gdi.getCY();
  rc.bottom = rc.top + dx;
  gdi.addRectangle(rc, colorDarkBlue, false, false);
  gdi.dropLine();
}


void PrintResultMachine::settings(gdioutput &gdi, oEvent &oe, bool created) {
  settingsTitle(gdi, "Resultatutskrift / export");
  string time=created ? "10:00" : getTimeMS(interval);
  startCancelInterval(gdi, "StartResult", created, IntervalMinute, time);

  if (created) {
    oe.getAllClasses(classesToPrint);
  }

  gdi.pushX();
  gdi.fillRight();
  gdi.addCheckbox("DoPrint", "Skriv ut", AutomaticCB, doPrint);
  gdi.dropLine(-0.5);
  gdi.addButton("PrinterSetup", "Skrivare...", AutomaticCB, "Välj skrivare...").setExtra(this);

  gdi.dropLine(4);
  gdi.popX();
  gdi.addCheckbox("DoExport", "Exportera", AutomaticCB, doExport);
  gdi.dropLine(-1);
  int cx = gdi.getCX();
  gdi.addInput("ExportFile", exportFile, 32, 0, "Fil att exportera till:");
  gdi.dropLine(0.7);
  gdi.addButton("BrowseFile", "Bläddra...", AutomaticCB);
  gdi.setCX(cx);
  gdi.dropLine(2.3);
  gdi.addCheckbox("StructuredExport", "Strukturerat exportformat", 0, structuredExport);
  gdi.addCheckbox("HTMLRefresh", "HTML med AutoRefresh", 0, htmlRefresh != 0);
  gdi.dropLine(1.2);
  gdi.setCX(cx);
  gdi.addInput("ExportScript", exportScript, 32, 0, "Skript att köra efter export:");
  gdi.dropLine(0.7);
  gdi.addButton("BrowseScript", "Bläddra...", AutomaticCB);
  gdi.dropLine(3);
  gdi.popX();

  gdi.setInputStatus("ExportFile", doExport);
  gdi.setInputStatus("ExportScript", doExport);
  gdi.setInputStatus("BrowseFile", doExport);
  gdi.setInputStatus("BrowseScript", doExport);
  gdi.setInputStatus("StructuredExport", doExport);
  gdi.setInputStatus("HTMLRefresh", doExport);
  gdi.setInputStatus("PrinterSetup", doPrint);

  if (!readOnly) {
    gdi.fillDown();
    gdi.addString("", 1, "Listval");
    gdi.dropLine();
    gdi.fillRight();
    gdi.addListBox("Classes", 150,300,0,"","", true);
    gdi.pushX();
    gdi.fillDown();
    vector< pair<string, size_t> > d;
    gdi.addItem("Classes", oe.fillClasses(d, oEvent::extraNone, oEvent::filterNone));
    gdi.setSelection("Classes", classesToPrint);

    gdi.addSelection("ListType", 200, 100, 0, "Lista");
    oe.fillListTypes(gdi, "ListType", 1);
    if (notShown) {
      notShown = false;
      ClassConfigInfo cnf;
      oe.getClassConfigurationInfo(cnf);
      int type = EStdResultListLARGE;
      if (cnf.hasRelay())
        type = EStdTeamResultListLegLARGE;
      else if (cnf.hasPatrol())
        type = EStdRaidResultListLARGE;

      gdi.selectItemByData("ListType", type);

    }
    else
      gdi.selectItemByData("ListType", listInfo.getListCode());

    gdi.addSelection("LegNumber", 140, 300, 0, "Sträcka:");
    oe.fillLegNumbers(gdi, "LegNumber");
    int leg = listInfo.getLegNumber();
    gdi.selectItemByData("LegNumber", leg==-1 ? 1000 : leg);

    gdi.addCheckbox("PageBreak", "Sidbrytning mellan klasser", 0, pageBreak);
    gdi.addCheckbox("ShowInterResults", "Visa mellantider", 0, showInterResult,
                                        "Mellantider visas för namngivna kontroller.");
    gdi.addCheckbox("SplitAnalysis", "Med sträcktidsanalys", 0, splitAnalysis);

    gdi.addCheckbox("OnlyChanged", "Skriv endast ut ändade sidor", 0, po.onlyChanged);

    gdi.popX();
    gdi.addButton("SelectAll", "Välj allt", AutomaticCB, "").setExtra("Classes");
    gdi.popX();
    gdi.addButton("SelectNone", "Välj inget", AutomaticCB, "").setExtra("Classes");
  }
  else {
    gdi.fillDown();
    gdi.addString("", 1, "Lista av typ 'X'#" + listInfo.getName());
    gdi.addCheckbox("OnlyChanged", "Skriv endast ut ändade sidor", 0, po.onlyChanged);
  }
}

void PrintResultMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  #ifndef MEOSDB

  if (lock)
    return;

  if (ast!=SyncDataUp) {
    lock = true;
    try {
      gdioutput gdiPrint("print", gdi.getScale(), gdi.getEncoding());
      gdiPrint.clearPage(false);
      oe->generateList(gdiPrint, true, listInfo, false);
      if (doPrint) {
        gdiPrint.refresh();
        gdiPrint.print(po, oe);
      }
      if (doExport) {
        if (!exportFile.empty()) {
          if (structuredExport)
            gdiPrint.writeTableHTML(gdi.toWide(exportFile), oe->getName(), htmlRefresh);
          else
            gdiPrint.writeHTML(gdi.toWide(exportFile), oe->getName(), htmlRefresh);

          if (!exportScript.empty()) {
            ShellExecute(NULL, NULL, exportScript.c_str(), exportFile.c_str(), NULL, SW_HIDE);
          }
        }
      }
    }
    catch (...) {
      lock = false;
      throw;
    }
    lock = false;
  }
  #else
    throw std::exception("Bad method call");
  #endif
}

void PrintResultMachine::status(gdioutput &gdi)
{
  gdi.fillRight();
  gdi.pushX();
  gdi.addString("", 0, name);
  gdi.addString("", 0, listInfo.getName());
  gdi.dropLine();
  if (doExport) {
    gdi.popX();
    gdi.addString("", 0, "Målfil: ");
    gdi.addStringUT(0, exportFile).setColor(colorRed);
    gdi.dropLine();
  }
  gdi.fillRight();
  gdi.popX();
  if (interval>0){
    gdi.addString("", 0, "Automatisk utskrift / export: ");
    gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
  }
  else {

  }
  gdi.popX();
  gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.addButton("PrintNow", "Exportera nu", AutomaticCB).setExtra(this);
  gdi.fillDown();
  gdi.addButton("Result", "Inställningar...", AutomaticCB).setExtra(this);
  gdi.popX();
}

void PrewarningMachine::settings(gdioutput &gdi, oEvent &oe, bool created) {
  settingsTitle(gdi, "Förvarningsröst");
  startCancelInterval(gdi, "StartPrewarning", created, IntervalNone, "");

  gdi.addString("", 10, "help:computer_voice");

  gdi.pushX();
  gdi.fillRight();
  gdi.addInput("WaveFolder", waveFolder, 32, 0, "Ljudfiler, baskatalog.");

  gdi.fillDown();
  gdi.dropLine();
  gdi.addButton("WaveBrowse", "Bläddra...", AutomaticCB);
  gdi.popX();

  gdi.addListBox("Controls", 100, 200, 0, "", "", true);
  gdi.pushX();
  gdi.fillDown();
  vector< pair<string, size_t> > d;
  oe.fillControls(d, oEvent::CTCourseControl);
  gdi.addItem("Controls", d);
  gdi.setSelection("Controls", controls);
  gdi.popX();
  gdi.addButton("SelectAll", "Välj alla", AutomaticCB, "").setExtra("Controls");
  gdi.popX();
}

void PrewarningMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  oe->playPrewarningSounds(waveFolder, controlsSI);
}

void PrewarningMachine::status(gdioutput &gdi)
{
  gdi.addString("", 1, name);

  string info="Förvarning på (SI-kod): ";
  bool first=true;

  if (controls.empty())
    info+="alla stämplingar";
  else {
    for (set<int>::iterator it=controlsSI.begin();it!=controlsSI.end();++it) {
      char bf[32];
      _itoa_s(*it, bf, 10);

      if (!first) info+=", ";
      else       first=false;

      info+=bf;
    }
  }
  gdi.addString("", 0, info);
  gdi.fillRight();
  gdi.pushX();

  gdi.popX();
  gdi.dropLine(0.3);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.addButton("TestVoice", "Testa rösten", AutomaticCB).setExtra(this);
  gdi.fillDown();
  gdi.addButton("Prewarning", "Inställningar...", AutomaticCB).setExtra(this);
  gdi.popX();
}

void PunchMachine::settings(gdioutput &gdi, oEvent &oe, bool created) {
  settingsTitle(gdi, "Test av stämplingsinläsningar");
  string time=created ? "0:10" : getTimeMS(interval);
  startCancelInterval(gdi, "StartPunch", created, IntervalMinute, time);

  gdi.addString("", 10, "help:simulate");

  gdi.pushX();
  gdi.fillRight();
  gdi.dropLine();

  gdi.addString("", 0, "Radiotider, kontroll:");
  gdi.dropLine(-0.2);
  gdi.addInput("Radio", "", 6, 0);

  gdi.fillDown();
  gdi.popX();
  gdi.dropLine(5);
  gdi.addString("", 1, "Generera testtävling");
  gdi.fillRight();
  gdi.addInput("nRunner", "100", 10, 0, "Antal löpare");
  gdi.addInput("nClass", "10", 10, 0, "Antal klasser");
  gdi.dropLine();
  gdi.addCheckbox("UseRelay", "Med stafettklasser");
  gdi.addButton("GenerateCMP", "Generera testtävling", AutomaticCB);
}

void PunchMachine::status(gdioutput &gdi)
{
  gdi.addString("", 1, name);
  gdi.fillRight();
  gdi.pushX();
  if (interval>0){
    gdi.addString("", 0, "Stämplar om: ");
    gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
  }
  else {

  }
  gdi.popX();
  gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.fillDown();
  gdi.addButton("Punches", "Inställningar...", AutomaticCB).setExtra(this);
  gdi.popX();
}

void PunchMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
#ifndef MEOSDB
  SICard sic;
  oe->generateTestCard(sic);
  SportIdent &si = TabSI::getSI(gdi);
  if (!sic.empty()) {
    if (!radio) si.AddCard(sic);
  }
  else gdi.addInfoBox("", "Failed to generate card.", interval*2);

  if (radio && !sic.empty()) {
    pRunner r=oe->getRunnerByCardNo(sic.CardNumber, 0, false);
    if (r && r->getCardNo()) {
      sic.CardNumber=r->getCardNo();
      sic.PunchOnly=true;
      sic.nPunch=1;
      sic.Punch[0].Code=radio;
      si.AddCard(sic);
    }
  }
#endif
}

void SplitsMachine::settings(gdioutput &gdi, oEvent &oe, bool created) {
  string time = "";
  if (interval>0)
    time = itos(interval);
  else if (created)
    time = "30";

  settingsTitle(gdi, "Sträcktider / WinSplits");
  startCancelInterval(gdi, "StartSplits", created, IntervalSecond, time);

  gdi.addString("", 10, "help:winsplits_auto");


  gdi.dropLine();
  gdi.fillRight();
  gdi.addInput("FileName", file, 30, 0, "Filnamn:");
  gdi.dropLine(0.9);
  gdi.addButton("BrowseSplits", "Bläddra...", AutomaticCB);

  gdi.popX();
  gdi.dropLine(2);
  gdi.addString("",  0, "Intervall (sekunder). Lämna blankt för att uppdatera när "
                                        "tävlingsdata ändras.");

}

void SplitsMachine::status(gdioutput &gdi)
{
  gdi.addString("", 1, name);
  if (!file.empty()) {
    gdi.fillRight();
    gdi.pushX();
    gdi.addString("", 0, "Fil: X#" + file);

    if (interval>0){
      gdi.popX();
      gdi.dropLine(1);
      gdi.addString("", 0, "Skriver sträcktider om: ");
      gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
      gdi.addString("", 0, "(sekunder)");
    }
    else {
      gdi.dropLine(1);
      gdi.addString("", 0, "Skriver sträcktider när tävlingsdata ändras.");
    }

    gdi.popX();
  }
  gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.fillDown();
  gdi.addButton("Splits", "Inställningar...", AutomaticCB).setExtra(this);
  gdi.popX();
}

void SplitsMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
#ifndef MEOSDB
  if ((interval>0 && ast==SyncTimer) || (interval==0 && ast==SyncDataUp)) {
    if (!file.empty())
      oe->exportIOFSplits(oEvent::IOF20, file.c_str(), true, false, classes, leg, false, true);
  }
#endif
}
