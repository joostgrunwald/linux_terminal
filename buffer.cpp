/**
  * Assignment: synchronization
  * Operating Systems
  */

/**
  Hint: F2 (or Control-klik) on a functionname to jump to the definition
  Hint: Ctrl-space to auto complete a functionname/variable.
  */

//dependencies
#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
//includes from here on are our own
#include <string>

using namespace std;

mutex bufmutex;
mutex logmutex;
mutex boundmutex;

// ! Global variables
vector<int> buffer;
vector<string> logger;
bool bounded = false;
int bufferbound = 0;

// * This function writes a single log to the log vector
void writeToLog(string log)
{
  //! entire function is critical
  logmutex.lock();
  logger.push_back(log);
  logmutex.unlock();
}

// * This function reads a specific log sample, by using an index as input
// * the function checks for wrong indexes and returns errors if there are any
string readFromLog(int index)
{
  int size = logger.size();

  //? if size = 1, we index 0
  //! BEGIN OF CRITICAL SECTION
  logmutex.lock();
  if (index < 0)
  {
    logmutex.unlock();
    //? not in size and not higher then size so negative
    std::cout << "ERROR: negative index" << endl;
    return "ERROR: negative index";
  }
  else if (size >= index + 1)
  {
    string output = logger[index];
    logmutex.unlock();
    //? We use the stirng output because otherwise the return messes up our lock
    return output;
  }
  //! END OF CRITICAL SECTION
  else if (size < index + 1)
  {
    logmutex.unlock();
    std::cout << "ERROR: index out of bounds" << endl;
    return "ERROR: index out of bounds";
  }
  else {
    logmutex.unlock();
  }
}

// * This function prints the entire log onto the terminal
// * It checks if the log is not empty, and shows that it is empty if it is.
void printLog()
{  
  //ascii art for visual purposes
  cout << endl;
  cout << "     )                                                      " << endl;
  cout << "  ( /(                         )               (             " << endl;
  cout << "  )\\())         (  (      ) ( /((              )\\     (  (   " << endl;
  cout << " ((_)\\  `  )   ))\\ )(  ( /( )\\())\\  (   (     ((_)(   )\\))(  " << endl;
  cout << "   ((_) /(/(  /((_|()\\ )(_)|_))((_) )\\  )\\ )   _  )\\ ((_))\\  " << endl;
  cout << "  / _ \\((_)_\\(_))  ((_|(_)_| |_ (_)((_)_(_/(  | |((_) (()(_) " << endl;
  cout << " | (_) |  _ \\) -_)|  _/ _` |  _|| / _ \\   \\)) | / _ \\/ _` |  " << endl;
  cout << "  \\___/| .__/\\___||_| \\__,_|\\__||_\\___/_||_|  |_\\___/\\__, | " << endl;
  cout << "       |_|                                           |___/ " << endl;
  cout << endl;
  logmutex.lock();
  //! BEGIN OF CRITICAL SECTION
  if (logger.size() > 0)
  {
    for (string element : logger)
      std::cout << element << endl;
    //! END OF CRITICAL SECTION
    logmutex.unlock();
    std::cout << endl;
  }
  else
  {
    logmutex.unlock();
    std::cout << "The log is empty." << endl;
  }
}

// * This function writes to the buffer, it therefore
// * Checks wether or not it is already full if bounded
// * the function also updates the log with operation about its success
void writeToBuffer(int element)
{
  bufmutex.lock();
  boundmutex.lock();
  //! ENTIRE FUNCTION IS CRITICAL
  if (bounded == true)
  {
    if (buffer.size() < bufferbound)
    {
      // ? There is room left so we push
      buffer.push_back(element);
      writeToLog("operation succeeded: added " + to_string(element) + " to buffer.");
    }
    else if (buffer.size() == bufferbound)
    {
      writeToLog("operation failed: The buffer has already reached its bound.");
    }
    boundmutex.unlock();
  }
  else // ? not bounded
  {
    boundmutex.unlock();
    buffer.push_back(element);
    writeToLog("operation succeeded: added " + to_string(element) + " to buffer");
  }
  bufmutex.unlock();
}

