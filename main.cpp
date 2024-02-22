//Notes:
//1) Idea is simple. First create two groups and then try to optimize them
//   by swapping entries in the group with available entries
//2) Due to time constraints swapping between groups was not implemented,
//   therefore the algorithm works better with high number of available elements
//   and when the count of each entry is less than the target count for the group.
//3) Works with any target group size (not only 5).

#include <iostream>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <bitset>
#include <string.h>

using namespace std::chrono;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::bitset;
using std::pair;

class Entry{
  public:
    Entry(const std::string& name, int count, int strength):
      m_name{name}, m_count{count}, m_strength{strength}{
	m_weight = count*strength;
      }
    const string& name() const {return m_name;}
    int weight()         const {return m_weight;}
    int count()          const {return m_count;}
    int strength()       const {return m_strength;}
    void print()         const {
      cout << m_name << " " << m_count << " " << m_strength << " " << m_weight << endl;
    }
  private:
    string m_name;
    int m_count;
    int m_strength;
    int m_weight;
  };

class RawData : public vector<Entry>{
  public:
    double Weight(){return m_weight;}
    double updateWeight(){
      int weight = 0;
      int count = 0;
      for (auto c : *this){
	weight += c.weight();
        count  += c.count();
      }
      m_weight = weight;
      if (count) m_weight /= count;
      return m_weight;
    }
  private:
    double m_weight;
  };

class Group : public vector<int>{
  public:
    Group(int max_size):vector<int>(max_size), m_strength{0}, m_size{0}, m_count{0}{}
    void Print(const RawData& data, const vector<int>& jentry){
      for (int i = 0; i < m_size; ++i) cout << data[jentry[(*this)[i]]].name() << " ";
      cout << ": " << m_strength << endl;
    }
    int Strength() const {return m_strength;}
    int Size() const {return m_size;}
    int Count() const {return m_count;}
    void addElement(int iel){(*this)[m_size++] = iel;}
    void modifyStrength(int change){m_strength += change;}
    pair<int, int> updateStrengthCount(const RawData& data, const vector<int>& jentry){
      m_strength = 0;
      m_count = 0;
      for (int i = 0; i < m_size; ++i) {
        int j = (*this)[i];
        m_strength += data[jentry[j]].strength()*data[jentry[j]].count();
        m_count    += data[jentry[j]].count();
      }
      return std::make_pair(m_strength, m_count);
    }
    bool create(int cnt, const vector<int>& ientry, const vector<int>& jentry, vector<bool>& javail, 
	        int& search_time){
      if (cnt == 0) return true;
      auto start = high_resolution_clock::now();

      auto timeout = [&](){
        auto stop = high_resolution_clock::now();
        int elapsed = duration_cast<seconds>(stop - start).count();
	search_time -= elapsed;
	start = stop;
        if (search_time < 0) return true;
	return false;
      };
      
      int ist = -1;
      for (int i = ientry[cnt-1]; i < ientry[cnt]; ++i){
        if (javail[i]){
          ist = i;
          if (rand()%2 ||          //pick it
              i == ientry[cnt]-1){ //pick last one if available
	    //cout << __LINE__ << " Off " << i << endl;
            javail[i] = false;
            addElement(i);
            return true;
          }
        }
	if (timeout()) return false;
      }
      if (ist > -1){ //pick first one if last was not available
	 //cout << __LINE__ << " Off " << ist << endl;
         javail[ist] = false;
         addElement(ist);
         return true;
      }

      int n0 = cnt/2+cnt%2;
      int prev_size = m_size;
      for (int i = cnt-1; i >= n0; --i){
        if (create(i, ientry, jentry, javail, search_time))
          if (create(cnt-i, ientry, jentry, javail, search_time)) 
            return true;
	if (timeout()) return false;
        for (int j = prev_size; j < m_size; ++j) {
	  //cout << __LINE__ << " On " << (*this)[j] << endl;
	  javail[(*this)[j]] = true;
	}
        m_size = prev_size; //Rollback
      }
      return false;
    }
  private:
    int m_size;
    int m_count;
    int m_strength;
  };

