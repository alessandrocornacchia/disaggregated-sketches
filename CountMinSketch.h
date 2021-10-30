/** 
    Daniel Alabi
    Count-Min Sketch Implementation based on paper by
    Muthukrishnan and Cormode, 2004
**/

#ifndef __SKETCHES_CMS_H
#define __SKETCHES_CMS_H

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <omnetpp.h>

// define some constants
#define LONG_PRIME 4294967311l
#define MIN(a,b)  (a < b ? a : b)

using namespace omnetpp;
using namespace std;

/** CountMinSketch class definition here **/
class CountMinSketch : public cSimpleModule {


    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        static simsignal_t pktProcessedSignal;
        static simsignal_t counterLoadSignal;
        static simsignal_t elementInsertedSignal;

    private:
        // width, depth
          unsigned int w,d;

          // eps (for error), 0.01 < eps < 1
          // the smaller the better
          double eps;

          // gamma (probability for accuracy), 0 < gamma < 1
          // the bigger the better
          double gamma;

          // aj, bj \in Z_p
          // both elements of fild Z_p used in generation of hash
          // function
          unsigned int aj, bj;

          // total count so far
          unsigned int total;
          unsigned int unique_elements;

          // array of arrays of counters
          int **C;

          // array of hash values for a particular item
          // contains two element arrays {aj,bj}
          int **hashes;

          // generate "new" aj,bj
          void genajbj(int **hashes, int i);

          void reindex(int vrow, int offset, int* row, int* col);

          // get bucket index on row j
          unsigned int get_bucket_index(unsigned int j, int item, int M);

    public:
      // constructor
      //CountMinSketch(float eps, float gamma);
      //CountMinSketch(int d, int w);
      // destructor
      //~CountMinSketch();

      //CountMinSketch(int d, int m);

      // update item (int) by count c
      void update(int item, int c=1, int k=-1);
      // update item (string) by count c
      void update(const char *item, int c=1, int k=-1);

      // estimate count of item i and return count
      unsigned int estimate(int item, int k=-1);
      unsigned int estimate(const char *item, int k=-1);

      // return total count
      unsigned int totalcount();

      // return total count normalized to number of counters
      double normalizedtotalcount();

      // return total unique elements counted in the sketch
      unsigned int total_unique_elements();

      // notify the sketch that a new element has arrived (used to update stats)
      void notify_new_element();

      // generates a hash value for a string
      // same as djb2 hash function
      unsigned int hashstr(const char *str);

      void create_counters();
      void init_hashes();

};

#endif
