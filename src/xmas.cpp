// Compile with:  g++ xmas.cpp
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <random>

#define MYASSERT(e) if ( !(e) ) \
                      std::cerr<<"ASSERTION("<< #e <<") failed at "<< __LINE__ << std::endl

#define G_MAN "Gman"

bool  g_is_single = false;

using namespace std;

// NOTE: Names are purposefully arranged so that spouses are adjacent to
// each other.
const char* namestrs[] = { 
  "Jim",
  "Marian",
  "Dave",
  "Kim",
  "Lisa",
  "Mike",
  "Matt",
  "Heidi",
  "Mark",
  "Amanda",
  "Rose",
  "Jason",
  "Julie",
  "Ryan",
  "Gina",
  G_MAN
};

const size_t numNames = sizeof(namestrs)/sizeof(namestrs[0]);

const string wildCard = G_MAN;

const char fileNameFmt[] = "dowling_xmas_%d.txt";
const char entryFmt[] = "%s \thas \t%s";


#define HIST_CNT 3

typedef vector<string> NameList;
typedef set<string> SelectionHistory;
typedef map<string, SelectionHistory> NameHistoryMap;
typedef map<string, string> NameMap;


bool readYear(int year, NameHistoryMap& historyMap)
{
  char buff[64];
  char buff1[64];
  char buff2[64];
  char buff3[64];

  snprintf(buff, sizeof(buff), fileNameFmt, year);

  ifstream infile(buff);

  if ( infile.bad() || !infile )
    { return false; }

  cout<<"Found previous year's selections: "<<year<<endl;

  while (infile.getline(buff1, sizeof(buff1)) )
    {
      infile>>ws;

      int cc = sscanf(buff1, entryFmt, buff2, buff3);
      if ( cc!=2 )
        {
          cerr<<"Invalid file format: '"<<buff<<"`"<<endl;
          return false;
        }

      string srcName(buff2);
      string destName(buff3);
      NameHistoryMap::iterator nameIter = historyMap.find(srcName);
      if ( nameIter==historyMap.end() )
        {
          cerr<<"Unknown name: '"<<srcName<<"`"<<endl;
          continue;
        }

      NameHistoryMap::iterator nameIter2 = historyMap.find(destName);
      if ( nameIter==historyMap.end() )
        {
          cerr<<"Unknown name: '"<<destName<<"`"<<endl;
          continue;
        }

      if ( nameIter->second.insert(destName).second==false )
        {
          cerr<<"UNEXPECTED: "<<srcName<<" had "<<destName<<" twice!"<<endl;
          continue;
        }
    }

  return true;
}


bool initHistory(int year, NameHistoryMap& historyMap)
{
  // Read the previous years...
  int yy;
  for (yy=year-1; yy>=(year-HIST_CNT); yy--)
    { readYear(yy, historyMap); }

  // Don't allow people to pick their SO's...
  for (yy=0; yy<numNames; yy+=2)
    {
      string n1(namestrs[yy]), n2(namestrs[yy+1]);
      NameHistoryMap::iterator nameIter = historyMap.find(n1);
      NameHistoryMap::iterator nameIter2 = historyMap.find(n2);
      
      nameIter->second.insert(n2);
      nameIter2->second.insert(n1);
    }
  
  return true;
}



