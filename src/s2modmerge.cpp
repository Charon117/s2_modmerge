#include <algorithm>
#include <chrono>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

using std::cout;
using std::endl;

//global variables
static std::ofstream logfileStream("s2modmerge_logfile.txt");

struct Settings {
  std::filesystem::path source;
  std::filesystem::path target;
  bool overwrites{false};
}settings;

/********************
 Templates start here
 ********************/

template <typename T>
T readFileIntoContainer(const std::filesystem::path path)
{
  std::ifstream pipeIn(path);
  T file;
  std::string line;
  while(std::getline(pipeIn, line)) { file.push_back(line); }

  return file;
}

template <typename T>
T IterateTillIdentifer
(
  T Iterator,
  std::string Identifier_s,
  bool Increment
)
{
  if(Increment) {
    for(; (*Iterator).find(Identifier_s) == std::string::npos; Iterator++) {}

    return Iterator;
  }
  else {
    for(; (*Iterator).find(Identifier_s) == std::string::npos; Iterator--) {}

    return Iterator;
  }
}

/**********************************
 Miscellaneous functions start here
 **********************************/


 /*****************
  General functions
  *****************/

template<class... Args>
void printAndLog(Args&&... args)
{
  (std::cout << ... << std::forward<Args>(args));
  (logfileStream << ... << std::forward<Args>(args));
}

template<class... Args>
auto printAndLogLn(Args&&... args)
{
  (std::cout << ... << std::forward<Args>(args)) << std::endl;
  (logfileStream << ... << std::forward<Args>(args)) << std::endl;
}

std::vector<std::string>::iterator GetLowerDelimeter_lsit(char BracketOpening_ch, char BracketClosing_ch, std::vector<std::string>::iterator DownwardsIterator_it, const std::vector<std::string>& lFile)
{
  for(int BracketCount_i = 0; DownwardsIterator_it != lFile.end(); DownwardsIterator_it++) {

    for(const char& c : *DownwardsIterator_it) {
      if(c == BracketOpening_ch) { BracketCount_i++; }
    }

    for(const char& c : *DownwardsIterator_it) {
      if(c == BracketClosing_ch) { BracketCount_i--; }
    }

    if(BracketCount_i <= 0) { break; }
  }

  return DownwardsIterator_it;
}
// Yes, const_iterator cant be converted to iterator, so you duplicate the logic
std::vector<std::string>::const_iterator GetLowerDelimeter_lsit(char BracketOpening_ch, char BracketClosing_ch, std::vector<std::string>::const_iterator DownwardsIterator_it, const std::vector<std::string>& lFile)
{
  for(int BracketCount_i = 0; DownwardsIterator_it != lFile.end(); DownwardsIterator_it++) {

    for(const char& c : *DownwardsIterator_it) {
      if(c == BracketOpening_ch) { BracketCount_i++; }
    }

    for(const char& c : *DownwardsIterator_it) {
      if(c == BracketClosing_ch) { BracketCount_i--; }
    }

    if(BracketCount_i <= 0) { break; }
  }

  return DownwardsIterator_it;
}

/**********************************
 Partial Merge Functions start here
 **********************************/

std::vector<std::string>::const_iterator PartialMergeIdentifierInTheMiddle_lsit
(
  std::vector<std::string>::const_iterator UppDelimIteratorM,
  std::string UppDelimString,
  std::string IdentifierString,
  std::string LowDelimString,
  std::vector<std::string>& InstallationFileList
)
{
  auto IdentifierIteratorM = UppDelimIteratorM;
  IdentifierIteratorM++;
  IdentifierIteratorM = IterateTillIdentifer(IdentifierIteratorM, IdentifierString, true);

  auto IdentifierIteratorIR = std::find_if(InstallationFileList.rbegin(), InstallationFileList.rend(),
                                           [&IdentifierIteratorM](const std::string& str) {return str.find(*IdentifierIteratorM) != std::string::npos;});
  if(IdentifierIteratorIR != InstallationFileList.rend()) {

    auto UppDelimAnchorI = std::prev(IdentifierIteratorIR.base());
    UppDelimAnchorI--;
    UppDelimAnchorI = IterateTillIdentifer(UppDelimAnchorI, UppDelimString, false);

    for(UppDelimIteratorM++; (*UppDelimIteratorM).find(LowDelimString) == std::string::npos; UppDelimIteratorM++) {

      if(size_t DelimeterPos = (*UppDelimIteratorM).find("="); DelimeterPos != std::string::npos) {

        size_t FirstAlphanumericPos = (*UppDelimIteratorM).find_first_not_of(" \t\r\v\n\f");
        std::string SubdataIdentifier = (*UppDelimIteratorM).substr(FirstAlphanumericPos, DelimeterPos - FirstAlphanumericPos + 1);

        auto UppDelimIteratorI = UppDelimAnchorI;
        for(UppDelimIteratorI++; (*UppDelimIteratorI).find(LowDelimString) == std::string::npos; UppDelimIteratorI++) {

          if((*UppDelimIteratorI).find(SubdataIdentifier) != std::string::npos) {

            *UppDelimIteratorI = *UppDelimIteratorM;
            break;
          }
        }
      }
    }

    return UppDelimIteratorM;
  }
  else {

    UppDelimIteratorM = IterateTillIdentifer(UppDelimIteratorM, LowDelimString, true);
    return UppDelimIteratorM;
  }
}

/****************************************************
 Merge Functions depending on data format starts here
 ****************************************************/

 // This function is supposed to be executed inside a for() > if() loop. The if() inside the parent function is there in order to have control over when the function is executed.
std::vector<std::string>::const_iterator MergeSegmentIdentifierInLowerDelimeter_lsit
(
  std::vector<std::string>::const_iterator UppDelim_mit2,
  std::string UppDelim_s,
  std::string LowDelim_s,
  std::string IdentifierDelimeter_s,
  std::vector<std::string>& InstallationFile_ils2
)
{
  auto LowDelim_mit = UppDelim_mit2;
  for(LowDelim_mit++; (*LowDelim_mit).find(LowDelim_s); LowDelim_mit++) {}

  std::string Identifier_s = (*LowDelim_mit).substr(0, (*LowDelim_mit).find(IdentifierDelimeter_s) + 1);

  auto LowDelim_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&Identifier_s](const std::string& str) {return str.find(Identifier_s) != std::string::npos;});
  if(LowDelim_riit != InstallationFile_ils2.rend()) {

    auto LowDelim_iit = std::prev(LowDelim_riit.base());

    auto UppDelim_iit = LowDelim_iit;
    for(UppDelim_iit--; (*UppDelim_iit).find(UppDelim_s) == std::string::npos; UppDelim_iit--) {}

    UppDelim_iit = InstallationFile_ils2.erase(UppDelim_iit, std::next(LowDelim_iit));
    InstallationFile_ils2.insert(UppDelim_iit, UppDelim_mit2, std::next(LowDelim_mit));
  }
  else {
    auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&LowDelim_s](const std::string& str) {return str.find(LowDelim_s) != std::string::npos;});
    auto Insert_iit = Insert_riit.base();

    Insert_iit = InstallationFile_ils2.insert(Insert_iit, "");
    InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
  }

  return UppDelim_mit2 = LowDelim_mit;
}

std::vector<std::string>::const_iterator MergeSegmentIdentifierInUpperDelimeter_lsit
(
  std::vector<std::string>::const_iterator UppDelim_mit2,
  std::string UppDelim_s,
  std::string LowDelim_s,
  std::string IdentifierDelimeter_s,
  std::vector<std::string>& InstallationFile_ils2
)
{
  std::string Identifier_s = (*UppDelim_mit2).substr(0, (*UppDelim_mit2).find(IdentifierDelimeter_s) + 1); // this assumes that IdentifierDelimeter_s is only 1 char long, but "+ 1" can easily be replaced with "+ string.lenght()"

  auto LowDelim_mit = UppDelim_mit2;
  for(LowDelim_mit++; (*LowDelim_mit).find(LowDelim_s) == std::string::npos; LowDelim_mit++) {}

  auto UppDelim_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&Identifier_s](const std::string& str) {return str.find(Identifier_s) != std::string::npos;});
  if(UppDelim_riit != InstallationFile_ils2.rend()) {

    auto UppDelim_iit = std::prev(UppDelim_riit.base());

    auto LowDelim_iit = UppDelim_iit;
    for(LowDelim_iit++; (*LowDelim_iit).find(LowDelim_s) == std::string::npos; LowDelim_iit++) {}

    UppDelim_iit = InstallationFile_ils2.erase(UppDelim_iit, std::next(LowDelim_iit));
    InstallationFile_ils2.insert(UppDelim_iit, UppDelim_mit2, std::next(LowDelim_mit));
  }
  else {
    auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&UppDelim_s](const std::string& str) {return str.find(UppDelim_s) != std::string::npos;});
    auto Insert_iit = Insert_riit.base();
    for(; (*Insert_iit).find(LowDelim_s) == std::string::npos; Insert_iit++) {}

    InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
  }

  return UppDelim_mit2 = LowDelim_mit;
}

