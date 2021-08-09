#include <SD.h>

/*
TODO:
clean up so file opening is abstracted away from main sketch. Happens transparently
parse and rewrite rootlog
console serial interface
send files to pi
handle many "start" statements in log
get_Count, get_Run_Time from main sketch
pause button
remote reset button
*/
/////
class Log
{
  public:
    Log();

    bool Exists(String filename);
    void New_Root_Log();
    void Initialize();
    void Open_Write_File(String file);
    void Calc_Running_Time();

    void RootLog_Entry(String date, int cycles, int time);
    void write_Mem_Avail(String location, int memory);
    void writeHeader();
    void writeTime();
    void writeCycle(int cycle_number, int cycle_time, float start_degrees, float end_degrees, int speed_setting);
    String readLine(File file);
    void write_String(String message);
    
    void printLog();
    void printLog(String name);
    void Print_RootLog();       // combine any of these
    void printDirectory();
    void printDirectory(File dir);
    void printDirRecursive(File dir, int numTabs);
    void print2digits(int number);
    
    int  parse_Cycle_Number(String line);
    time_t parseDate(String str);
    time_t parseStartTime(String line);
    void parse_Timestamp(String line);
    
    void parseCycle(String line);
    void parse_Cycle_Entry(String line); // combine any of these?
    void parseEntry(String line);
    void parseLine(String line);
    
    void Parse_Day_Log(String file);
    void skipLine();
    
    File writeFile;
    File readFile;
    File RootDir;
    File RootLog;
    char fileName[20];
    char* Name = (char*)malloc(20*sizeof(char));
    const int chipSelect = BUILTIN_SDCARD;
    int Log_Date;

    // Member Variables declared as PROGMEM to store in Flash instead of using SRAM
    const String MARK_NEW_ENTRY PROGMEM= "----------------------------------------------------------------------------------------";
    const char* SEPARATE_BY_SLASH PROGMEM= "////////////////////////////////////////////////////////////////////////////";
    const char* ROOTLOG_TXT PROGMEM= "RootLog.txt";
    const char* ROOTLOG_CREATED PROGMEM= "\tROOT LOG CREATED ";
    const char* NOTE PROGMEM= "Each entry is a daily total, not cumulative";
//    const char* ROOTLOG_CREATED PROGMEM= "\tROOT LOG CREATED ";
    const char* TIMESTAMP PROGMEM= "\t\tTimestamp\n";
    
    









    
};
