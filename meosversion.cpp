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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden

************************************************************************/
#include "stdafx.h"
#include <vector>
#include "meos_util.h"

//ABCDEFGHIJKLMNO
//V2: ABCDEFGHIHJKMN
//V31: a
int getMeosBuild() {
  string revision("$Rev: 307 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuvxyz
//V2: abcdefgh
//V3: abcdefghijklmnopqrstuvxyz
//V31: abcde
//V32: abcdefgh
string getMeosDate() {
  string date("$Date: 2015-04-26 21:12:58 +0200 (sö, 26 apr 2015) $");
  return date.substr(7,10);
}

string getBuildType() {
  return ""; // No parantheses (...)
}

string getMajorVersion() {
  return "3.2";
}

string getMeosFullVersion() {
  char bf[256];
  string maj = getMajorVersion();
  if (getBuildType().empty())
    sprintf_s(bf, "Version X#%s.%d, %s", maj.c_str(), getMeosBuild(), getMeosDate().c_str());
  else
    sprintf_s(bf, "Version X#%s.%d, %s %s", maj.c_str(), getMeosBuild(), getBuildType().c_str(), getMeosDate().c_str());
  return bf;
}

string getMeosCompectVersion() {
  if (getBuildType().empty())
    return getMajorVersion() + "." + itos(getMeosBuild());
  else
    return getMajorVersion() + "." + itos(getMeosBuild()) + "(" + getBuildType() + ")";
}

void getSupporters(vector<string> &supp)
{
  supp.push_back("Centrum OK");
  supp.push_back("Ove Persson, Pite� IF");
  supp.push_back("OK Rodhen");
  supp.push_back("T�by Extreme Challenge");
  supp.push_back("Thomas Engberg, VK Uvarna");
  supp.push_back("Eilert Edin, Sidensj� IK");
  supp.push_back("G�ran Nordh, Trollh�ttans SK");
  supp.push_back("Roger Gustavsson, OK Tisaren");
  supp.push_back("Sundsvalls OK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Helsingborgs SOK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Rune Thur�n, Vallentuna-�sseby OL");
  supp.push_back("Roland Persson, Kalmar OK");
  supp.push_back("Robert Jessen, Fr�mmestads IK");
  supp.push_back("Almby IK, �rebro");
  supp.push_back("Peter Rydes�ter, Rehns BK");
  supp.push_back("IK Hakarpspojkarna");
  supp.push_back("Rydboholms SK");
  supp.push_back("IFK Kiruna");
  supp.push_back("Peter Andersson, S�ders SOL");
  supp.push_back("Bj�rkfors GoIF");
  supp.push_back("OK Ziemelkurzeme");
  supp.push_back("Big Foot Orienteers");
  supp.push_back("FIF Hiller�d");
  supp.push_back("Anne Udd");
  supp.push_back("OK Orinto");
  supp.push_back("SOK Tr�ff");
  supp.push_back("Gamleby OK");
  supp.push_back("V�nersborgs SK");
  supp.push_back("Henrik Ortman, V�ster�s SOK");
  supp.push_back("Leif Olofsson, Sjuntorp");
  supp.push_back("Vallentuna/�sseby OL");
  supp.push_back("Oskarstr�m OK");
  supp.push_back("Skogsl�parna");
  supp.push_back("OK Milan");
  supp.push_back("GoIF Tjalve");
  supp.push_back("OK Sk�rmen");
  supp.push_back("�stkredsen");
  supp.push_back("OK Roskilde");
  supp.push_back("Holb�k Orienteringsklub");
  supp.push_back("Bodens BK");
  supp.push_back("OK Tyr, Karlstad");
  supp.push_back("G�teborg-Majorna OK");
  supp.push_back("OK J�rnb�rarna, Kopparberg");
  supp.push_back("FK �sen");
  supp.push_back("Ballerup OK");
  supp.push_back("Olivier Benevello, Valbonne SAO");
  supp.push_back("Tommy W�hlin, OK Enen");
  supp.push_back("Hjobygdens OK");
  supp.push_back("Tisvilde Hegn OK");
  supp.push_back("Lindebygdens OK");
  supp.push_back("OK Flundrehof");
  supp.push_back("Vittj�rvs IK");
  supp.push_back("Annebergs GIF");
  supp.push_back("Lars-Eric Gahlin, �stersunds OK");
  supp.push_back("Sundsvalls OK:s Veteraner");
  supp.push_back("Kinnastr�ms SK");
  supp.push_back("OK Pan �rhus");
  supp.push_back("Jan Ernberg, T�by OK");
  supp.push_back("Stj�rnorps SK");
  supp.push_back("M�lndal Outdoor IF");
}