std::vector<std::string>::const_iterator MergeSegmentIdentifierInTheMiddle_lsit
(
  std::vector<std::string>::const_iterator UppDelim_mit2,
  std::string UppDelim_s,
  std::string Identifier_s,
  std::string LowDelim_s,
  std::vector<std::string>& InstallationFile_ils2
)
{
  auto Identifier_mit = UppDelim_mit2;
  for(Identifier_mit++; (*Identifier_mit).find(Identifier_s) == std::string::npos; Identifier_mit++) {}

  auto LowDelim_mit = Identifier_mit;
  for(LowDelim_mit++; (*LowDelim_mit).find(LowDelim_s) == std::string::npos; LowDelim_mit++) {}

  auto Identifier_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
  if(Identifier_riit != InstallationFile_ils2.rend()) {

    auto Identifier_iit = std::prev(Identifier_riit.base());

    auto UppDelim_iit = Identifier_iit;
    for(UppDelim_iit--; (*UppDelim_iit).find(UppDelim_s) == std::string::npos; UppDelim_iit--) {}

    auto LowDelim_iit = Identifier_iit;
    for(LowDelim_iit++; (*LowDelim_iit).find(LowDelim_s) == std::string::npos; LowDelim_iit++) {}

    UppDelim_iit = InstallationFile_ils2.erase(UppDelim_iit, std::next(LowDelim_iit));
    InstallationFile_ils2.insert(UppDelim_iit, UppDelim_mit2, std::next(LowDelim_mit));
  }
  else {
    auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&LowDelim_s](const std::string& str) {return str.find(LowDelim_s) != std::string::npos;});
    auto Insert_iit = Insert_riit.base();

    Insert_iit = InstallationFile_ils2.insert(Insert_iit, "");
    InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
  }

  return UppDelim_mit2 = LowDelim_mit;
}

std::vector<std::string>::const_iterator MergeSegmentCountingBrackets_lsit
(
  std::vector<std::string>::const_iterator UppDelim_mit2,
  std::string UppDelim_s,
  std::string Identifier_s,
  const std::vector<std::string>& MergeFile_mls2,
  std::vector<std::string>& InstallationFile_ils2,
  char BracketOpening_ch,
  char BracketClosing_ch
)
{
  auto Identifier_mit = UppDelim_mit2;
  for(Identifier_mit++; (*Identifier_mit).find(Identifier_s) == std::string::npos; Identifier_mit++) {}

  auto LowDelim_mit = UppDelim_mit2;
  for(int BracketCount_i = 0; LowDelim_mit != MergeFile_mls2.end(); LowDelim_mit++) {

    for(const char& c : *LowDelim_mit) {
      if(c == BracketOpening_ch) { BracketCount_i++; }
    }

    for(const char& c : *LowDelim_mit) {
      if(c == BracketClosing_ch) { BracketCount_i--; }
    }

    if(BracketCount_i <= 0) { break; }
  }

  auto Identifier_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
  if(Identifier_riit != InstallationFile_ils2.rend()) {

    auto Identifier_iit = std::prev(Identifier_riit.base());

    auto UppDelim_iit = Identifier_iit;
    for(UppDelim_iit--; (*UppDelim_iit).find(UppDelim_s) == std::string::npos; UppDelim_iit--) {}

    auto LowDelim_iit = UppDelim_iit;
    for(int BracketCount_i = 0; LowDelim_iit != InstallationFile_ils2.end(); LowDelim_iit++) {

      for(char& c : *LowDelim_iit) {
        if(c == BracketOpening_ch) { BracketCount_i++; }
      }

      for(char& c : *LowDelim_iit) {
        if(c == BracketClosing_ch) { BracketCount_i--; }
      }

      if(BracketCount_i <= 0) { break; }
    }

    UppDelim_iit = InstallationFile_ils2.erase(UppDelim_iit, std::next(LowDelim_iit));
    InstallationFile_ils2.insert(UppDelim_iit, UppDelim_mit2, std::next(LowDelim_mit));
  }
  else {
    auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&UppDelim_s](const std::string& str) {return str.find(UppDelim_s) != std::string::npos;});
    auto Insert_iit = std::prev(Insert_riit.base());
    Insert_iit = GetLowerDelimeter_lsit('{', '}', Insert_iit, InstallationFile_ils2);

    Insert_iit = InstallationFile_ils2.insert(std::next(Insert_iit), "");
    InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
  }

  return UppDelim_mit2 = LowDelim_mit;
}
// again, duplicate because of legacy code
std::vector<std::string>::iterator MergeSegmentCountingBrackets_lsit
(
  std::vector<std::string>::iterator UppDelim_mit2,
  std::string UppDelim_s,
  std::string Identifier_s,
  const std::vector<std::string>& MergeFile_mls2,
  std::vector<std::string>& InstallationFile_ils2,
  char BracketOpening_ch,
  char BracketClosing_ch
)
{
  auto Identifier_mit = UppDelim_mit2;
  for(Identifier_mit++; (*Identifier_mit).find(Identifier_s) == std::string::npos; Identifier_mit++) {}

  auto LowDelim_mit = UppDelim_mit2;
  for(int BracketCount_i = 0; LowDelim_mit != MergeFile_mls2.end(); LowDelim_mit++) {

    for(const char& c : *LowDelim_mit) {
      if(c == BracketOpening_ch) { BracketCount_i++; }
    }

    for(const char& c : *LowDelim_mit) {
      if(c == BracketClosing_ch) { BracketCount_i--; }
    }

    if(BracketCount_i <= 0) { break; }
  }

  auto Identifier_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
  if(Identifier_riit != InstallationFile_ils2.rend()) {

    auto Identifier_iit = std::prev(Identifier_riit.base());

    auto UppDelim_iit = Identifier_iit;
    for(UppDelim_iit--; (*UppDelim_iit).find(UppDelim_s) == std::string::npos; UppDelim_iit--) {}

    auto LowDelim_iit = UppDelim_iit;
    for(int BracketCount_i = 0; LowDelim_iit != InstallationFile_ils2.end(); LowDelim_iit++) {

      for(char& c : *LowDelim_iit) {
        if(c == BracketOpening_ch) { BracketCount_i++; }
      }

      for(char& c : *LowDelim_iit) {
        if(c == BracketClosing_ch) { BracketCount_i--; }
      }

      if(BracketCount_i <= 0) { break; }
    }

    UppDelim_iit = InstallationFile_ils2.erase(UppDelim_iit, std::next(LowDelim_iit));
    InstallationFile_ils2.insert(UppDelim_iit, UppDelim_mit2, std::next(LowDelim_mit));
  }
  else {
    auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&UppDelim_s](const std::string& str) {return str.find(UppDelim_s) != std::string::npos;});
    auto Insert_iit = std::prev(Insert_riit.base());
    Insert_iit = GetLowerDelimeter_lsit('{', '}', Insert_iit, InstallationFile_ils2);

    Insert_iit = InstallationFile_ils2.insert(std::next(Insert_iit), "");
    InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
  }

  return UppDelim_mit2 = LowDelim_mit;
}

std::vector<std::string>::iterator MergeSegmentSetIdentifier_lsit
(
  std::vector<std::string>::iterator UppDelim_mit2,
  std::string IdentifierDelimeter_s,
  std::vector<std::string>& MergeFile_mls2,
  std::vector<std::string>& InstallationFile_ils2,
  char BracketOpening_ch,
  char BracketClosing_ch
)
{
  std::string GroupIdentifier_s = *UppDelim_mit2;
  std::string Identifier_s = *UppDelim_mit2;
  GroupIdentifier_s = GroupIdentifier_s.substr(0, GroupIdentifier_s.find("(") + 1);
  Identifier_s = Identifier_s.substr(0, Identifier_s.find(IdentifierDelimeter_s) + 1);

  auto SaveIteratorPosition_mit = std::prev(UppDelim_mit2);

  for(auto Delete_iit = InstallationFile_ils2.begin(); Delete_iit != InstallationFile_ils2.end(); Delete_iit++) {

    if((*Delete_iit).find(Identifier_s) != std::string::npos) {

      auto LowDelimDelete_iit = Delete_iit;
      LowDelimDelete_iit = GetLowerDelimeter_lsit(BracketOpening_ch, BracketClosing_ch, LowDelimDelete_iit, InstallationFile_ils2);

      Delete_iit = InstallationFile_ils2.erase(Delete_iit, next(LowDelimDelete_iit));
      Delete_iit--;
    }
  }

  /*******************************************************************************************************************************************
   Insertion point gets looked up once for every set, and then the last line of the last insertion marks the new insertion point.
   This means N file wide searches have to be done, where N is the number of sets.
   A possible efficiency save can be made here to look up the entry point once per file, and then move it everytime a new entry gets inserted.
   Have to check if insertion point stays TableIdentifier_s.
   *******************************************************************************************************************************************/
  auto Insert_riit = std::find_if(InstallationFile_ils2.rbegin(), InstallationFile_ils2.rend(), [&GroupIdentifier_s](const std::string& str) {return str.find(GroupIdentifier_s) != std::string::npos;});
  auto Insert_iit = prev(Insert_riit.base());
  Insert_iit = GetLowerDelimeter_lsit(BracketOpening_ch, BracketClosing_ch, Insert_iit, InstallationFile_ils2);

  for(; UppDelim_mit2 != MergeFile_mls2.end(); UppDelim_mit2++) {

    if((*UppDelim_mit2).find(Identifier_s) != std::string::npos) {

      auto LowDelim_mit = UppDelim_mit2;
      LowDelim_mit = GetLowerDelimeter_lsit(BracketOpening_ch, BracketClosing_ch, LowDelim_mit, MergeFile_mls2);

      Insert_iit = InstallationFile_ils2.insert(std::next(Insert_iit), UppDelim_mit2, std::next(LowDelim_mit));
      Insert_iit = GetLowerDelimeter_lsit(BracketOpening_ch, BracketClosing_ch, Insert_iit, InstallationFile_ils2); //Good thought on saving the insertion point instead of running a file wide search for the entry point again.

      UppDelim_mit2 = MergeFile_mls2.erase(UppDelim_mit2, std::next(LowDelim_mit));
      UppDelim_mit2--;
    }
  }

  return SaveIteratorPosition_mit;
}

