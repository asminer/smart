
// $Id$

#include "mdds.h"
#include "mdd_ops.h"
#include <stdlib.h>
#include "../Base/timers.h"

int K;
int* root;
int* size;
node_manager bar;
operations cruft(&bar);
timer stopwatch;

// #define SHOW_MXD
// #define SHOW_FINAL
// #define SHOW_DISCONNECTED

void ReadMDD(const char* filename)
{
  FILE* foo = fopen(filename, "r"); 
  if (NULL==foo) {
    Output << "Couldn't open file " << filename << "\n"; 
    Output.flush();
    return;
  }
  InputStream in(foo);

  // File format is:
  // index
  // level
  // truncated--full size
  // entry1 ... entryn

  int mapsize = 1024;
  int* indexmap = (int*) malloc(mapsize*sizeof(int));
  indexmap[0] = 0;
  indexmap[1] = 1;
  
  int index;
  int level;
  while (in.Get(index)) {
    int sz;
    in.Get(level);
    in.Get(sz);
    if (level>0) size[level] = MAX(size[level], sz);
    if (level<0) size[-level] = MAX(size[-level], sz);
    int n = bar.TempNode(level, sz);
    if (index>=mapsize) {
      mapsize = (1+index/1024)*1024;
      indexmap = (int *) realloc(indexmap, mapsize*sizeof(int));
      DCASSERT(indexmap);
    }
    // read, translate, and store the pointers
    for (int i=0; i<sz; i++) {
      int ptr;
      in.Get(ptr);
      bar.Link(indexmap[ptr]);
      bar.SetArc(n, i, indexmap[ptr]);
    }
    indexmap[index] = bar.Reduce(n);
  }
  DCASSERT(0==root[level]);
  root[level] = indexmap[index];
  free(indexmap);
}

int MakeString(int K) 
{
  int below = 1;
  for (int k=1; k<=K; k++) {
    int a = bar.TempNode(k, size[k]);
    bar.SetArc(a, 0, below);
    below = a;
  }
  return below;
}

void smart_exit()
{
}

// returns true on success
bool ProcessArgs(int argc, char** argv)
{
  int p;
  int T;
  for (p=1; ; p++) {
    if (p>=argc) return false;

    if (argv[p][0] == '-') {
      // we have an option
      switch (argv[p][1]) {

	case 'o':	// optimistic caches
			bar.SetPessimism(false);
			continue;

	case 'p':	// pessimistic caches
			bar.SetPessimism(true);
			continue;

        case 'r':	// recycle holes
			bar.SetHoleRecycling(true);
			continue;

	case 'l':	// Don't recycle holes
			bar.SetHoleRecycling(false);
			continue;

	case 'c':	// grab compaction threshold
			p++;
			if (p>=argc) return false;
			T = atoi(argv[p]);	
			if (T<=0) return false;	
			bar.SetCompactionThreshold(T);
			continue;
			
      } // switch

      // bad option
      return false;
    } // if -
 
    // should be K
    break;
  }

  if (p+2 >= argc) return false;  // not enough args left!

  K = atoi(argv[p]);
  if (K<=0) return false;

  size = new int[K+1];
  root = new int[K+1];

  for (p++; p<argc; p++) {
    Output << "Reading " << argv[p] << "\n";
    Output.flush();
    ReadMDD(argv[p]);
  }

  while (1) {
    if (size[K]) break;
    if (0==K) break;
    K--;
  }

  return true;
}

int main(int argc, char** argv)
{
  if (!ProcessArgs(argc, argv)) {
    Output << "Usage: " << argv[0] << " [options] K file1 ... filen\n";
    Output << "\nOptions are (only the first char. matches):\n";
    Output << "\t-o\tUse optimistic caches (default)\n";
    Output << "\t-p\tUse pessimistic caches\n";
    Output << "\n";
    Output << "\t-r\tRecycle holes (default)\n";
    Output << "\t-l\tLazy: Don't recycle holes\n";
    Output << "\n";
    Output << "\t-c #\tSet compaction threshold value\n";
    Output << "\n";
    return 0;
  }

  if (bar.IsPessimistic()) 
	Output << "Using pessimistic caches\n";
  else
	Output << "Using optimistic caches\n";

  if (bar.AreHolesRecycled())
	Output << "Holes are recycled\n";
  else
	Output << "Holes are not recycled\n";

  Output << "Compaction threshold is " << bar.CompactionThreshold() << " slots\n";

  Output << "Sizes: [";
  Output.PutArray(size+1, K);
  Output << "]\n";
  Output << "Transitions by level:\n";
  for (int i=K; i; i--) 
    if (root[i]) 
      Output << "\t" << i << " : " << root[i] << "\n";

  int reachset = MakeString(K);

#ifdef SHOW_MXD
  Output << "Initial: " << reachset << "\n";
  bar.Dump(Output);
#endif

  Output << bar.CurrentNodes() << " nodes used for transitions\n";

  Output << "Starting " << K << " level saturation\n"; 
  Output.flush();

  stopwatch.Start();
  reachset = cruft.Saturate(reachset, root, size, K);  
  stopwatch.Stop();
  Output << "Saturation took " << stopwatch << "\n";

  int card = cruft.Count(reachset);

  Output << card << " reachable states\n";

  Output << "max hole chain: " << bar.MaxHoleChain() << "\n";
  Output.Pad('-', 60);
  Output << "\n";
  cruft.CacheReport(Output);

  // bar.DumpInternal(Output);
  
  Output << "Clearing cache\n";
  Output.flush();
  cruft.ClearUCache();
  cruft.ClearFCache();

#ifdef SHOW_DISCONNECTED
  Output << "Disconnected, undeleted nodes:\n";

  int lm = 2+bar.PeakNodes();
  bool* marked = new bool[lm];
  for (i=0; i<lm; i++) marked[i] = false;
  cruft.Mark(reachset, marked);
  for (i=K; i; i--) cruft.Mark(root[i], marked);
  
  int active = 0;
  for (i=2; i<lm; i++) {
    if (marked[i]) {
      active++;
      continue;
    }
    if (bar.isNodeDeleted(i)) continue;
    Output.Put(i, 6);
    Output << "\t";
    bar.ShowNode(Output, i);
    Output << "\n";
    Output.flush();
  }
#endif

#ifdef SHOW_FINAL
  Output << "Reachset: " << reachset << "\n";
  bar.Dump(Output);
#endif

  bar.Compact();

  Output.Pad('-', 60);
  Output << "\nNodes: \t" << bar.PeakNodes() << " peak ";
  Output << "\t" << bar.CurrentNodes() << " current\n";

  Output << "Memory: \t" << bar.PeakMemory() << " peak ";
  Output << "\t" << bar.CurrentMemory() << " current\n";
  Output << "\t\t" << bar.CurrentMemory() - bar.MemoryHoles() << " actual ";
  Output << "\t" << bar.MemoryHoles() << " in holes\n";

  Output << "\n";
  Output << "max hole chain: " << bar.MaxHoleChain() << "\n";
  Output << "number of compactions: " << bar.NumCompactions() << "\n";

  Output << "\n";

  return 0;
}