class SwapFromAvailable{
  public:
    int expectedChange() const {return m_change;}
    void apply(vector<bool>& javail){
      if (m_group == nullptr) return;

      //cout << __LINE__ << " Off " << m_new_val << endl;
      assert(javail[m_new_val] == true);
      javail[m_new_val] = false;
      if (m_replace_idx >= 0){
        //cout << __LINE__ << " On " << m_old_val << endl;
        assert(javail[m_old_val] == false);
        javail[m_old_val] = true;
        (*m_group)[m_replace_idx] = m_new_val;
      }
      else{
	m_group->addElement(m_new_val);
      }
      m_group->modifyStrength(m_change);
    }
    
    Group* m_group;
    int m_replace_idx;
    int m_new_val;
    int m_old_val;
    int m_change;
  };

class SwapFromAvailableOperations : public vector<SwapFromAvailable>{
  public:
    int expectedChange() const {
      int change = 0;
      for (auto oper : *this){
	change += oper.expectedChange();
      }
      return change;
    }
    void apply(vector<bool>& javail){
      for (auto oper : *this){
	oper.apply(javail);
      }
    }
  private:
  protected:
  };

bool FindBetterInAvailable(
    const RawData& data, const vector<int>& ientry, const vector<int>& jentry, vector<bool>& javail, Group& group, 
    int cnt, int todo, int replace_idx, int old_strength, int target_diff, bool at_least_one, SwapFromAvailableOperations& operations)
  {
    vector<int> new_val(todo); 
    vector<int> best_val(todo), best2_val(todo); 
    int new_strength = 0, done = 0, best_diff = target_diff;
    int best2_diff = 0;
    bool found = false, found2 = false;
    for (int k = ientry[cnt-1]; k < ientry[cnt]; ++k){
      if (javail[k]){
        new_strength += data[jentry[k]].strength()*cnt;
        new_val[done] = k;
	++done;
	if (done == todo){
          int change = new_strength - old_strength;
          int new_diff = target_diff + change;
	  if (at_least_one){
            if (abs(new_diff) < abs(best2_diff)){
              best2_diff  = new_diff;
              for (int i = 0; i < todo; ++i) {
	        best2_val[i] = new_val[i];
	      }
	      found2 = true;
	    }
	  }
          if (abs(new_diff) < abs(best_diff)){
	    //cout << __LINE__ << " new_strength=" << new_strength << " old_strength=" << old_strength << " cnt=" << cnt << endl;
	    //cout << __LINE__ << " change=" << change << " " << abs(new_diff) << " " << abs(best_diff) << " " << target_diff << endl;
            best_diff  = new_diff;
            found = true;
            for (int i = 0; i < todo; ++i) {
	      best_val[i] = new_val[i];
	    }
          }
	  done = 0;
	  new_strength = 0;
        }
      }
    }
    if (at_least_one && !found && found2){
      best_val = best2_val;
      found = true;
    }
    if (found){
      int old_val  = replace_idx >= 0 ? group[replace_idx] : -1; //old element value
      new_strength = data[jentry[best_val[0]]].strength()*cnt;
      int change = new_strength - old_strength;
      //cout << "operation 0: " << replace_idx << " " <<  best_val[0] << " " << old_val << " " << change << " " << cnt << " " << todo << endl;
      operations.push_back(SwapFromAvailable{&group, replace_idx, best_val[0], old_val, change});
      for (int i = 1; i < todo; ++i){
        new_strength = data[jentry[best_val[i]]].strength()*cnt;
        change = new_strength;
        //cout << "operation " << i << ": " << -1 << " " <<  best_val[i] << " " << old_val << " " << change << endl;
        operations.push_back(SwapFromAvailable{&group, -1, best_val[i], -1, change});
      }
      return true;
    }

    return false;
  }