bool pickNames(const NameHistoryMap& historyMap,
               NameMap& nameMap)
{
  // Prime the random number generator.
  std::random_device rd;
  std::default_random_engine re; 
  re.seed(rd());

  std::uniform_int_distribution<size_t> rdist(0, numNames-1);

  NameList nameList;
  NameMap revMap;

  // Pre-populate the name map with the current names.
  int nn;
  for (nn=0; nn<numNames; nn++)
    { nameList.insert(nameList.end(),string(namestrs[nn])); }

  // Keep trying until we find a workable configuration.
  bool finished = false;
  while ( !finished )
    {
      revMap.clear();
      NameList::const_iterator currName = nameList.begin();
      const NameList::const_iterator endName = nameList.end();

      // Iterate through all the people, making selections as we go along.
      finished = true;
      for ( ; finished && currName!=endName; ++currName )
        {
          NameHistoryMap::const_iterator histIter = historyMap.find(*currName);
          NameMap::iterator pickIter = nameMap.find(*currName);
          SelectionHistory::const_iterator histEnd = histIter->second.end(); 

          MYASSERT( histIter!=historyMap.end() && pickIter!=nameMap.end() );
          
          // Make a random selection for this person
          string currPick;
          int count = 0;
          do {
            if ( ++count == 5 )
              {
                finished = false;
                break;
              }

            currPick = nameList[rdist(re)];

            // This is a workable selection if this person has not already
            // been selected for someone else and if this selection is not
            // already in this person's history.
            if ( currPick!=*currName
                 && revMap.find(currPick)==revMap.end() 
                 && histIter->second.find(currPick) == histEnd ) 
              { break; }
          } while ( 1 );

          if ( finished )
            {
              // Handle the wildcard specially.
              if ( g_is_single && *currName == wildCard )
                { 
                  // Somebody should've picked the wildcard by now...
                  NameMap::iterator wcIter= revMap.find(wildCard);
                  if ( wcIter != revMap.end() )
                    {
                      pickIter = nameMap.find(wcIter->second);
                      histIter = historyMap.find(wcIter->second);
                      histEnd = histIter->second.end(); 
                      
                      if ( wcIter->second!=*currName
                           && wcIter->second!=currPick
                           && histIter->second.find(currPick) == histEnd ) 
                        { 
                          cerr<<"Compressing wildcard : "<< wcIter->second
                              <<"  -->  "<<currPick<<endl;
                          pickIter->second = currPick;
                          MYASSERT(revMap.find(currPick)==revMap.end());
                          revMap[currPick] = wcIter->second;
                          nameMap.erase(wildCard);
                        }
                      else
                        {
                          cerr<<"Bad wildcard compression: "
                              <<wcIter->second<<"  -->  "<<currPick<<endl;
                          cerr<<"\t-- trying again..."<<endl;
                          finished = false;
                        }
                    }
                  else
                    {
                      cerr<<"ERROR -- Didn't find wildcard match..."<<endl;
                      finished = false;
                      break;
                    }
                }
              else
                {
                  // Remember this selection and remember the reverse
                  // mapping.
                  pickIter->second = currPick;
                  MYASSERT(revMap.find(currPick)==revMap.end());
                  revMap[currPick] = pickIter->first;
                }
            } // if finished...
        } // for all names...
      
    } // while still looking for valid mapping...
  
  return true;
}



bool writeYear(int year, const NameMap& nameMap)
{
  char buff[64];

  snprintf(buff, sizeof(buff), fileNameFmt, year);

  ofstream outfile(buff);

  if ( outfile.bad() || !outfile )
    {
      cerr<<"Could not open file: "<<buff<<endl;
      return false;
    }

  cout << endl;

  int nn;
  for (nn=0; nn<numNames; nn++)
    {
      const string name(namestrs[nn]);
      
      // Skip the wildcard...
      if ( g_is_single && name == wildCard )
        { continue; }

      NameMap::const_iterator nameIter = nameMap.find(name);
      
      if ( nameIter == nameMap.end() )
        {
          cerr<<"ERROR -- Could not find: " << name <<endl;
          continue;
        }

      snprintf(buff, sizeof(buff), entryFmt,
              nameIter->first.c_str(),
              nameIter->second.c_str());

      cout << buff <<endl;
      outfile<<buff<<endl;
    }

#if 0
  NameMap::const_iterator nameIter = nameMap.begin();
  const NameMap::const_iterator lastNameIter = nameMap.end();

  for ( ; nameIter != lastNameIter; ++nameIter )
    {
      sprintf(buff, entryFmt,
              nameIter->first.c_str(),
              nameIter->second.c_str());

      cout << buff <<endl;
      outfile<<buff<<endl;
    }
#endif

  return true;
}



int main(int argc, char *argv[])
{
  int year = 0;
  
  if ( argc < 2 )
    {
      time_t currTime = 0;
      time(&currTime);
      struct tm ts = *std::localtime(&currTime);

      year = ts.tm_year+1900;

      cout<<"Making selections for year: "<<year<<endl;
    }
  else
    { sscanf(argv[1], "%d", &year); }

  if ( year < 2002 )
    {
      cerr << "Invalid year!"<<endl;
      cerr<<"Format: \t"<<argv[0]<<" <year>"<<endl;
      return -2;
    }

  // Is Gina bringing a date?
  g_is_single = !!getenv("GINA_IS_SINGLE");
  if ( g_is_single )
    { cerr <<"Gina is apparently single..."<<endl; }

  NameHistoryMap historyMap; 
  NameMap nameMap; 

  // Pre-populate the name map with the current names.
  int nn;
  for (nn=0; nn<numNames; nn++)
    {
      const string name(namestrs[nn]);

      historyMap[name];
      nameMap[name];
    }

  initHistory(year, historyMap);

  if ( !pickNames(historyMap, nameMap) )
    { return -3; }

  writeYear(year, nameMap);

  return 0;
}