int MergeSegmentUniqueIdentifierLine(const std::vector<std::string>& Keywords_v2, const std::vector<std::string>& MergeFile_mls2, std::vector<std::string>& InstallationFile_ils2)
{
  for(std::string VectorKeyword_s : Keywords_v2) {

    auto Line_mit = std::find_if(MergeFile_mls2.begin(), MergeFile_mls2.end(), [&VectorKeyword_s](const std::string& str) {return str.find(VectorKeyword_s) != std::string::npos;});
    if(Line_mit != MergeFile_mls2.end()) {

      auto Line_iit = std::find_if(InstallationFile_ils2.begin(), InstallationFile_ils2.end(), [&VectorKeyword_s](const std::string& str) {return str.find(VectorKeyword_s) != std::string::npos;});
      if(Line_iit != InstallationFile_ils2.end()) { *Line_iit = *Line_mit; }
    }
  }

  return 0;
}

/************************
 Merge Functions end here
 ************************/

 /*************************
  File Functions start here
  *************************/

auto mergeOptionsDefault_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  const std::vector<std::string> Keywords_v = {"lobby_ip =", "lobby_port =", "binding_zoomin                        =", "binding_zoomout                       =",
                                        "binding_mountslot                     =", "binding_heal			              =", "binding_minimap0                      =",
                                       "binding_inventory                     =", "binding_combatarts                    =", "binding_attributes                    =",
                                       "binding_teamdialog                    =", "binding_logbook	                      =", "binding_autocollect                   =",
                                       "binding_pause                         =", "binding_home_teleport                 =", "binding_call_specialpet               =",
                                       "binding_screenshot                    =", "binding_savegame                      =", "binding_godspell					  =",
                                       "binding_togglecamera				  =", "binding_viewleft                      =", "binding_viewright                     =",
                                       "binding_walkfwd                       =", "binding_walkbwd                       =", "binding_camera_looknorth              =",
                                       "binding_weapon_1                      =", "binding_weapon_2                      =", "binding_weapon_3                      =",
                                       "binding_weapon_4                      =", "binding_weapon_5                      =", "binding_spell_1                       =",
                                       "binding_spell_2                       =", "binding_spell_3                       =", "binding_spell_4	                      =",
                                       "binding_spell_5	                      =", "binding_potion_1                      =", "binding_potion_2                      =",
                                       "binding_potion_3                      =", "binding_potion_4                      =", "binding_potion_5                      =",
                                       "binding_potion_6                      =", "binding_buff_1                        =", "binding_buff_2                        =",
                                       "binding_buff_3                        =", "binding_orb1                          =", "binding_orb2                          =",
                                       "binding_orb3                          =", "binding_orb4                          =", "binding_toggle_minion_attackrange     =",
                                       "binding_toggle_minion_attackmode      =", "binding_minion_status                 =", "binding_worldmap                      =",
                                       "binding_worldmap_zoomout              =", "binding_worldmap_zoomin               ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);
}

auto mergeHeightmap_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("scaling") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "scaling", "mgr.defineScaling(", ",", targetFile);
    }
  }
}

auto mergeAutoexec_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  if(std::find_if(sourceFile.begin(), sourceFile.end(), [](const std::string& str) {return str.find("--//DisableSplash", 0) == 0;}) != sourceFile.end()) {

    for(bool ContinueDeleting = true; ContinueDeleting;) {

      auto DeleteSplash_it = std::find_if(targetFile.begin(), targetFile.end(), [](const std::string& str) {return str.find("for index,trailer in sys.findFiles(") != std::string::npos;});
      if(DeleteSplash_it != targetFile.end()) {

        auto DeleteSplashEnd_it = DeleteSplash_it;
        for(; (*DeleteSplashEnd_it).find("end") == std::string::npos; DeleteSplashEnd_it++) {}

        targetFile.erase(DeleteSplash_it, std::next(DeleteSplashEnd_it));
      }
      else {
        ContinueDeleting = false;
      }
    }
  }
}

auto mergeBehaviour_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newBehaviour =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newBehaviour =", "name         =", "mgr.createBehaviour(", targetFile);
    }
  }
}

auto mergeTypification_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newTypification =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newTypification =", "mgr.createTypification(", ",", targetFile);
    }
  }
}

auto mergeSpells_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.defineSpell(") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInUpperDelimeter_lsit(UppDelim_mit, "mgr.defineSpell(", ")", ",", targetFile);
      continue;
    }

    if((*UppDelim_mit).find("mgr.addTokenBonus(") != std::string::npos) {

      std::string Identifier_s = (*UppDelim_mit).substr(0, (*UppDelim_mit).find(",") + 1);

      auto Line_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_s](const std::string& str) {return str.find(Identifier_s) != std::string::npos;});
      *Line_riit = *UppDelim_mit;
    }
  }
}

auto mergeMaterial_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newMaterial =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newMaterial =", "mgr.createMaterial(", ",", targetFile);
    }
  }
}

auto mergeItemtype_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"FLAG_NOALTINFO =", "FLAG_NOSPAWN =", "FLAG_EQUIPCUT =", "FLAG_HASPHYSICS =", "FLAG_DISABLEHAIR =", "FLAG_HASHAIR =", "FLAG_HASBLINKANIM =",
                                       "FLAG_CASTNOSHADOW =", "FLAG_HASIDLEANIM =", "FLAG_HASSOUND =", "FLAG_OBSTRUCTSOUND =", "FLAG_WALKTHROUGH =", "FLAG_ISENTERABLE =",
                                       "FLAG_EDITORONLY =", "FLAG_QUADTREEOFF =", "FLAG_PERPENDICULAR1 =", "FLAG_PERPENDICULAR2 =", "FLAG_NOGO =", "FLAG_CAMERASHAKE =", "FLAG_DAMPSOUND =",
                                       "FLAG_BLACKBACK =", "FLAG_FLYING =", "FLAG_EDITOR_EXCLUDE  =", "FLAG_HASPREVIEWIMAGE =", "FLAG_HAS_WORLDOBJECT_RS =", "FLAG_CLICKTHROUGH =",
                                       "FLAG_EXCLUDEFROMNAVIGEN =", "FLAG_NO_ANIM_OPTIMIZATION ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newItemType =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newItemType =", "mgr.typeCreate(", ",", targetFile);
    }
  }
}

auto mergeIteminfo_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newItemInfo =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newItemInfo =", "type           =", "mgr.itemInfoCreate(", targetFile);
    }
  }
}

auto mergeDefines_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"WEATHERFLAG_NIGHT", "WEATHERFLAG_LIGHTNING", "WEATHERFLAG_RAIN", "WEATHERFLAG_WIND", "WEATHERFLAG_EARTHQUAKE", "WEATHERFLAG_FOG",
                                       "WEATHERFLAG_GROUNDFOG", "WEATHERFLAG_HIGHNOON", "MOVIE_INTRO", "MOVIE_OUTRO_LIGHT", "MOVIE_OUTRO_SHADOW", "MOVIE_CONCERT"};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);
}

auto mergeCreatureinfo_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"SMT_DEFAULT =", "SMT_HERO ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newCreatureInfo =") != std::string::npos) {

      if((*UppDelim_mit).find("MODMERGE = \"partial replace\"") != std::string::npos) {

        PartialMergeIdentifierInTheMiddle_lsit(UppDelim_mit, "newCreatureInfo = {", "type         =", "mgr.creatureInfoCreate(", targetFile);
        continue;
      }

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newCreatureInfo =", "type         =", "mgr.creatureInfoCreate(", targetFile);
    }
  }
}

auto mergeBooks_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"local BOOK_FOOD      =", "local BOOK_LOSTDIARY =", "local BOOK_MAGIC     =", "local BOOK_ORCISH    =", "local BOOK_RELIGIONS =", "local BOOK_WEAPONS   =",
                                       "local BOOK_ANATOMY   =", "local BOOK_MONSTERS  =", "local BOOK_HERBS     =", "local BOOK_TRAVEL    =", "local BOOK_GUIDE     =", "local BOOK_HELP      =",
                                       "local BOOK_ADDON     ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.defineBook(") != std::string::npos) {

      std::string Identifier_s = (*UppDelim_mit).substr((*UppDelim_mit).find("BOOK"), (*UppDelim_mit).rfind(",") - (*UppDelim_mit).find("BOOK") + 1);

      auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_s](const std::string& str) {return str.find(Identifier_s) != std::string::npos;});
      if(UppDelim_riit != targetFile.rend()) {

        *UppDelim_riit = *UppDelim_mit;
      }
      else {

        std::string Identifier_Type_s = Identifier_s.substr(0, Identifier_s.find(",") + 1);

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_Type_s](const std::string& str) {return str.find(Identifier_Type_s) != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        targetFile.insert(Insert_iit, *UppDelim_mit);
      }
    }
  }
}

auto mergeWeaponpool_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addWeaponPool {") != std::string::npos) {

      MergeSegmentCountingBrackets_lsit(UppDelim_mit, "mgr.addWeaponPool {", "dbid =", sourceFile, targetFile, '{', '}');
    }
  }
}

auto mergeWaypoints(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.createWay(") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInUpperDelimeter_lsit(UppDelim_mit, "mgr.createWay(", ")", ",", targetFile);
    }
  }
}

auto mergeTriggervolumes_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v{"local TVTYPE_BOX       =", "local TVTYPE_SPHERE    ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addTriggerVolume(") != std::string::npos) {

      MergeSegmentCountingBrackets_lsit(UppDelim_mit, "mgr.addTriggerVolume(", "name =", sourceFile, targetFile, '(', ')');
    }
  }
}

