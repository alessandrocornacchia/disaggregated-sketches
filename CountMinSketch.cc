
#include "CountMinSketch.h"
/**
   Class definition for CountMinSketch.
   public operations:
   // overloaded updates
   void update(int item, int c);
   void update(char *item, int c);
   // overloaded estimates
   unsigned int estimate(int item);
   unsigned int estimate(char *item);
**/

simsignal_t CountMinSketch::pktProcessedSignal = registerSignal("packets");
simsignal_t CountMinSketch::counterLoadSignal = registerSignal("counterLoad");

Define_Module(CountMinSketch);

void CountMinSketch::create_counters() {
    // initialize counter array of arrays, C
    C = new int *[d];
    unsigned int i, j;
    for (i = 0; i < d; i++) {
        C[i] = new int[w];
        for (j = 0; j < w; j++) {
            C[i][j] = 0;
        }
    }
}

void CountMinSketch::init_hashes() {
    // initialize d pairwise independent hashes
    hashes = new int*[d];
    for (unsigned int i = 0; i < d; i++) {
        hashes[i] = new int[2];
        genajbj(hashes, i);
    }
}

// CountMinSketch constructor
// ep -> error 0.01 < ep < 1 (the smaller the better)
// gamma -> probability for error (the smaller the better) 0 < gamm < 1
void CountMinSketch::initialize()
{
    //total = 0;
    d = (int)par("d");
    w = (int)par("w");
    total = 0;

    if (d == -1 && w == -1) {
        eps = par("eps").doubleValue();
        gamma = par("delta").doubleValue();

        if (!(0.009 <= eps && eps < 1)) {
            throw cRuntimeError("eps must be in this range: [0.01, 1)");
          } else if (!(0 < gamma && gamma < 1)) {
            throw cRuntimeError("gamma must be in this range: (0,1)");
          }
          w = ceil(exp(1)/eps);
          d = ceil(log(1/gamma));
    }
    create_counters();
    init_hashes();

    WATCH(C);
}

void CountMinSketch::handleMessage(cMessage *msg)
{
    throw cRuntimeError("Not supposed to receive messages");
}

void CountMinSketch::finish() {

    emit(CountMinSketch::counterLoadSignal, double(total)/(d*w));

    // free array of counters, C
      unsigned int i;
      for (i = 0; i < d; i++) {
        delete[] C[i];
      }
      delete[] C;

      // free array of hash values
      for (i = 0; i < d; i++) {
        delete[] hashes[i];
      }
      delete[] hashes;
}

// CountMinSkectch destructor
//CountMinSketch::~CountMinSketch() {
//  // free array of counters, C
//  unsigned int i;
//  for (i = 0; i < d; i++) {
//    delete[] C[i];
//  }
//  delete[] C;
//
//  // free array of hash values
//  for (i = 0; i < d; i++) {
//    delete[] hashes[i];
//  }
//  delete[] hashes;
//}

// CountMinSketch totalcount returns the
// total count of all items in the sketch
unsigned int CountMinSketch::totalcount() {
  return total;
}

// hashes on row j to get one counter
unsigned int CountMinSketch::get_bucket_index(unsigned int j, int item, int M) {
    return (unsigned int)(((long)hashes[j][0]*item+hashes[j][1]) % LONG_PRIME) % M;
}

// countMinSketch update item count (int)
void CountMinSketch::update(int item, int c, int k) {

    Enter_Method("update()");
    total = total + c;

    emit(CountMinSketch::pktProcessedSignal, 1l);

    unsigned int hashval = 0;

    if(k == -1) { // use in the standard mode
        for (unsigned int j = 0; j < d; j++) {
            hashval = get_bucket_index(j,item, w);
            C[j][hashval] = C[j][hashval] + c;
        }
    } else if (k > 0 && k <= d) { // use a subset with offset
        int row = 0, col = 0;
        int M = d * w / k; // memory available for the each hash function when we use k of them
        for (unsigned int j = 0; j < k; j++) {
            hashval = get_bucket_index(j, item, M);
            reindex(hashval, j*M, &row, &col);
            C[row][col] = C[row][col] + c;
        }
    } else {
        throw cRuntimeError("Requested number of hash exceeds available");
    }
}

/*
 * given a "virtual" row index, i.e., bucket chosen by an hash function,
 * reindex maps the very same index onto the actual sketch organization
 */
void CountMinSketch::reindex(int vrow, int offset, int* row, int* col) {
    *row = floor(double(vrow + offset) / w);
    // column index
    *col = (vrow + offset) % w;
}

// countMinSketch update item count (string)
void CountMinSketch::update(const char *str, int c, int k) {
    int hashval = hashstr(str);
    update(hashval, c, k);
}

// CountMinSketch estimate item count (int)
unsigned int CountMinSketch::estimate(int item, int k) {
    Enter_Method("estimate()");
    int minval = numeric_limits<int>::max();
    unsigned int hashval = 0;

    if(k == -1) { // use in the standard mode
        for (unsigned int j = 0; j < d; j++) {
            hashval = get_bucket_index(j,item, w);
            minval = MIN(minval, C[j][hashval]);
        }
    }
    else if (k > 0 && k <= d) {
        int row = 0, col = 0;
        int M = d * w / k; // memory available for the each hash function when we use k of them
        for (unsigned int j = 0; j < k; j++) {
            hashval = get_bucket_index(j, item, M);
            reindex(hashval, j*M, &row, &col);
            minval = MIN(minval, C[row][col]);
        }
    } else {
        throw cRuntimeError("Requested number of hash exceeds available");
    }
    return minval;
}

// CountMinSketch estimate item count (string)
unsigned int CountMinSketch::estimate(const char *str, int k) {
    int hashval = hashstr(str);
    return estimate(hashval, k);
}

// generates aj,bj from field Z_p for use in hashing
void CountMinSketch::genajbj(int** hashes, int i) {
  hashes[i][0] = int(float(intuniform(0, RAND_MAX))*float(LONG_PRIME)/float(RAND_MAX) + 1);
  hashes[i][1] = int(float(intuniform(0, RAND_MAX))*float(LONG_PRIME)/float(RAND_MAX) + 1);
}

// generates a hash value for a sting
// same as djb2 hash function
unsigned int CountMinSketch::hashstr(const char *str) {
  unsigned long hash = 5381;
  int c;
  while (c = *str++) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}