bool TryFromAvailable(
    const RawData& data, const vector<int>& ientry, const vector<int>& jentry, vector<bool>& javail, Group& group, 
    int target, SwapFromAvailableOperations& operations)
  {
    int target_diff = group.Strength() - target;
    //check 1 to 1
    for (int i = 0; i < group.Size(); ++i){
      int j = jentry[group[i]];
      int cnt = data[j].count();
      int old_strength = cnt*data[j].strength();
      bool found_better = FindBetterInAvailable(data, ientry, jentry, javail, group, cnt, 1, i, old_strength, target_diff, false, operations);
      if (found_better) return true;
    }
    //return false;
    //fprintf(stderr,"step FILE:%s LINE:%d \n", __FILE__, __LINE__);
    for (int i = 0; i < group.Size(); ++i){
      int j = jentry[group[i]];
      int cnt = data[j].count();
      int old_strength = cnt*data[j].strength();
      int n0 = cnt/2+cnt%2;
      for (int ist = cnt-1; ist >= n0; --ist){
	int jth = cnt - ist;
	for (int itimes = 1; itimes <= ist; ++itimes){
	  int icnt = ist/itimes;
	  if (icnt > 0 && ist%itimes == 0){
	    for (int jtimes = 1; jtimes <= jth; ++jtimes){
	      int jcnt = jth/jtimes;
	      if (jcnt > 0 && jth%jtimes == 0){
		assert(icnt*itimes+jcnt*jtimes == cnt);
                //cout << __LINE__ << " icount=" << icnt << " itodo=" << itimes << endl;
                //cout << __LINE__ << " jcount=" << jcnt << " jtodo=" << jtimes << " replace=" << i << endl;
		int itimes_tot = itimes;
                if (icnt == jcnt) {
		  jcnt = 0;
		  itimes_tot += jtimes;
		}
                bool found_better = FindBetterInAvailable(data, ientry, jentry, javail, group, icnt, itimes_tot, i, old_strength, target_diff, true, operations);
		//cout << "operations found i=" << operations.size() << endl;
		if (found_better && jcnt){
                  found_better = FindBetterInAvailable(data, ientry, jentry, javail, group, jcnt, jtimes, -1, 0, target_diff, true, operations);
		}
		//cout << "operations found j=" << operations.size() << endl;
		if (found_better){
                  int change = operations.expectedChange();
                  int new_diff = target_diff+change;
		  //fprintf(stderr,"step FILE:%s LINE:%d new_diff=%d target_diff=%d\n", __FILE__, __LINE__, new_diff, target_diff);
		  if (abs(new_diff) < abs(target_diff)) 
		    return true;
		}
	        operations.clear();
	      }
	    }
	  }
	}
      }
    }
    
    return false;
  }

bool BalanceTwoGroups(
    const RawData& data, 
    const vector<int>& ientry, const vector<int>& jentry, vector<bool> javail, 
    Group& group1, Group& group2, 
    int search_time, int max_oper, int thresh)
  {
    if (abs(group1.Strength() - group2.Strength()) <= thresh) return false; //quick return

    bool has_improved = false;
    auto start = high_resolution_clock::now();
    auto stop = start;
    int elapsed, noper = 0;
    SwapFromAvailableOperations ongroup1; 
    SwapFromAvailableOperations ongroup2; 
    constexpr int noperations = 2;
    bitset<noperations> better;
    do {
      better.reset();
      ++noper;
      //group1.Print(data, jentry);     
      //group2.Print(data, jentry);
      //fprintf(stderr,"step FILE:%s LINE:%d pass=%d.1\n", __FILE__, __LINE__, noper);
      better[0] = TryFromAvailable(data, ientry, jentry, javail, group1, group2.Strength(), ongroup1); 
      if(better[0]) {
	ongroup1.apply(javail);
        ongroup1.clear();
      }
      if (abs(group1.Strength() - group2.Strength()) <= thresh) return true; //quick return

      //group1.Print(data, jentry);     
      //group2.Print(data, jentry);
      //fprintf(stderr,"step FILE:%s LINE:%d pass=%d.2\n", __FILE__, __LINE__, noper);
      better[1] = TryFromAvailable(data, ientry, jentry, javail, group2, group1.Strength(), ongroup2);
      if(better[1]) {
	ongroup2.apply(javail);
        ongroup2.clear();
      }
      if (abs(group1.Strength() - group2.Strength()) <= thresh) return true; //quick return
      
      has_improved = has_improved || better.any();
      stop = high_resolution_clock::now();
      elapsed = duration_cast<seconds>(stop - start).count();
    }while (elapsed < search_time && noper < max_oper && better.any());

    return has_improved;
  }