auto mergeTriggerarea_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"local TVTYPE_BOX", "local TVTYPE_SPHERE", "local TVTYPE_POINT"};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addTriggerArea(") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "mgr.addTriggerArea(", "name=", ")", targetFile);
    }
  }
}

auto mergeSpawn_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addSpawn (") != std::string::npos) {

      std::string sPosSectorIdentifier = (*UppDelim_mit).substr(0, (*UppDelim_mit).rfind(",") + 1);

      auto IdentifierLayermap_mit = UppDelim_mit;
      for(IdentifierLayermap_mit++; (*IdentifierLayermap_mit).find("layermap_id =") == std::string::npos; IdentifierLayermap_mit++) {}

      auto LowDelim_mit = IdentifierLayermap_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find(")") == std::string::npos; LowDelim_mit++) {}

      auto UppDelim_riit = targetFile.rbegin();
      for(; UppDelim_riit != targetFile.rend(); UppDelim_riit++) {

        if((*UppDelim_riit).find(sPosSectorIdentifier) != std::string::npos) {

          auto UppDelim_iit = UppDelim_riit.base();
          for(; (*UppDelim_iit).find("layermap_id =") == std::string::npos; UppDelim_iit++) {}

          if((*UppDelim_iit).find(*IdentifierLayermap_mit) != std::string::npos) {

            auto LowDelim_iit = UppDelim_iit;
            for(; (*LowDelim_iit).find(")") == std::string::npos; LowDelim_iit++) {}

            for(UppDelim_iit--; (*UppDelim_iit).find("mgr.addSpawn (") == std::string::npos; UppDelim_iit--) {}

            UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
            targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));

            UppDelim_mit = LowDelim_mit;
            break;
          }
        }
      }

      if(UppDelim_riit == targetFile.rend()) {

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find(")") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        Insert_iit = targetFile.insert(Insert_iit, "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }
    }
  }
}

auto mergeRegion_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.regionSetup(") != std::string::npos) {

      auto LowDelim_mit = UppDelim_mit;
      LowDelim_mit = GetLowerDelimeter_lsit('(', ')', LowDelim_mit, sourceFile);

      auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&UppDelim_mit](const std::string& str) {return str.find(*UppDelim_mit) != std::string::npos;});
      if(UppDelim_riit != targetFile.rend()) {

        auto UppDelim_iit = std::prev(UppDelim_riit.base());

        auto LowDelim_iit = UppDelim_iit;
        LowDelim_iit = GetLowerDelimeter_lsit('(', ')', LowDelim_iit, targetFile);

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find(")") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        Insert_iit = targetFile.insert(Insert_iit, "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }
  }
}

auto mergeQuest_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++)
  {
    bool Check_b = false;

    if((*UppDelim_mit).find("quest.createTaskCreature(") != std::string::npos) { Check_b = true; }
    if((*UppDelim_mit).find("quest.createTaskItem(") != std::string::npos) { Check_b = true; }
    if((*UppDelim_mit).find("quest.createQuest(") != std::string::npos) { Check_b = true; }

    if(Check_b) {

      auto LowDelim_mit = UppDelim_mit;
      for(; (*LowDelim_mit).find(")") == std::string::npos; LowDelim_mit++);

      auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&UppDelim_mit](const std::string& str) {return str.find(*UppDelim_mit) != std::string::npos;});
      if(UppDelim_riit != targetFile.rend()) {

        auto UppDelim_iit = std::prev(UppDelim_riit.base());

        auto LowDelim_iit = UppDelim_iit;
        for(; (*LowDelim_iit).find(")") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find(")") != std::string::npos;}); // everything just gets appended after the last entry, due to the sensibility of the file
        auto Insert_iit = Insert_riit.base();

        Insert_iit = targetFile.insert(Insert_iit, "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }
  }
}

auto mergePortal_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"local PORTALTYPE_WORLD =", "local PORTALTYPE_SHIP_NORTH  =", "local PORTALTYPE_SHIP_SOUTH  ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto Line_mit = sourceFile.begin(); Line_mit != sourceFile.end(); Line_mit++) {

    if((*Line_mit).find("mgr.definePortal {") != std::string::npos) {

      std::string Type_s = (*Line_mit).substr((*Line_mit).find("type="), (*Line_mit).find(" ", (*Line_mit).find("type=")) - (*Line_mit).find("type="));
      std::string Identifier_s = (*Line_mit).substr((*Line_mit).find("id="), (*Line_mit).find(",") + 1 - (*Line_mit).find("id="));

      auto Line_riit = targetFile.rbegin();
      for(; Line_riit != targetFile.rend(); Line_riit++) {

        if((*Line_riit).find(Type_s) != std::string::npos) {
          if((*Line_riit).find(Identifier_s) != std::string::npos) {

            *Line_riit = *Line_mit;
            break;
          }
        }
      }

      if(Line_riit == targetFile.rend()) {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Type_s](const std::string& str) {return str.find(Type_s) != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        targetFile.insert(Insert_iit, *Line_mit);
      }
    }
  }
}

auto mergeFaction_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addFaction {") != std::string::npos) {

      auto Identifier_mit = UppDelim_mit;
      for(Identifier_mit++; (*Identifier_mit).find("id =") == std::string::npos; Identifier_mit++) {}

      auto LowDelim_mit = Identifier_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find("}") == std::string::npos; LowDelim_mit++) {}

      auto Identifier_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
      if(Identifier_riit != targetFile.rend()) {

        auto Identifier_iit = std::prev(Identifier_riit.base());

        auto UppDelim_iit = Identifier_iit;
        for(UppDelim_iit--; (*UppDelim_iit).find("mgr.addFaction {") == std::string::npos; UppDelim_iit--) {}

        auto LowDelim_iit = Identifier_iit;
        for(LowDelim_iit++; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addFaction {") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();
        for(; (*Insert_iit).find("}") == std::string::npos; Insert_iit++) {}

        Insert_iit = targetFile.insert(std::next(Insert_iit), "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }

    if((*UppDelim_mit).find("mgr.addFactionRelation {") != std::string::npos) {

      auto Identifier_mit1 = UppDelim_mit;
      for(Identifier_mit1++; (*Identifier_mit1).find("id1 =") == std::string::npos; Identifier_mit1++) {}

      auto Identifier_mit2 = Identifier_mit1;
      for(Identifier_mit2++; (*Identifier_mit2).find("id2 =") == std::string::npos; Identifier_mit2++) {}

      auto LowDelim_mit = Identifier_mit2;
      for(LowDelim_mit++; (*LowDelim_mit).find("}") == std::string::npos; LowDelim_mit++) {}


      bool NeedInsert_b = true;
      for(auto Identifier_riit1 = targetFile.rbegin(); Identifier_riit1 != targetFile.rend(); Identifier_riit1++) {

        if((*Identifier_mit1) == (*Identifier_riit1)) {

          auto Identifier_iit1 = Identifier_riit1.base();
          if(*Identifier_mit2 == *Identifier_iit1) {

            auto UppDelim_iit = Identifier_iit1;
            for(UppDelim_iit--; (*UppDelim_iit).find("mgr.addFactionRelation {") == std::string::npos; UppDelim_iit--) {}

            auto LowDelim_iit = Identifier_iit1;
            for(LowDelim_iit++; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

            UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
            targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));

            NeedInsert_b = false;
            break;
          }
        }
      }

      if(NeedInsert_b) {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addFactionRelation {") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();
        for(; (*Insert_iit).find("}") == std::string::npos; Insert_iit++) {}

        Insert_iit = targetFile.insert(std::next(Insert_iit), "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }
    }
  }
}

auto mergeEquipsets_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  bool Equipslot_b = true;
  auto UppDelim_mit = sourceFile.begin();
  auto GetComments_mit = UppDelim_mit;
  for(; UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if(Equipslot_b) {

      if((*UppDelim_mit).find("local EQUIPSLOT = {") != std::string::npos) {
        auto TableStart_iit = std::find_if(targetFile.begin(), targetFile.end(), [](const std::string& str) {return str.find("local EQUIPSLOT = {") != std::string::npos;});

        for(UppDelim_mit++; (*UppDelim_mit).find("}") == std::string::npos; UppDelim_mit++) {

          std::string Identifier_s = (*UppDelim_mit).substr(0, (*UppDelim_mit).find("=") + 1);

          auto TableLine_iit = TableStart_iit;
          for(; (*TableLine_iit).find("}") == std::string::npos; TableLine_iit++) {

            if((*TableLine_iit).find(Identifier_s) != std::string::npos) {

              *TableLine_iit = *UppDelim_mit;
            }
          }
        }

        GetComments_mit = UppDelim_mit;
        Equipslot_b = false;
      }
    }

    if((*UppDelim_mit).find("mgr.createEquipset {") != std::string::npos) {

      auto Identifier_mit = UppDelim_mit;
      for(Identifier_mit++; (*Identifier_mit).find("id =") == std::string::npos; Identifier_mit++) {}

      auto LowDelim_mit = UppDelim_mit;
      LowDelim_mit = GetLowerDelimeter_lsit('{', '}', LowDelim_mit, sourceFile);

      auto Identifier_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
      if(Identifier_riit != targetFile.rend()) {

        auto Identifier_iit = std::prev(Identifier_riit.base());

        auto UppDelim_iit = Identifier_iit;
        for(UppDelim_iit--; (*UppDelim_iit).find("mgr.createEquipset {") == std::string::npos; UppDelim_iit--) {}

        auto GetComments_iit = UppDelim_iit;
        for(GetComments_iit--; (*GetComments_iit).find("}") == std::string::npos; GetComments_iit--) {}

        auto LowDelim_iit = UppDelim_iit;
        LowDelim_iit = GetLowerDelimeter_lsit('{', '}', LowDelim_iit, targetFile);

        GetComments_iit = targetFile.erase(std::next(GetComments_iit), std::next(LowDelim_iit));
        targetFile.insert(GetComments_iit, std::next(GetComments_mit), std::next(LowDelim_mit));

        UppDelim_mit = LowDelim_mit;
        GetComments_mit = LowDelim_mit;
        continue;
      }
      else {
        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("}") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        targetFile.insert(Insert_iit, std::next(GetComments_mit), std::next(LowDelim_mit));

        UppDelim_mit = LowDelim_mit;
        GetComments_mit = LowDelim_mit;
        continue;
      }
    }
  }


  int EquipsetCount_i = 0;
  int EquipentryCount_i = 0;
  for(auto LCheck_b_iit = targetFile.begin(); LCheck_b_iit != targetFile.end(); LCheck_b_iit++) {

    if((*LCheck_b_iit).find("mgr.createEquipset {") != std::string::npos) { EquipsetCount_i++; }
    if((*LCheck_b_iit).find("EQUIPSLOT.") != std::string::npos) { EquipentryCount_i++; }
  }

  auto ReserveEquipsets_iit = std::find_if(targetFile.begin(), targetFile.end(), [](const std::string& str) {return str.find("mgr.reserveEquipsets(") != std::string::npos;});
  *ReserveEquipsets_iit = "mgr.reserveEquipsets(" + std::to_string(EquipsetCount_i) + "," + std::to_string(EquipentryCount_i) + ")";
}

auto mergeDrop_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.createDroplist(") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInUpperDelimeter_lsit(UppDelim_mit, "mgr.createDroplist(", ")", ",", targetFile);
      continue;
    }

    if((*UppDelim_mit).find("mgr.createDroppattern(") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInUpperDelimeter_lsit(UppDelim_mit, "mgr.createDroppattern(", ")", ",", targetFile);
      continue;
    }

    if((*UppDelim_mit).find("local shrinkheadDropMap =") != std::string::npos) {

      auto ShrinkheadStart_irit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("local shrinkheadDropMap =") != std::string::npos;});
      auto ShrinkheadStart_iit = std::prev(ShrinkheadStart_irit.base());
      for(; (*UppDelim_mit).find("mgr.loadShrinkheadDropMap(") == std::string::npos; UppDelim_mit++) {

        unsigned long long Mark_ull = (*UppDelim_mit).find("SUBFAM");
        if(Mark_ull != std::string::npos) {

          std::string SubfamIdentifier_s = (*UppDelim_mit).substr((*UppDelim_mit).find("\""), (*UppDelim_mit).rfind("\"") - (*UppDelim_mit).find("\"") + 1);
          std::string SubfamSwap_s = (*UppDelim_mit).substr((*UppDelim_mit).rfind("{"), (*UppDelim_mit).rfind("}") - (*UppDelim_mit).rfind("{") + 1); // This is necessary because of some misformat in the file, otherwise we could just write *itLBShrinkhead = *UppDelim_mit

          auto itLBShrinkhead = ShrinkheadStart_iit;
          for(itLBShrinkhead++; itLBShrinkhead != targetFile.end(); itLBShrinkhead++) {

            if((*itLBShrinkhead).find(SubfamIdentifier_s) != std::string::npos) {

              (*itLBShrinkhead).replace((*itLBShrinkhead).rfind("{"), (*itLBShrinkhead).rfind("}") - (*itLBShrinkhead).rfind("{") + 1, SubfamSwap_s);
              break;
            }

            if((*itLBShrinkhead).find("mgr.loadShrinkheadDropMap(") != std::string::npos) {

              targetFile.insert(prev(itLBShrinkhead), *UppDelim_mit);
              break;
            }
          }
        }
      }
    }
  }
}