// * This function sets the max bound of the buffer
// * as base case we check for legitimate inputs
// * It also provides the log with information about its success
void setBufferBound(int userbound)
{
  //! THIS SECTION IS CRITICAL
  boundmutex.lock();
  int usedbound = userbound;
  boundmutex.unlock();
  //! END OF CRITICAL SECTION

  if (usedbound == 0)
  {
    writeToLog("operation failed: invalid value 0 for parameter userbound.");
    cout << "you are putting in an invalid value (0), bound has to be > 0" << endl;
  }
  else if (usedbound < 0)
  {
    writeToLog("operation failed: negative value for parameter userbound.");
    cout << "you are putting in an invalid value (negative), bound has to be > 0" << endl;
  }
  else
  {
    writeToLog("operation succeeded: set " + to_string(usedbound) + " as buffer bound.");
    //! CRITICAL SECTION BEGIN
    boundmutex.lock();
    bounded = true;
    bufferbound = usedbound;

    boundmutex.unlock();
    bufmutex.lock();
    // ? Case where buffer > new bound, we remove elements exceeding bound
    if (buffer.size() > bufferbound)
    {
      //? we basicly remove range(userbound --> end of buffer)
      buffer.erase(buffer.begin() + usedbound, buffer.end());
    }
    bufmutex.unlock();
    //! CRITICAL SECTION END
  }
}

// * This function removes the buffer bound
void removeBufferBound()
{
  //! ENTIRE FUNCTION IS CRITICAL
  boundmutex.lock();
  bounded = false;
  writeToLog("operation succeeded: set the buffer bound to unbounded");
  boundmutex.unlock();
  // we do not have to reset the bound (because it will be reassigned when enabled again)
}

// * This function prints out the entire buffer to the screen
// * It checks if the buffer is empty and shows the buffer is empty if it is.
void printBuffer()
{
  bufmutex.lock();
  //! BEGIN OF CRITICAL SECTION
  if (buffer.size() > 0)
  {
    std::cout << "BUFFER: { ";
    for (int element : buffer)
      std::cout << element << ' ';
    bufmutex.unlock();
    //! END OF CRITICAL SECTION
    std::cout << "}" << endl;
  }
  else
  {
    bufmutex.unlock();
    std::cout << "buffer is empty" << endl;
  }
}

// * This function removes a specific element from the buffer
// * it also checks for wrong indexes
// * the function also writes to log about its succes
void removeFromBuffer(int index)
{
  bufmutex.lock();
  //! BEGIN OF CRITICAL SECTION
  int size = buffer.size();
  if (index < 0)
  {
    std::cout << "ERROR: negative index" << endl;
    writeToLog("operation failed: negative index supplied to remove.");
  }
  //size 1 = index 0, so index 1 should be illegal
  else if (index < size)
  {
    buffer.erase(buffer.begin() + index);
    bufmutex.unlock();
    writeToLog("operation succeeded: removed " + to_string(index) + " from buffer.");
  }
  //! END OF CRITICAL SECTION
  else if (index >= size)
  {
    std::cout << "ERROR: index out of bounds" << endl;
    writeToLog("operation failed: out of bounds index supplied to remove.");
  }
  bufmutex.unlock();
}

// * This function is used for testing some buffer operations
void buffertest1()
{
  std::cout << "empty buffer: " << endl;
  printBuffer();
  writeToBuffer(3);
  writeToBuffer(4);
  writeToBuffer(5);
  std::cout << "filled buffer: " << endl;
  printBuffer();
  removeFromBuffer(0);
  std::cout << "buffer - elem 0: " << endl;
  printBuffer();
}