class Parameters{
  public:
    unsigned int seed;
    unsigned int thresh;
    unsigned int group_count;
    unsigned int search_time;
    unsigned int max_oper;
    unsigned int max_trys;
  private:
  protected:
  };

  pair<RawData, RawData> generateTwoSimilarGroups(
      const RawData& data, const Parameters& param)
  {
    unsigned int seed         = param.seed       ;
    unsigned int thresh       = param.thresh     ;
    unsigned int group_count  = param.group_count;
    unsigned int search_time  = param.search_time;
    unsigned int max_oper     = param.max_oper   ;
    unsigned int max_trys     = param.max_trys   ;

    int nentries = data.size();
    vector<int> ientry(group_count+1, 0);
    vector<int> jentry(nentries);
    for (int i = 0; i < nentries; ++i){
      int cnt = data[i].count();
      if (cnt <= group_count && cnt > 0) //entries with count zero or above group_count are irrelevant
	++ientry[cnt];
    }
    for (int i = 0; i < group_count; ++i) 
      ientry[i+1] += ientry[i];
    for (int i = 0; i < nentries; ++i){
      int cnt = data[i].count()-1;
      if (cnt < group_count && cnt >= 0) {
	jentry[ientry[cnt]] = i;
	++ientry[cnt];
      }
    }
    for (int i = group_count-1; i >= 0; --i) 
      ientry[i+1] = ientry[i]; 
    ientry[0] = 0;

    //sort entries with same count according to strength
    for (int i = 0; i < group_count; ++i){
      std::sort(jentry.begin()+ientry[i], jentry.begin()+ientry[i+1], 
	   [&](int i, int j){return data[i].strength() < data[j].strength();});
    }

    //for (int i = 0; i < group_count; ++i){
    //  cout << "Count " << i+1 << "--->";
    //  for (int j = ientry[i]; j < ientry[i+1]; ++j)
    //    cout << data[jentry[j]].name() << "(" << data[jentry[j]].count() << "," << data[jentry[j]].strength() << ") ";
    //  cout << endl;
    //}
    
    srand(seed);
    Group group1_best(group_count);
    Group group2_best(group_count);
    for (int i = 0; i < max_trys; ++i){
      //cout << "Group " << i+1 << endl << "-----------" << endl;
      vector<bool> javail(nentries, true);
      Group group1(group_count);
      int max_time = search_time;
      if (!group1.create(group_count, ientry, jentry, javail, max_time)) continue; 
      group1.updateStrengthCount(data, jentry);

      Group group2(group_count);
      max_time = search_time;
      if (!group2.create(group_count, ientry, jentry, javail, max_time)) continue; 
      group2.updateStrengthCount(data, jentry);

      //group1.Print(data, jentry);
      //group2.Print(data, jentry);

      bool found_improved = BalanceTwoGroups(data, ientry, jentry, javail, group1, group2, search_time, max_oper, thresh);
      assert(group1.Strength() == group1.updateStrengthCount(data, jentry).first);
      assert(group2.Strength() == group2.updateStrengthCount(data, jentry).first);
      //DO NOT MOVE these above
      assert(group1.Count() == group_count); 
      assert(group2.Count() == group_count);
      //if (found_improved){
      //  cout << "Group " << i+1 << endl << "-----------" << endl;
      //  group1.Print(data, jentry);
      //  group2.Print(data, jentry);
      //}

      if (i == 0 || abs(group1_best.Strength() - group2_best.Strength()) > abs(group1.Strength() - group2.Strength())){
	group1_best = group1;
	group2_best = group2;
      }
    }
    //group1_best.Print(data, jentry);
    //group2_best.Print(data, jentry);
    RawData data1, data2;
    for (int i = 0; i < group1_best.Size(); ++i){
      int j = jentry[group1_best[i]];
      data1.push_back(data[j]);
    }
    data1.updateWeight();
    for (int i = 0; i < group2_best.Size(); ++i){
      int j = jentry[group2_best[i]];
      data2.push_back(data[j]);
    }
    data2.updateWeight();
    return std::make_pair(data1, data2);
  }