auto mergeCreatures_txt(const std::vector<std::string>& sourceFileOriginal, std::vector<std::string>& targetFile) -> void
{
  // duplicate because of MergeSegmentSetIdentifier_lsit
  auto sourceFile{sourceFileOriginal};

  auto UppDelim_mrit = std::find_if(sourceFile.rbegin(), sourceFile.rend(), [](const std::string& str) {return str.find("mgr.addMapPos {") != std::string::npos;});
  if(UppDelim_mrit != sourceFile.rend()) {

    for(auto UppDelim_iit = targetFile.begin(); UppDelim_iit != targetFile.end(); UppDelim_iit++) {

      if((*UppDelim_iit).find("mgr.addMapPos {") != std::string::npos) {

        auto LowDelim_iit = UppDelim_iit;
        LowDelim_iit = GetLowerDelimeter_lsit('{', '}', LowDelim_iit, targetFile);

        UppDelim_iit = targetFile.erase(UppDelim_iit, next(LowDelim_iit));
        UppDelim_iit--; //there is not continue; without incremention, so use while() where you can control the incremention if such is necessary
      }
    }
  }

  auto InsertMap_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addMapPos =") != std::string::npos;});
  auto InsertMap_iit = InsertMap_riit.base();
  InsertMap_iit++; InsertMap_iit++;

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.createCreature {") != std::string::npos) {

      UppDelim_mit = MergeSegmentCountingBrackets_lsit(UppDelim_mit, "mgr.createCreature {", "id =", sourceFile, targetFile, '{', '}');
      continue;
    }

    if((*UppDelim_mit).find("mgr.addCreatureBonus(") != std::string::npos) {

      UppDelim_mit = MergeSegmentSetIdentifier_lsit(UppDelim_mit, ",", sourceFile, targetFile, '(', ')');
      continue;
    }

    if((*UppDelim_mit).find("mgr.createSkill {") != std::string::npos) {

      auto GetComments_mit = UppDelim_mit;
      for(GetComments_mit--; bool(((*GetComments_mit).find("}") == std::string::npos) && (*GetComments_mit != "end")); GetComments_mit--) {}
      GetComments_mit++;

      auto Identifier_mit = UppDelim_mit;
      for(Identifier_mit++; (*Identifier_mit).find("skill_name =") == std::string::npos; Identifier_mit++) {}

      auto LowDelim_mit = Identifier_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find("}") == std::string::npos; LowDelim_mit++) {}

      auto Identifier_iit = targetFile.end();
      for(auto UppDelim_iit = targetFile.begin(); UppDelim_iit != targetFile.end(); UppDelim_iit++) {

        if((*UppDelim_iit).find("mgr.createSkill {") != std::string::npos) {

          for(; (*UppDelim_iit).find("}") == std::string::npos; UppDelim_iit++) {
            if(*UppDelim_iit == *Identifier_mit) { Identifier_iit = UppDelim_iit; }
          }
        }
      }
      if(Identifier_iit != targetFile.end()) {

        auto GetComments_iit = std::prev(Identifier_iit, 2);
        for(; ((*GetComments_iit).find("}") == std::string::npos) && (*GetComments_iit != "end"); GetComments_iit--) {}
        GetComments_iit++;

        auto LowDelim_iit = Identifier_iit;
        for(LowDelim_iit++; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

        GetComments_iit = targetFile.erase(GetComments_iit, next(LowDelim_iit));
        targetFile.insert(GetComments_iit, GetComments_mit, next(LowDelim_mit));
      }
      else {
        auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.createSkill {") != std::string::npos;});
        auto LowDelim_iit = UppDelim_riit.base();
        for(; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

        targetFile.insert(next(LowDelim_iit), GetComments_mit, next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }


    if((*UppDelim_mit).find("mgr.addCreatureSkill(") != std::string::npos) {

      UppDelim_mit = MergeSegmentSetIdentifier_lsit(UppDelim_mit, ",", sourceFile, targetFile, '(', ')');
      continue;
    }


    if((*UppDelim_mit).find("mgr.addCreatureBpRelation {") != std::string::npos) {

      auto Identifier_mit = UppDelim_mit;
      for(; (*Identifier_mit).find("creature_id =") == std::string::npos; Identifier_mit++) {}
      std::string Identifier_s = (*Identifier_mit).substr((*Identifier_mit).find("creature_id ="), (*Identifier_mit).find(",") - (*Identifier_mit).find("creature_id =") + 1);

      auto LowDelim_mit = Identifier_mit;
      for(; (*LowDelim_mit).find("}") == std::string::npos; LowDelim_mit++) {}


      auto UppDelim_iit = targetFile.begin();
      bool FoundIdentifier_b = false;
      for(; UppDelim_iit != targetFile.end(); UppDelim_iit++) {

        if((*UppDelim_iit).find("mgr.addCreatureBpRelation {") != std::string::npos) {

          for(; UppDelim_iit != targetFile.end(); UppDelim_iit++) {

            if((*UppDelim_iit).find(Identifier_s) != std::string::npos) { FoundIdentifier_b = true; break; }
            if((*UppDelim_iit).find("}") != std::string::npos) { break; }
          }

          if(FoundIdentifier_b) { break; }
        }
      }
      if(UppDelim_iit != targetFile.end()) {

        for(; (*UppDelim_iit).find("mgr.addCreatureBpRelation {") == std::string::npos; UppDelim_iit--) {}

        auto LowDelim_iit = UppDelim_iit;
        for(; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, next(LowDelim_mit));
      }
      else {
        auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addCreatureBpRelation {") != std::string::npos;});
        UppDelim_iit = std::prev(UppDelim_riit.base());
        for(; (*UppDelim_iit).find("}") == std::string::npos; UppDelim_iit++) {}

        UppDelim_iit = targetFile.insert(std::next(UppDelim_iit), "");
        targetFile.insert(next(UppDelim_iit), UppDelim_mit, next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }


    if((*UppDelim_mit).find("mgr.addMapPos {") != std::string::npos) {

      auto LowDelim_mit = UppDelim_mit;
      LowDelim_mit = GetLowerDelimeter_lsit('{', '}', LowDelim_mit, sourceFile);

      targetFile.insert(InsertMap_iit, UppDelim_mit, next(LowDelim_mit));

      UppDelim_mit = LowDelim_mit;
      continue;
    }
  }
}

auto mergeBlueprint_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newBlueprint =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newBlueprint =", "mgr.createBlueprint(", ",", targetFile);
    }
    if((*UppDelim_mit).find("newBonus =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newBonus =", "mgr.createBonus(", ",", targetFile);
    }
    if((*UppDelim_mit).find("newBonusgroup =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newBonusgroup =", "mgr.createBonusgroup(", ",", targetFile);
    }
    if((*UppDelim_mit).find("newBlueprintSet =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newBlueprintSet =", "mgr.createBlueprintset(", ",", targetFile);
    }
  }
}

auto mergeSurface_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"SURFACE_FLAG_MASKED =", "SURFACE_FLAG_TRANSPARENT =", "SURFACE_FLAG_OPAQUE =", "SURFACE_FLAG_SFX =", "SURFACE_FLAG_TENERGY =",
                                        "SURFACE_FLAG_PULSATING_GLOW =", "SURFACE_FLAG_CONSTANT_FUR =", "SURFACE_FLAG_VARIABLE_FUR =", "SURFACE_FLAG_BLACKBACK =",
                                        "SURFACE_FLAG_DOUBLESIDED =", "SURFACE_FLAG_PARTICLEEMITTER =", "SURFACE_FLAG_CLICKTHROUGH =", "SURFACE_FLAG_MINIMESHABLE =",
                                        "SURFACE_FLAG_ZMASKED =", "SURFACE_FLAG_SHELLFX =", "SURFACE_FLAG_CLOTHFX ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("diffusePnt    =") != std::string::npos) {

      for(UppDelim_mit--; (*UppDelim_mit).find("{") == std::string::npos; UppDelim_mit--) {}

      auto LowDelim_mit = UppDelim_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find("}") == std::string::npos; LowDelim_mit++);

      std::string Identifier_s = (*UppDelim_mit).substr(0, (*UppDelim_mit).find("=") + 1);
      auto UppDelim_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_s](const std::string& str) {return str.find(Identifier_s) != std::string::npos;});
      if(UppDelim_riit != targetFile.rend()) {

        auto UppDelim_iit = std::prev(UppDelim_riit.base());

        auto LowDelim_iit = UppDelim_iit;
        for(LowDelim_iit++; (*LowDelim_iit).find("}") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("ambDiff       =") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();
        for(; (*Insert_iit).find("}") == std::string::npos; Insert_iit++) {}

        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }

    if((*UppDelim_mit).find("newSurface =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newSurface =", "name         =", "mgr.surfCreate(", targetFile);
    }
  }
}

