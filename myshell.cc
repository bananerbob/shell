#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <sys/wait.h>
#include <errno.h>                       // man errno for information
#include <cassert>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <stdlib.h>
#include <cstring>

using namespace std;

#define each(I) for( typeof((I).begin()) it=(I).begin(); it!=(I).end(); ++it )




void thread_run ( vector<string> tok){
  // Option processing: (1) redirect I/O as requested and (2) build  
  // a C-style list of arguments, i.e., an array of pointers to
  // C-strings, terminated by an occurrence of the null poiinter.
  //
  cout << "THREAD RUNNNING" << endl; 
  string progname = tok[0]; 
  char* arglist[ 1 + tok.size() ];   // "1+" for a terminating null ptr.
  int argct = 0;
  for ( int i = 0; i != tok.size(); ++i ) {
    if      ( tok[i] == "&" || tok[i] == ";" ) break;   // arglist done.
    else if ( tok[i] == "<"  ) freopen( tok[++i].c_str(), "r", stdin  );
    else if ( tok[i] == ">"  ) freopen( tok[++i].c_str(), "w", stdout );
    else if ( tok[i] == ">>" ) freopen( tok[++i].c_str(), "a", stdout );
    else if ( tok[i] == "2>" ) freopen( tok[++i].c_str(), "w", stderr );
    else if ( tok[i] == "|"  ) {                   // create a pipeline.
      int mypipe[2];   
      int& pipe_out = mypipe[0];
      int& pipe_in  = mypipe[1];
      // Find two available ports and create a pipe between them, and 
      // store output port# into pipe_out and input port# to pipe_in.
      if ( pipe( mypipe ) ) {     // All that is done here by pipe().
        cerr << "myshell: " << strerror(errno) << endl; // report err
        return;
      } else if ( fork() ) {  // you're the parent and consumer here.
    dup2( pipe_out, STDIN_FILENO ); // connect pipe_out to stdin.
        close( pipe_out );        // close original pipe connections.
        close( pipe_in );   
        while ( tok.front() != "|" ) tok.erase( tok.begin() );
        tok.erase(tok.begin());                    // get rid of "|".
        //exit( doit( tok ) );        // recurse on what's left of tok.
      } else {                 // you're the child and producer here.
        dup2( pipe_in, STDOUT_FILENO ); // connect pipe_in to stdout.
        close( pipe_out );        // close original pipe connections.
        close( pipe_in );
        break;                      // exec with the current arglist.
      } 
    } else {           // add this token a C-style argv for execvp().
      // Append tok[i].c_str() to arglist
      arglist[argct] = new char[1+tok[i].size()];  
      strcpy( arglist[argct], tok[i].c_str() );
      // arglist[argct] = const_cast<char*>( tok[i].c_str() );
      // Per C++2003, Section 21.3.7: "Nor shall the program treat 
      // the returned value [ of .c_str() ] as a valid pointer value 
      // after any subsequent call to a non-const member function of 
      // basic_string that designates the same object as this."
      // And, there are no subsequent operations on these strings.
      arglist[++argct] = 0; // C-lists of strings end w null pointer.
    }
  }
 

  // tilde expansion
  if ( progname[0] == '~' ) progname = getenv("HOME")+progname.substr(1);
  execvp( progname.c_str(), arglist );         // execute the command.
  // If we get here, an error occurred in the child's attempt to exec.
  cerr << "myshell: " << strerror(errno) << endl;     // report error.
  //exit(0);                  // child must not return, so must die now.
}

int doit( vector<string> tok ) {  
  // Executes a parsed command line returning command's exit status.

  if ( tok.size() == 0 ) return 0;             // nothing to be done.

  string progname = tok[0];  
  assert( progname != "" );

  // A child process can't cd for its parent.
  if ( progname == "cd" ) {                    // chdir() and return.
    chdir( tok.size() > 1 ? tok[1].c_str() : getenv("HOME") ); 
    if ( ! errno ) return 0;
    cerr << "myshell: cd: " << strerror(errno) << endl;
    return -1;
  } 

  cout << "Starting thread1" << endl; 
  // fork.  And, wait if child to run in foreground.
  if ( pid_t kidpid = fork() ) 
  {       
    // You're the parent.
    if ( errno || tok.back() == "&") return 0;
    int temp = 0;                
    waitpid( kidpid, &temp, 0 ); 
    return ( WIFEXITED(temp) ) ? WEXITSTATUS(temp) : -1;
  } 
  // You're the child.
  cout << "Starting thread2" << endl; 
  std::thread thread1 ( thread_run,tok); 
  cout << "Starting thread3" << endl; 
  thread1.join(); 
}


int main( int argc, char* argv[] ) {
  while ( ! cin.eof() ) {
    cout << "? " ;                                         // prompt.
    string temp = "";
    getline( cin, temp );
    cout.flush();
    if ( temp == "exit" ) break;                             // exit.

    stringstream ss(temp);      // split temp at white spaces into v.
    while ( ss ) {
      vector<string> v;
      string s;
      while ( ss >> s ) {
        v.push_back(s);
        if ( s == "&" || s == ";" ) break;    
      }
     // thread t(do_work);
      int status = doit( v );           // FIX make status available.
      if ( errno ) cerr << "myshell: " << strerror(errno) << endl;
    } 

  }
  cout << "exit" << endl;
  return 0;                                                  // exit.
}


///////////////// Diagnostic Tools /////////////////////////

   // cout.flush();
   // if ( WIFEXITED(status) ) { // reports exit status of proc.
   //   cout << "exit status = " << WEXITSTATUS(status) << endl;
   // }