int main(int nargs, char**argv )
  {
    unsigned int seed   = -1;
    unsigned int thresh =  0;
    unsigned int group_count  =  5;
    unsigned int search_time  =  2;
    unsigned int max_oper  = 10;
    unsigned int max_trys  = 10;
    unsigned int ecount    = -1;
    unsigned int ewgt      = 100;
    unsigned int popsz     = 200;

    for (int i = 1; i < nargs; ++i){
      char* eq = strchr(argv[i], '='); 
      if (eq){
	*eq = '\0';
        if      (strcmp(argv[i], "-seed")   == 0)  seed        = atoi(eq+1); 
        else if (strcmp(argv[i], "-thresh") == 0)  thresh      = atoi(eq+1); 
        else if (strcmp(argv[i], "-gcount") == 0)  group_count = atoi(eq+1); 
        else if (strcmp(argv[i], "-time")   == 0)  search_time = atoi(eq+1); 
        else if (strcmp(argv[i], "-noper")  == 0)  max_oper    = atoi(eq+1); 
        else if (strcmp(argv[i], "-ntrys")  == 0)  max_trys    = atoi(eq+1); 
        else if (strcmp(argv[i], "-ecount") == 0)  ecount      = atoi(eq+1); 
        else if (strcmp(argv[i], "-ewght")  == 0)  ewgt        = atoi(eq+1); 
        else if (strcmp(argv[i], "-popsz")  == 0)  popsz       = atoi(eq+1); 
      }
    }
    //initialize random number generation for entry picking
    if (seed == -1){
      time_t t;
      seed = (unsigned int) time(&t);
    }
    if (ecount == -1) ecount = group_count;

    cout << argv[0] << " -seed=" << seed << " -thresh=" << thresh << " -gcount=" << group_count
         << " -time=" << search_time << " -noper=" << max_oper << " -ntrys=" << max_trys 
	 << " -ecount=" << ecount << " -ewght=" << ewgt << " -popsz=" << popsz
	 << endl;
    Parameters param{seed, thresh, group_count, search_time, max_oper, max_trys};
 
    RawData data;
    /*data.push_back(Entry{"A", 1, 600});
    data.push_back(Entry{"B", 2, 310});
    data.push_back(Entry{"C", 2, 200});
    data.push_back(Entry{"D", 2, 300});
    data.push_back(Entry{"E", 4, 450});
    data.push_back(Entry{"F", 1, 220});
    data.push_back(Entry{"G", 3, 195}); **/
    char name[4];
    for (int i = 0; i < popsz; ++i){
      for (int j = 0; j < 4; ++j) name[j] = rand()%26 + 97;
      int cnt = rand()%ecount+1;
      int wgt = rand()%ewgt+1;
      data.push_back(Entry{name, cnt, wgt});
    }
    cout << "RawData" << endl << "----------" << endl;
    for (auto c : data) c.print(); cout << endl;
    std::pair<RawData, RawData> groups = generateTwoSimilarGroups(data, param);
    

    cout << "Group1: " << groups.first.Weight() << endl << "----------" << endl;
    for (auto c : groups.first) c.print(); cout << endl;

    cout << "Group2: " << groups.second.Weight() << endl << "----------" << endl;
    for (auto c : groups.second) c.print(); cout << endl;

    cout << "Similarity:" << abs(groups.first.Weight() - groups.second.Weight()) << endl;
    return 0;
  }