auto mergeSoundresources_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newResource =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newResource =", "mgr.soundResourceCreate(", ")", targetFile);
    }
  }
}

auto mergeSoundProfile_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newProfile =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newProfile =", "profilename  =", "mgr.soundProfileCreate(", targetFile);
    }
  }
}

auto mergePoidata_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto Line_mit = sourceFile.begin(); Line_mit != sourceFile.end(); Line_mit++) {

    if((*Line_mit).find("mgr.addPoIData(") != std::string::npos) {

      std::string Keyword_ms = (*Line_mit).substr((*Line_mit).find("id="), (*Line_mit).find(",") - (*Line_mit).find("id="));

      auto Line_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Keyword_ms](const std::string& str) {return ((str.find(Keyword_ms) != std::string::npos) && (str.find("mgr.addPoIData(") != std::string::npos));});
      if(Line_riit != targetFile.rend()) {

        *Line_riit = *Line_mit;
      }
      else {
        auto Insert_iit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addPoIData(") != std::string::npos;});
        targetFile.insert(Insert_iit.base(), Line_mit, std::next(Line_mit));
      }
    }
  }
}

auto mergePatches_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"PATCH_FLAG_TENERGY ="};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newPatch =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInTheMiddle_lsit(UppDelim_mit, "newPatch =", "type         =", "mgr.patchCreate(", targetFile);
    }
  }
}

auto mergeMinitype_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  std::vector<std::string> Keywords_v = {"TYPE_GRASS =", "TYPE_FLOWER =", "TYPE_MUSHROOM =", "TYPE_FLOWEREX =", "TYPE_STONES =", "TYPE_BRANCHES =", "TYPE_LEAVES =", "TYPE_BLOODBONES =",};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newMini =") != std::string::npos) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newMini =", "mgr.miniCreate(", ",", targetFile);
    }
  }
}

auto mergeKeycodes_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  // Yes we duplicate the file, because ... legacy code
  const std::string mFile_s{[&sourceFile]() {
    std::string sourceFileAsString;
    for(const auto& line : sourceFile) {
      sourceFileAsString += line;
    }
    return sourceFileAsString;
  }()};

  std::string File_is{[&targetFile]() {
    std::string sourceFileAsString;
    for(const auto& line : targetFile) {
      sourceFileAsString += line;
    }
    return sourceFileAsString;
  }()};

  std::istringstream FileStream_mss(mFile_s);
  FileStream_mss.ignore(100000, '{');

  for(std::string FileSubstring_ms = ""; FileSubstring_ms.find("}") == std::string::npos;) {
    FileSubstring_ms = "";
    std::getline(FileStream_mss, FileSubstring_ms, ',');

    if(FileSubstring_ms.find('}') != std::string::npos) {
      break;
    }

    if(FileSubstring_ms.find("=") != std::string::npos) {

      FileSubstring_ms.erase(std::remove_if(FileSubstring_ms.begin(), FileSubstring_ms.end(), isspace), FileSubstring_ms.end()); // dont understand why removing whitespaces is so difficult. Isnt std::replace() easier to use ?
      std::string FileKeyword_ms = FileSubstring_ms.substr(0, FileSubstring_ms.find("="));
      FileKeyword_ms = ' ' + FileKeyword_ms + ' ';
      std::string mFileKeycode_ms = FileSubstring_ms.substr(FileSubstring_ms.find("=") + 1, std::string::npos);

      unsigned long long CountStart_i = File_is.find(FileKeyword_ms); //if the order of data in the file changes this code is prone to fail.
      if(CountStart_i < 100000) {

        unsigned long long CountTillDelim_i = CountStart_i;
        for(; File_is[CountTillDelim_i] != ','; ++CountTillDelim_i) {}

        File_is.replace(CountStart_i, CountTillDelim_i - CountStart_i, FileKeyword_ms + "= " + mFileKeycode_ms);
      }
      else {
        FileKeyword_ms.erase(0, 1);
        FileKeyword_ms = '\t' + FileKeyword_ms;

        CountStart_i = File_is.find(FileKeyword_ms); // Check happens again, the whole if() check for a \s and a \t version
        if(CountStart_i < 100000) {

          unsigned long long CountTillDelim_i = CountStart_i;
          for(; File_is[CountTillDelim_i] != ','; ++CountTillDelim_i) {}

          File_is.replace(CountStart_i, CountTillDelim_i - CountStart_i, FileKeyword_ms + "= " + mFileKeycode_ms);
        }
        else {
          CountStart_i = File_is.rfind('}');
          File_is.insert(CountStart_i - 1, "\n" + FileKeyword_ms + "= " + mFileKeycode_ms + ',');
        }
      }
    }
  }

  std::stringstream pipe{File_is};
  std::string line;
  std::vector<std::string> file;
  while(std::getline(pipe, line)) { file.push_back(line); }
  targetFile = file;
}

auto mergeEnvironment_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  const std::vector<std::string> Keywords_v = {"local TYPE_WATER", "local TYPE_LAVA", "local TYPE_PLASMA", "local TYPE_TENERGY"};
  MergeSegmentUniqueIdentifierLine(Keywords_v, sourceFile, targetFile);

  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("newWaterType =") == 0) {

      UppDelim_mit = MergeSegmentIdentifierInLowerDelimeter_lsit(UppDelim_mit, "newWaterType =", "mgr.waterTypeCreate(", ",", targetFile);
      continue;
    }
  }
}

auto mergeEliza_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("mgr.addDlgVoices(") != std::string::npos || (*UppDelim_mit).find("mgr.addDlgAdvanceVoices(") != std::string::npos || (*UppDelim_mit).find("mgr.addDlgFactionVoices(") != std::string::npos || (*UppDelim_mit).find("mgr.addDlgCombatVoices(") != std::string::npos) {

      std::string GroupIdentifier_s = (*UppDelim_mit).substr(0, (*UppDelim_mit).find("(") + 1);
      std::string SubIdentifier_s;
      if(GroupIdentifier_s == "mgr.addDlgVoices(" || GroupIdentifier_s == "mgr.addDlgCombatVoices(") { SubIdentifier_s = "group ="; }
      else { SubIdentifier_s = "race ="; }

      int iIdentiferStart = (*UppDelim_mit).find('"', (*UppDelim_mit).find(SubIdentifier_s));
      int iIdentifierEnd = (*UppDelim_mit).find('"', iIdentiferStart + 1);
      std::string Identifier_s = (*UppDelim_mit).substr(iIdentiferStart, iIdentifierEnd - iIdentiferStart + 1);

      auto UppDelim_riit = targetFile.rbegin();
      for(; UppDelim_riit != targetFile.rend(); UppDelim_riit++) {

        if((*UppDelim_riit).find(GroupIdentifier_s) != std::string::npos && (*UppDelim_riit).find(Identifier_s) != std::string::npos) {

          *UppDelim_riit = *UppDelim_mit;
          break;
        }
      }
      if(UppDelim_riit == targetFile.rend()) {

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&GroupIdentifier_s](const std::string& str) {return str.find(GroupIdentifier_s) != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        targetFile.insert(Insert_iit, *UppDelim_mit);
      }

      continue;
    }


    if((*UppDelim_mit).find("mgr.addDlgGroupDlg(") != std::string::npos) {

      auto LowDelim_mit = UppDelim_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find(")") == std::string::npos; LowDelim_mit++) {}

      auto UppDelim_riit = std::find(targetFile.rbegin(), targetFile.rend(), *UppDelim_mit);
      if(UppDelim_riit != targetFile.rend()) {

        auto UppDelim_iit = std::prev(UppDelim_riit.base());

        auto LowDelim_iit = UppDelim_iit;
        for(LowDelim_iit++; (*LowDelim_iit).find(")") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find(")") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        targetFile.insert(Insert_iit, UppDelim_mit, std::next(LowDelim_mit));
      }

      UppDelim_mit = LowDelim_mit;
      continue;
    }
  }
}

