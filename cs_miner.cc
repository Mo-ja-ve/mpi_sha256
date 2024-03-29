//
// A simple CS Coin miner program for CS 4000, Homework #5
//
// The program reads in two big integers l and r, and stores them in the
// boost data structure cpp_int.
//
// This CS Coin miner starts at 0 and iteratively tries all integers until
// it finds an integer z such that l < shar256(z) < r.
// It then outputs z and quits.
//
// Written by David W. Juedes, April 9th, 2021.
//
//
#include <mpi.h>
#include <iostream>
#include <boost/multiprecision/cpp_int.hpp>
#include <string>
#include <iostream>
#include <iomanip>
#include <openssl/sha.h>
#include <cassert>
using namespace std;
using namespace boost::multiprecision;

//  function to broadcast to other processes, will be used twice in the main() for the two numbers from file we'll be using
void broadcast(unsigned char &cpp_intExport){

  MPI_Bcast(&cpp_intExport, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
}
//
// Function: to_bits
// Converts a cpp_int x into a vector of unsigned characters (8-bit ints).
//
//
vector<unsigned char> to_bits(cpp_int x) {
  unsigned char c;

  vector<unsigned char> buffer;
  while (x>0) {
    c = (unsigned char) x%256;
    buffer.push_back(c);
    x=x/256;
  }

  return buffer;
}

//
// Function sha256(x)
// Applies the standard sha256 algorithm from openssl to a cpp_int.
// Returns the corresponding cpp_int.
// Note: SHA256_DIGEST_LENGTH is 32.
// (8*32 = 256).
//
cpp_int sha256(const cpp_int x)
{
  vector<unsigned char> int_buffer;
  int_buffer = to_bits(x);
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(&int_buffer[0],int_buffer.size(),hash);
  cpp_int total = 0;
  for (int i=0;i<SHA256_DIGEST_LENGTH;i++) {
    total = total*256+(int) hash[i];
  }

  return total;
}


//  Andriy Kalinichenko
int main(int argc, char *argv[]) {

  // init stuff for mpi
  int num_procs, myid;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // reading in from file, storing each long number in v_int
  vector <cpp_int> v_int;
  if(myid == 0){
    while (!cin.eof()) {
      string line;
      getline(cin, line);
      if(!cin.fail()){
        v_int.emplace_back(line);
      }
    }
  }

  //  we need to do some conversion from cpp int into a format acceptable for MPI_Bcast
  //  we use boost import_bits and export_bits for such a process, converting our first number at v_int[0]
  //  into a vector of unsigned chars - call our broadcast function on each char in that vec - then convert that
  //  char vec back into cpp_int in each process
  vector <unsigned char> cpp_intExport;
  if(myid == 0)
    export_bits(v_int[0], back_inserter(cpp_intExport), 8);

  int num_export = cpp_intExport.size();
  MPI_Bcast(&num_export, 1, MPI_INT, 0, MPI_COMM_WORLD);
  cpp_intExport.resize(num_export);

  for(int i = 0; i < cpp_intExport.size(); i++){
    unsigned char charint = cpp_intExport[i];
    broadcast(charint);
    unsigned char temp = charint;
    cpp_intExport[i] = temp;
  }

  cpp_int t1 = 0;
  import_bits(t1, cpp_intExport.begin(), cpp_intExport.end());

  vector <unsigned char> cpp_intExport_2;

  if(myid == 0)
    export_bits(v_int[1], back_inserter(cpp_intExport_2), 8);

  num_export = cpp_intExport_2.size();
  MPI_Bcast(&num_export, 1, MPI_INT, 0, MPI_COMM_WORLD);
  cpp_intExport_2.resize(num_export);

  for(int i = 0; i < cpp_intExport_2.size(); i++){
    unsigned char charint = cpp_intExport_2[i];
    broadcast(charint);
    unsigned char temp = charint;
    cpp_intExport_2[i] = temp;
  }

  cpp_int t2 = 0;
  import_bits(t2, cpp_intExport_2.begin(), cpp_intExport_2.end());

  // broadcasting and conversions finished, we now check t1 is smaller than t2
  assert(t1 < t2);

  //  each process will now conver a range of numbers to traverse, searching for our lucky number
  //  the starting number for t will be determined by multiplying my id by 25000, and each processes
  //  will search based on it's designated 25000 range chunk - if the number is not found another
  //  itteration of this process will be done, using a larger chunk range each time - until our number is found
  cpp_int t      = (cpp_int)myid*25000;
  int stop_index = (int)t+25000;
  cpp_int next_sha;
  int found = 0;
  int local_found = 0;
  while (!found) {
    next_sha = sha256(t);
    if (t1 < next_sha) {
      if (next_sha < t2) {
        local_found = 1;
	//cerr << next_sha << endl;
	     cout << t << endl;
      }
    }
    //  found is used as our while loop control, so the process that finds our lucky number sets 'local_found'
    //  then allreduce is called thus taking the local_found set to 1 by the finding process and reducing it
    //  to all other processes thus breaking everyone out of the loop
    MPI_Allreduce(&local_found, &found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    t=t+1;
    if(t == stop_index){
      t = t*(cpp_int)num_procs;
      stop_index = (int)t + (num_procs*25000);
    }
  }

  MPI_Finalize();
}