// * This function is used for testing some special cases
void buffertest2()
{
  printLog();           //empty log
  printBuffer();        //empty buffer
  readFromLog(20);      //out of bounds
  readFromLog(-4);      //negative index
  setBufferBound(0);    //empty buffer size
  setBufferBound(-3);   //negative buffer size
  removeFromBuffer(0);  //declaring an empty buffer
  removeFromBuffer(20); //out of bounds
  removeFromBuffer(-2); //negative index
  printLog();           //show output in log of above edge cases
}

// * this function tests buffer bound edge cases
void buffertest3()
{
    //Test normal buffer bound
    printBuffer();
    writeToBuffer(3);
    writeToBuffer(4);
    writeToBuffer(5);
    writeToBuffer(3);
    writeToBuffer(4);
    writeToBuffer(5);
    writeToBuffer(3);
    writeToBuffer(4);
    writeToBuffer(5);
    printBuffer();
    setBufferBound(4);

    //Test incrementing buffer bounds
    printBuffer();
    writeToBuffer(3);
    writeToBuffer(4);
    writeToBuffer(5);
    printLog();
    printBuffer();
    setBufferBound(8);
    printBuffer();
    writeToBuffer(6);
    writeToBuffer(7);
    writeToBuffer(8);
    writeToBuffer(9);
    writeToBuffer(10);
    printBuffer();
    printLog();

    //Test what happens when we remove the buffer bound
    removeBufferBound();
    writeToBuffer(66);
    writeToBuffer(76);
    writeToBuffer(86);
    writeToBuffer(96);
    writeToBuffer(160);
    printBuffer();
    printLog();
}

// * This functions tests buffer functions threaded
void buffertest4()
{

  //test buffertest1 with threads
  std::thread t1 (buffertest1);  
  std::thread t2 (buffertest1); 
  t1.join();
  t2.join();

  //test buffertest2 with threads
  std::thread t3 (buffertest2);
  std::thread t4 (buffertest2);
  t3.join();
  t4.join();

  //test buffertest3 with threads
  std::thread t5 (buffertest3);
  std::thread t6 (buffertest3);
  t5.join();
  t6.join();

  //test the functions trough each other
  std::thread t7 (buffertest1);
  std::thread t8 (buffertest2);
  t7.join();
  t8.join();

  //test the functions trough each other
  std::thread t9 (buffertest1);
  std::thread t10 (buffertest3);
  t9.join();
  t10.join();
  
  //test the functions trough each other
  std::thread t11 (buffertest2);
  std::thread t12 (buffertest3);
  t11.join();
  t12.join();
}

// * This function tests threads with more then 2 threads
// * the main power of this is finding any deadlocks/starvation
void buffertest5()
{
  //test the functions trough each other
  std::thread t12 (buffertest1);
  std::thread t13 (buffertest2); //1 --> 2 --> 3 gives a mistake
  std::thread t14 (buffertest3);
  t12.join();
  t13.join();
  t14.join();

  std::thread t15 (buffertest1);
  std::thread t16 (buffertest1);
  std::thread t17 (buffertest1);
  t15.join();
  t16.join();
  t17.join();

  std::thread t18 (buffertest2);
  std::thread t19 (buffertest2);
  std::thread t20 (buffertest3);
  std::thread t21 (buffertest3);
  std::thread t22 (buffertest1);
  t18.join();
  t19.join();
  t20.join();
  t21.join();
  t22.join();

  std::thread t23 (buffertest1);
  std::thread t24 (buffertest3);
  std::thread t25 (buffertest2);
  std::thread t26 (buffertest3);
  std::thread t27 (buffertest1);
  std::thread t28 (buffertest2);
  std::thread t29 (buffertest3);
  std::thread t30 (buffertest3);
  std::thread t31 (buffertest2);
  t23.join();
  t24.join();
  t25.join();
  t26.join();
  t27.join();
  t28.join();
  t29.join();
  t30.join();
  t31.join();

}

//* This runs all tests combined
void alltests()
{
  buffertest2(); //edge case first
  buffertest1();
  buffertest3();
  buffertest4();
  buffertest5();
}

// * This is the main function, it is called when the program is ran
int main(int argc, char *argv[])
{
  alltests();
  return 0;
}