auto mergeAnimation_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  // DEPRECATED, deletes duplicated entries
  // if(DeleteDublicates2 == "Yes") {
  //   printAndLog("\n\tDublicated [base] entries are getting removed.\n", true);

  //   for(auto RemoveDublicates_iit = InstallationFile_ils.begin(); RemoveDublicates_iit != InstallationFile_ils.end(); ++RemoveDublicates_iit) {

  //     if((*RemoveDublicates_iit).find("[\"base\"]") != std::string::npos) {

  //       for(bool NoDublicate = false; NoDublicate != true;) {
  //         auto RemoveDublicates2_iit = std::find_if(std::next(RemoveDublicates_iit), InstallationFile_ils.end(), [&RemoveDublicates_iit](const std::string& str) {return str.find(*RemoveDublicates_iit) != std::string::npos;});

  //         if(RemoveDublicates2_iit != InstallationFile_ils.end()) {

  //           auto UppDelim_iit = RemoveDublicates2_iit;
  //           for(; (*UppDelim_iit).find("AnimInfo =") == std::string::npos; UppDelim_iit--) {}

  //           auto LowDelim_iit = RemoveDublicates2_iit;
  //           for(; (*LowDelim_iit).find("mgr.addAnimInfo") == std::string::npos; LowDelim_iit++) {}

  //           printAndLog(*RemoveDublicates2_iit + " dublicate has been removed.", true);

  //           InstallationFile_ils.erase(UppDelim_iit, std::next(LowDelim_iit));
  //         }
  //         else {
  //           NoDublicate = true;
  //         }
  //       }
  //     }
  //   }
  // }

  for(auto RemoveDublicatedEmptyLines_iit = targetFile.begin(); RemoveDublicatedEmptyLines_iit != targetFile.end(); RemoveDublicatedEmptyLines_iit++) {
    if((*RemoveDublicatedEmptyLines_iit == "") && (*std::next(RemoveDublicatedEmptyLines_iit) == "")) {
      targetFile.erase(std::next(RemoveDublicatedEmptyLines_iit));
      RemoveDublicatedEmptyLines_iit--;
    }
  }

  printAndLogLn("\n\tTrimmed multi-empty-lines to single-empty-lines\n");

  for(auto Identifier_mit = sourceFile.begin(); Identifier_mit != sourceFile.end(); Identifier_mit++) {

    if((*Identifier_mit).find("[\"base\"] =") != std::string::npos) {

      auto ItemType_mit = Identifier_mit;
      for(ItemType_mit--; (*ItemType_mit).find("[\"itemType\"] =") == std::string::npos; ItemType_mit--) {}

      auto UppDelim_mit = ItemType_mit;
      for(UppDelim_mit--; (*UppDelim_mit).find("AnimInfo =") == std::string::npos; UppDelim_mit--) {}

      auto LowDelim_mit = Identifier_mit;
      for(LowDelim_mit++; (*LowDelim_mit).find("mgr.addAnimInfo") == std::string::npos; LowDelim_mit++) {}


      auto Identifier_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
      if(Identifier_riit != targetFile.rend()) {

        auto Identifier_iit = std::prev(Identifier_riit.base());

        auto UppDelim_iit = Identifier_iit;
        for(UppDelim_iit--; (*UppDelim_iit).find("AnimInfo =") == std::string::npos; UppDelim_iit--) {}

        auto LowDelim_iit = Identifier_iit;
        for(LowDelim_iit++; (*LowDelim_iit).find("mgr.addAnimInfo") == std::string::npos; LowDelim_iit++) {}

        UppDelim_iit = targetFile.erase(UppDelim_iit, std::next(LowDelim_iit));
        targetFile.insert(UppDelim_iit, UppDelim_mit, std::next(LowDelim_mit));
      }
      else {

        auto Insert_riit = std::find_if(targetFile.rbegin(), targetFile.rend(), [](const std::string& str) {return str.find("mgr.addAnimInfo") != std::string::npos;});
        auto Insert_iit = Insert_riit.base();

        Insert_iit = targetFile.insert(Insert_iit, "");
        targetFile.insert(std::next(Insert_iit), UppDelim_mit, std::next(LowDelim_mit));
      }


      for(bool DublicatedItemType_b = true; DublicatedItemType_b == true;) { // If the loop cant find another instance of [itemType] it returns false. If it doesnt, it rerolls the ItemType ID and checks for uniqueness again.

        auto DublicatedItemType_iit = std::find_if(targetFile.begin(), targetFile.end(), [&Identifier_mit](const std::string& str) {return str.find(*Identifier_mit) != std::string::npos;});
        for(DublicatedItemType_iit--; (*DublicatedItemType_iit).find("[\"itemType\"] =") == std::string::npos; DublicatedItemType_iit--) {}

        auto DublicatedItemType_iit2 = std::find_if(targetFile.begin(), targetFile.end(), [&DublicatedItemType_iit](const std::string& str) {return str.find(*DublicatedItemType_iit) != std::string::npos;});
        auto DublicatedItemType_iit3 = std::find_if(std::next(DublicatedItemType_iit2), targetFile.end(), [&DublicatedItemType_iit](const std::string& str) {return str.find(*DublicatedItemType_iit) != std::string::npos;});

        if(DublicatedItemType_iit3 != targetFile.end()) {

          std::string OutputString = *ItemType_mit + " is not unique, [itemType] ID will get randomly rerolled: ";
          int RandNum_i = std::rand() % 99999 + 1;
          std::string NewItemTypeID_s = "\t[\"itemType\"] = ";
          NewItemTypeID_s = NewItemTypeID_s + std::to_string(RandNum_i) + ',';

          *DublicatedItemType_iit = NewItemTypeID_s;

          printAndLogLn(OutputString + std::to_string(RandNum_i));
        }
        else {

          DublicatedItemType_b = false;
        }
      }

      UppDelim_mit = LowDelim_mit;
    }
  }
}

//Update: While the original plan was to have generic merge codes, and specify them for each file, the realisation that every file is sufficiently different means a merge code will have to be written for each file. The advantages of this is that it makes troubleshooting easier.
auto mergeBalance_txt(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile) -> void
{
  for(auto UppDelim_mit = sourceFile.begin(); UppDelim_mit != sourceFile.end(); UppDelim_mit++) {

    if((*UppDelim_mit).find("local newBalanceValues =") != std::string::npos || (*UppDelim_mit).find("local beValues =") != std::string::npos ||
       (*UppDelim_mit).find("local basetables =") != std::string::npos) {

      std::string TableIdentifier_ms = (*UppDelim_mit).substr(0, (*UppDelim_mit).find("="));
      auto TableStart_iit = std::find_if(targetFile.begin(), targetFile.end(),
                                         [&TableIdentifier_ms](const std::string& str) {return str.find(TableIdentifier_ms) != std::string::npos;});

      auto TableLine_mit = UppDelim_mit;
      for(TableLine_mit++; (*TableLine_mit).find("mgr.") == std::string::npos; TableLine_mit++) {

        if((*TableLine_mit).find("=") != std::string::npos) {

          // Stringstream ignores possible isspace in front, and only streams a word until isspace is found again
          std::istringstream TableStream(*TableLine_mit);
          std::string TableIdentifier_s;
          TableStream >> TableIdentifier_s;
          TableIdentifier_s = TableIdentifier_s + " ";

          TableStream.str(std::string());
          TableStream.clear();

          bool NeedsInsertion_b = true;
          auto TableLine_iit = TableStart_iit;
          for(TableLine_iit++; (*TableLine_iit).find("mgr.") == std::string::npos; TableLine_iit++) {

            if((*TableLine_iit).find(TableIdentifier_s) != std::string::npos) {

              *TableLine_iit = *TableLine_mit;
              NeedsInsertion_b = false;
              break;
            }
          }

          if(NeedsInsertion_b) {

            targetFile.insert(std::prev(TableLine_iit), *TableLine_mit);
          }
        }
      }

      UppDelim_mit = TableLine_mit;
      continue;
    }

    //Now its getting crazy. We are supporting non specified code identifiers which will have to be linked in the base game dll. Update:Since identifers inside of "local subfam..." arent unique this is now part of the main MergeFileA procedure
    if((*UppDelim_mit).find("local subfamSlots =") != std::string::npos || (*UppDelim_mit).find("local subfamDroplists =") != std::string::npos || (*UppDelim_mit).find("local shrinkheadMinionMap =") != std::string::npos) {
      std::string TableIdentifier_ms = (*UppDelim_mit).substr(0, (*UppDelim_mit).find("="));
      auto TableStart_iit = std::find_if(targetFile.begin(), targetFile.end(), [&TableIdentifier_ms](const std::string& str) {return str.find(TableIdentifier_ms) != std::string::npos;});

      auto TableLine_mit = UppDelim_mit;
      for(TableLine_mit++; (*TableLine_mit).find("mgr.") == std::string::npos; TableLine_mit++) {

        if((*TableLine_mit).find("SUBFAM") != std::string::npos) {

          std::string TableIdentifier_s = (*TableLine_mit).substr((*TableLine_mit).find("\""), (*TableLine_mit).rfind("\"") - (*TableLine_mit).find("\"") + 1);

          bool NeedsInsertion_b = true;
          auto TableLine_iit = TableStart_iit;
          for(TableLine_iit++; (*TableLine_iit).find("mgr.") == std::string::npos; TableLine_iit++) {

            if((*TableLine_iit).find(TableIdentifier_s) != std::string::npos) {

              *TableLine_iit = *TableLine_mit;
              NeedsInsertion_b = false;
              break;
            }
          }

          if(NeedsInsertion_b) {

            targetFile.insert(std::prev(TableLine_iit), *TableLine_mit);
          }
        }
      }

      UppDelim_mit = TableLine_mit;
      continue;
    }
  }
}

/***********************
 File Functions end here
 ***********************/

auto processCommandLineArguments(const int argc, char const* const* const argv) -> void
{
  if(argc < 5) { throw std::runtime_error("Too few commandline arguments."); }
  for(std::size_t i{1}; i < argc; ++i) {

    if(std::string(argv[i]) == std::string("-h")) {
      printAndLogLn("Available options:");
      printAndLogLn("--source <source_path> | Specifies source path");
      printAndLogLn("--target <target_path> | Specifies target path");
      printAndLogLn("--overwrite | Overwrites all files instead of merging");
      continue;
    }

    if(std::string(argv[i]) == std::string("--source")) {
      settings.source = argv[++i];
      if(!std::filesystem::exists(settings.source)) { throw std::runtime_error("Source path doesnt exist"); }
      printAndLogLn("Source path: " + settings.source.string());
      continue;
    }

    if(std::string(argv[i]) == std::string("--target")) {
      settings.target = argv[++i];
      if(!std::filesystem::exists(settings.source)) { throw std::runtime_error("Target path doesnt exist"); }
      printAndLogLn("Target path: " + settings.target.string());
      continue;
    }

    if(std::string(argv[i]) == std::string("--overwrites")) {
      settings.overwrites = true;
      printAndLogLn("Overwriting all files");
      continue;
    }
  }
}

auto copyFile(const std::filesystem::directory_entry& entry, const std::filesystem::path& target) {
  // std::filesystem::copy_options::overwrite_existing is broken on gcc(on Windows) 14.2 and below
  // This fixed hasnt made it into gcc 14.2: https://github.com/gcc-mirror/gcc/commit/017e3f89b081e4828a588a3bd27b5feacea042b7
  if(std::filesystem::exists(target)) {
    // And because write protected files cant get std::filesystem::removed()ed we need to grant write permissions
    // https://github.com/microsoft/STL/issues/1511
    std::filesystem::permissions(target, std::filesystem::perms::owner_write, std::filesystem::perm_options::add);
    std::error_code error;
    std::filesystem::remove(target, error);
    if(error) {
      std::filesystem::permissions(target, std::filesystem::perms::owner_write, std::filesystem::perm_options::remove);
      throw std::filesystem::filesystem_error("Couldnt remove \"" + target.string() + "\" by granting write permissions", error);
    }
  }
  // this can randomely fail if the OS hogs the file
  std::filesystem::copy(entry.path(), target);
}

auto copyAllFiles() {
  printAndLogLn("Copying all regular files from directory ", settings.source, " to ", settings.target, ", and creating directory structures as needed");

  // this could be a coroutine
  const std::size_t maxFileCount = []() {
    std::size_t count{};
    for(const auto &entry : std::filesystem::recursive_directory_iterator(settings.source)) {
      if(entry.is_regular_file()) { ++count; };
    }
    return count;
  }();

  std::size_t filePos{};
  for(const auto &entry : std::filesystem::recursive_directory_iterator(settings.source))
  {
    const auto relativePath{std::filesystem::relative(entry.path(), settings.source)};
    const std::filesystem::path target{settings.target / relativePath};

    if(entry.is_directory() && !std::filesystem::exists(target)) {
      create_directory(target, entry.path());
    }
    if(!entry.is_regular_file()) { continue; }

    // this could also be a coroutine. No need to update this every file you copy over
    std::cout << "\rCopying: " << ++filePos << "/" << maxFileCount;

    copyFile(entry, target);
  }
}

const std::map<std::string, std::function<void(const std::vector<std::string>& sourceFile, std::vector<std::string>& targetFile)>> mergeFunctions{
  {"balance.txt", mergeBalance_txt},
  {"animation.txt", mergeAnimation_txt},
  {"Eliza.txt", mergeEliza_txt},
  {"environment.txt", mergeEnvironment_txt},
  {"keycodes.txt", mergeKeycodes_txt},
  {"minitype.txt", mergeMinitype_txt},
  {"patches.txt", mergePatches_txt},
  {"poidata.txt", mergePoidata_txt},
  {"soundprofile.txt", mergeSoundProfile_txt},
  {"soundresources.txt", mergeSoundresources_txt},
  {"surface.txt", mergeSurface_txt},
  {"blueprint.txt", mergeBlueprint_txt},
  {"creatures.txt", mergeCreatures_txt},
  {"drop.txt", mergeDrop_txt},
  {"equipsets.txt", mergeEquipsets_txt},
  {"faction.txt", mergeFaction_txt},
  {"portals.txt", mergePortal_txt},
  {"quest.txt", mergeQuest_txt},
  {"region.txt", mergeRegion_txt},
  {"spawn.txt", mergeSpawn_txt},
  {"triggerarea.txt", mergeTriggerarea_txt},
  {"triggervolumes.txt", mergeTriggervolumes_txt},
  {"waypoints.txt", mergeWaypoints},
  {"weaponpool.txt", mergeWeaponpool_txt},
  {"books.txt", mergeBooks_txt},
  {"creatureinfo.txt", mergeCreatureinfo_txt},
  {"defines.txt", mergeDefines_txt},
  {"iteminfo.txt", mergeIteminfo_txt},
  {"itemtype.txt", mergeItemtype_txt},
  {"material.txt", mergeMaterial_txt},
  {"spells.txt", mergeSpells_txt},
  {"typification.txt", mergeTypification_txt},
  {"behaviour.txt", mergeBehaviour_txt},
  {"autoexec.txt", mergeAutoexec_txt},
  {"heightmap.txt", mergeHeightmap_txt},
  {"optionsDefault.txt", mergeOptionsDefault_txt}
};

// TODO, put everything into a namespace ?

int main(int argc, char** argv)
{
  try {
    printAndLogLn("Welcome to the Sacred 2 Modmerge Tool");
    printAndLogLn("If you need help add -h as a command line argument");
    printAndLogLn("");

    processCommandLineArguments(argc, argv);
    printAndLogLn("");

    if(settings.overwrites) {
      copyAllFiles();
      printAndLogLn("All files copied");
      return 0;
    }

    const auto mergingStart = std::chrono::high_resolution_clock::now();
    for(const auto& entry : std::filesystem::recursive_directory_iterator(settings.source))
    {
      const auto relativePath{std::filesystem::relative(entry.path(), settings.source)};
      const std::filesystem::path target{settings.target / relativePath};
      if(entry.is_directory() && !std::filesystem::exists(target)) {
        create_directory(target, entry.path());
      }
      if(!entry.is_regular_file()) { continue; }

      if(!std::filesystem::exists(target)) {
        std::filesystem::copy(entry.path(), target);
        continue;
      }

      const auto sourceFile{readFileIntoContainer<std::vector<std::string>>(entry.path())};
      if(entry.path().extension() == ".txt" && std::find_if(sourceFile.begin(), sourceFile.end(), [](const std::string& str) {return str.rfind("--//OVERWRITE", 0) != str.npos;}) != sourceFile.end()) {
        printAndLogLn("Copying ... ", relativePath);
        copyFile(entry, target);
        continue;
      }

      auto targetFile{readFileIntoContainer<std::vector<std::string>>(target)};
      const auto nameFunctionPair{mergeFunctions.find(entry.path().filename().string())};
      if(nameFunctionPair != mergeFunctions.end()) {
        printAndLogLn(relativePath, " ... is getting scanned right now.");
        nameFunctionPair->second(sourceFile, targetFile);

        std::ofstream pipeOut(target);
        for(auto line = targetFile.begin(); line != targetFile.end(); ++line) {
          pipeOut << *line << std::endl;
        }
        pipeOut.close();
        printAndLogLn(relativePath, " ... has been scanned and merged for elements found.");
        continue;
      }

      /* Following files do not have modmerge functionality
       * "collision.txt", "pathObjects.txt", "questscripts.txt", "respawn.txt", "weather.txt", "worldobjects.txt", "treasure.txt", "genMipMapInfo.txt", "landscape.txt", "roadmap.txt", "relations.txt", "soundcluster.txt"
       */
      printAndLogLn("Copying ... ", relativePath);
      copyFile(entry, target);
    }

    const auto mergingEnd = std::chrono::high_resolution_clock::now();
    const auto MergeDuration = std::chrono::duration_cast<std::chrono::seconds>(mergingEnd - mergingStart).count();
    printAndLogLn("Merging and copying took: ", MergeDuration, "s");
  }
  catch(const std::exception& e) {
    printAndLogLn(e.what());
    return 1;
  }

  return 0;
}
