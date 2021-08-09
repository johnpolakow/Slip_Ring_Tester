#include "Log.h"
#include <Wire.h>
#include <TimeLib.h>
#include <ctime>
#include <string.h>
#include <DS1307RTC.h>

tmElements_t rtc_time;
String Sfilename;
String Date_Today;
int Log_Number=1;

struct log_data{
  time_t  start_time=0;
  time_t  running_time=0;
  time_t  last_timestamp=0;
  time_t date = 0;
  int count=0;
};

struct log_data parse_log;
int count_last_run =0;

// Note: All write operations are ended with writeFile.flush(), which writes the buffer to disk. 
Log::Log()
{
  Serial.println(F("Opening SD card..."));
  if (!SD.begin(chipSelect)){ Serial.println(F("Cannot open SD card")); }
  
  RootDir = SD.open("/");
  Log::printDirectory(RootDir); // List SD contents
  RootDir.close();
  SD.remove("6-21-18.txt");
  if (!Log::Exists("RootLog.txt"))
  {
    Serial.println(F("No RootLog"));
    Log::New_Root_Log();
  }
  Log::Print_RootLog();
  Log::Initialize();
  // Read Values from Previous Log File
}

bool Log::Exists(String filename) 
{
  strcpy(fileName, filename.c_str());
  filename.toCharArray(fileName, sizeof(fileName));
  if(SD.exists(fileName)){ return true; }
  else return false;
}

void Log::Initialize()
{
  // Create Log Filename for Today
  RTC.read(rtc_time);   // reads time now
  Date_Today = (String)rtc_time.Month+"-"+(String)rtc_time.Day+"-"+(String)(tmYearToCalendar(rtc_time.Year)-2000);
  Sfilename = Date_Today+".txt";
  strcpy(fileName, Sfilename.c_str());  // copy string to char array
  Sfilename.toCharArray(fileName, sizeof(fileName));
  Serial.print(F("LogFile for Today: ")); Serial.println(fileName);
  Log_Date = rtc_time.Day;    // save day of month of this log


      //SD.remove(fileName);

      
  delay(5);
  if(SD.exists(fileName)){ Serial.print(fileName);Serial.println(F(" already exists, appending")); 
        writeFile = SD.open(fileName, FILE_WRITE);
        writeFile.println(MARK_NEW_ENTRY);  // printing a marker where testing resumed
        writeFile.print("Start: "); 
        Log::writeTime();
        //writeFile.close();
  }
  else    // create new log file for today
  {  
        Serial.println(F("Creating new LogFile, doesnt exist yet"));
        writeFile = SD.open(fileName, FILE_WRITE);
        //Serial.print("file name: "); Serial.println(fileName);
        //Serial.print("file open: "); Serial.println(writeFile);
        Log::writeHeader();
        //writeFile.close();
  }
}

void Log::Open_Write_File(String file)
{
  Sfilename = Date_Today+".txt";
  strcpy(fileName, file.c_str());  // copy string to char array
  file.toCharArray(fileName, sizeof(fileName));
  Serial.print(F("LogFile for Today: ")); Serial.println(fileName);
  //Log_Date = rtc_time.Day;    // save day of month of this log

  writeFile.close();
  delay(10);
  writeFile = SD.open(fileName);
  Serial.print("result of file open: "); Serial.println(writeFile);
  
}

void Log::New_Root_Log()
{
  RTC.read(rtc_time);
  Date_Today = (String)rtc_time.Month+"-"+(String)rtc_time.Day+"-"+(String)tmYearToCalendar(rtc_time.Year);
  
  RootLog = SD.open(ROOTLOG_TXT, FILE_WRITE);
  RootLog.print(ROOTLOG_CREATED); RootLog.println(Date_Today); 
  RootLog.println(SEPARATE_BY_SLASH);
  RootLog.println(NOTE);
  RootLog.println(MARK_NEW_ENTRY);
  RootLog.println("6-5-2018 \tCycles: 450 \t Time: \t 0h 31min 0sec");
  RootLog.println(MARK_NEW_ENTRY);
  RootLog.close();
}

void Log::RootLog_Entry(String date, int cycles, int time)
{
  RootLog = SD.open(ROOTLOG_TXT, FILE_WRITE);
  RootLog.print(date); 
  RootLog.print("\tCycles: "); RootLog.print(cycles);
  //RootLog.print("\tTime: "); RootLog.println(rtc_time);
  RootLog.println(MARK_NEW_ENTRY);
  RootLog.close();
}

void Log::Print_RootLog()
{
    Serial.println(F("RootLog: "));
    Serial.println();
    String line;
    RootLog = SD.open(ROOTLOG_TXT);
    while(RootLog.available()) 
    {
        line = readLine(RootLog);
        if(line.length() > 0) Serial.println(line);
    }
    Serial.println();
    RootLog.close();
}

void Log::write_Mem_Avail(String location, int memory)
{
  writeFile.print("\tMem: ");
  writeFile.println(memory);
}

void Log::writeHeader()
{
    writeFile.println(fileName);
    //writeFile.print("Start Time: "); Log::writeTime();
    Serial.println();
    writeFile.print("\tCycle Time \tStart(degs) \tEnd(degs) \tSpeed Setting(degs/sec)"); //writeFile.print("\n");
    writeFile.print(TIMESTAMP);
    writeFile.println(MARK_NEW_ENTRY);
    writeFile.print("Start: "); 
    Log::writeTime(); // writeTime has the flush() operation
}

void Log::write_String(String message)
{
    writeFile.println(message); 
}

void Log::writeCycle(int cycle_number, int cycle_time_ms, float start_degrees, float end_degrees, int speed_setting)
{
    writeFile.print("In write cycle"); 
    //writeFile = SD.open(fileName, FILE_WRITE);
    //Serial.print("writeFile Status: "); Serial.println(writeFile);
    float time_seconds = cycle_time_ms/1000.00;
    RTC.read(rtc_time);
    int today = rtc_time.Day; // numeric day of the month
    if(today != Log_Date) // Start new Log
    {
        // Create Log for new day (after midnight)
        writeFile.println("New day, starting a new log");
        RTC.read(rtc_time);
        Date_Today = (String)rtc_time.Month+"-"+(String)rtc_time.Day+"-"+(String)(tmYearToCalendar(rtc_time.Year)-2000);
        Sfilename = Date_Today+".txt";
        strcpy(fileName, Sfilename.c_str());
        Sfilename.toCharArray(fileName, sizeof(fileName));
        writeFile.print("New fileName: "); writeFile.println(fileName);
        Log_Date = rtc_time.Day;
        writeFile = SD.open(fileName, FILE_WRITE);
        Log::writeHeader();
        writeFile.flush();
    }
    writeFile.print("C"); writeFile.print(cycle_number); writeFile.print(" ");
    writeFile.print("\ttime: "); writeFile.print(time_seconds); writeFile.print("s");
    writeFile.print("\tstart: "); writeFile.print(start_degrees);writeFile.print("*");
    writeFile.print("\tend: "); writeFile.print(end_degrees);writeFile.print("*");
    writeFile.print("\tspeed: "); writeFile.print(speed_setting); writeFile.print("*/sec");
    writeFile.print("\t\t");
    Log::writeTime(); // writeTime() has the flush() operation
    //writeFile.close(); 
    //writeFile.println("Leaving write cycle"); 
    writeFile.flush();
}

void Log::writeTime()
{
      Log::write_String("In Write Time"); 
  //writeFile = SD.open(fileName, FILE_WRITE);
  RTC.read(rtc_time);
  //Serial.print("time: ");Serial.print(rtc_time.Hour); Serial.print(":"); Serial.print(rtc_time.Minute);Serial.print(":"); Serial.println(rtc_time.Second);
  String now;
  String secs;
  String mins;
  int minutes=rtc_time.Minute;
  int seconds=rtc_time.Second;
  if(seconds<10) secs ="0"+(String)seconds;
  else secs = (String)seconds;
  
  if(minutes<10) mins = "0"+(String)minutes;
  else mins = (String)minutes;

  writeFile.println("Before string assignment");
  now = (String)rtc_time.Hour+":"+mins+":"+secs;
  writeFile.println(now);
  writeFile.println("Leaving write time, next line is writefile.flush");
  writeFile.flush();
}

void Log::printLog()
{
    if(!readFile) { readFile = SD.open(fileName); }
    String line;
    //Serial.print("filename: "); Serial.println(fileName);
    //Serial.print("result of file open: "); Serial.println(readFile);
    
    Serial.println();
    // read until empty
    while(readFile.available()) 
    {
        line = readLine(readFile);
        if(line.length() > 0) Serial.println((line));
    }
    readFile.close();
}

void Log::printLog(String name)
{
    strcpy(fileName, name.c_str());
    name.toCharArray(fileName, sizeof(fileName));
    File log = SD.open(fileName);
    String line;

    //Serial.print("filename: "); Serial.println(fileName);
    //Serial.print("result of file open: "); Serial.println(log);
    Serial.println();
    while(log.available()) // read until empty
    {
        line = readLine(log);
        if(line.length() > 0) Serial.println(line);
    }
    log.close();
}

String Log::readLine(File file) 
{
    int maximumLineLength = 128;
    char *lineBuffer = (char *)malloc(sizeof(char) * maximumLineLength);

    if (lineBuffer == NULL) { Serial.println("Error null buffer."); exit(1); }

    int ascii;
    int count = 0;
    while (file.available() && count<maximumLineLength) {
        ascii=file.read();    // gets decimal ascii code of character just read
        lineBuffer[count] = (char)ascii;
        if(ascii==10 || ascii==13) // 10 and 13 are ASCII code for end of line 
        {
          break;
        }
        count++;
    }

    lineBuffer[count] = '\0'; // terminates char array
    String line(lineBuffer);  // convert char* to string
    free(lineBuffer);         // free allocated memory
    return line;
}

void Log::printDirectory()
{
  Serial.println(F("SD Card Contents:")); Serial.println();
  Log::printDirRecursive(RootDir, 1);
}
void Log::printDirectory(File dir)
{
  Serial.println(F("SD Card Contents:")); Serial.println();
  Log::printDirRecursive(dir, 1);
}

void Log::printDirRecursive(File dir, int numTabs) 
{
   String file_name;
   while(true) 
   {
      File entry =  dir.openNextFile();
      if(!entry){ break; }
      
      for(uint8_t i=0; i<numTabs; i++){ Serial.print('\t'); }
      file_name =entry.name();
      Serial.print(entry.name());
      if(entry.isDirectory()) 
      {
        Serial.println(F("/"));
        printDirRecursive(entry, numTabs+1);
      } 
      else 
      {
       // print file size
       // check and see if contains "Log" and "18" (the year), if so increment count to parsenum+
          if(file_name.indexOf("Log")!=-1 && file_name.indexOf("18")!=-1)
          {
              char ch = file_name.charAt(1);
              int number = (int)ch-48;
              if(number >= Log_Number)
              Log_Number = number+1;
          }
          Serial.print(F("\t"));  // tab
          Serial.println(entry.size(), DEC);
      }
      entry.close();
   }
}

void Log::print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.print('0');  // digit zero
  }
  Serial.print(number);
}

void Log::Parse_Day_Log(String file)
{
  int index_dash, index_dot, index_s, index_asterisk, index_colon, index_time;
  int current_cycle =0;
  int cycle_count=0;
  int total_running_time = 0; // (seconds)
  int start_time = 0;
  String line;
  int count_this_run;
  int last_total=0;
  
  strcpy(fileName, file.c_str());
  file.toCharArray(fileName, sizeof(fileName));
  File day_log = SD.open(fileName);
  String first_line = Log::readLine(day_log);

  Serial.print(F("PARSING: ")); Serial.println(fileName);
  parse_log.date=parseDate(first_line);  // Parsing top line containing date
  //Serial.print("PARSE DATE: "); Serial.println(parse_log.date);
  parse_log.count = 0;
  count_last_run=0;
  parse_log.start_time=0;
  parse_log.running_time = 0;
  parse_log.last_timestamp = 0;
  
  skipLine(); // Discard line containing Cycle Time, Start(degs), End(degs), etc. 
  skipLine();   // Discard Line Containing "-------------------"

  // while available, parse entry
  while(day_log.available())
  {
      line=Log::readLine(day_log);
      if(line.length()>0)
      {
          Log::parseLine(line);
          Serial.println(line);
      }
  }
  Serial.print(F("\t[Count Last run: ")); Serial.print(parse_log.count-count_last_run); Serial.print("]");
  Calc_Running_Time();
  Serial.print(F("\t[Total Count: ")); Serial.print(parse_log.count); Serial.print("]"); Serial.println("\n");
  day_log.close();
}

void Log::parseLine(String line)
{
     //Serial.print(line);
     //Serial.print("\t\t[parse_log.count: "); Serial.print(parse_log.count); Serial.println("]");
      if(line.length() > 0)
      { 
        if((line.indexOf("Start: ")!=-1) || line.indexOf("Start ")!=-1)  // Contains "Start"
        {
          // only calculate running time if this isnt the start of the file:
          if(parse_log.count!=0) { Serial.print(F("\t[Count Last run: ")); Serial.print(parse_log.count-count_last_run); Serial.print("]"); 
          count_last_run = parse_log.count;}
          if(parse_log.start_time!=0) { Calc_Running_Time(); }  // calculate time for last run, a line with start means it was stopped for awhile
          parse_log.start_time=Log::parseStartTime(line);
        }
        if(line.startsWith("C"))  // is a cycle
        {
            Log::parse_Cycle_Entry(line);
            time_t diff = parse_log.last_timestamp-parse_log.start_time;
            //Serial.print(F("Time Last entry: ")); Serial.print(hour(diff)); Serial.print(":"); print2digits(minute(diff)); Serial.print(":"); print2digits(second(diff));Serial.println(); 
        }
        if(line.startsWith("-----")) // discard
        { /*Dont do anything, skip this line*/ }
      }
}

void Log::Calc_Running_Time()
{
    time_t diff = parse_log.last_timestamp-parse_log.start_time;
    Serial.print(F("\t[Time Last run: ")); Serial.print(hour(diff)); Serial.print(":"); print2digits(minute(diff)); Serial.print(":"); print2digits(second(diff)); 
    parse_log.running_time+=(parse_log.last_timestamp-parse_log.start_time);
    Serial.print(F(", Total running time: ")); print2digits(hour(parse_log.running_time)); Serial.print(":"); print2digits(minute(parse_log.running_time)); Serial.print(":"); print2digits(second(parse_log.running_time)); 
    Serial.println("]");
}

void Log::parse_Cycle_Entry(String entry)
{
    int number = Log::parse_Cycle_Number(entry);  // Extract cycle number from CXXX
    if(number>0){ parse_log.count++; }
    Log::parse_Timestamp(entry);     // Extract timestamp at end of line
}


int Log::parse_Cycle_Number(String line)
{
    String cycle;
    // Parse the cycle
    int index_time = line.indexOf("time:");
    String parse_string = line.substring(1, index_time);
   // Serial.print("parse string: "); Serial.println(parse_string);
    for(int i=0;i<parse_string.length(); i++)
    {
      if(isDigit(parse_string[i])){ cycle+=parse_string.charAt(i); }
    }
    int current_cycle = cycle.toInt();
    //Serial.print("Parsed Cycle: "); Serial.println(current_cycle);
    return current_cycle;
}

void Log::parse_Timestamp(String line)
{
    tm tmstamp;
    time_t timestamp;
    int hour, minute, second;
    int index_2;
    
    int index = line.indexOf("/sec");
    String trimmed = line.substring(index); // "/sec" until end of line
    index+=1;
    String parse_time = trimmed;

    index_2 = parse_time.indexOf(":"); // looking for next occurrence of ":", then read before that which is hours
    String str_hour = parse_time.substring(1, index_2);
    String digits;
    for(int i=0;i<str_hour.length(); i++)
    {
      if(isDigit(str_hour[i])){ digits+=str_hour.charAt(i); }  // extract only digits from resulting substring
    }
    str_hour = digits;
    hour=str_hour.toInt();
    
    index = index_2+1;  // advance so we dont include anything already used

    index_2 = parse_time.indexOf(":", index); // looking for next occurrence of ":", seconds
    String str_min = parse_time.substring(index,index_2);
    minute = str_min.toInt();

    String str_sec = parse_time.substring(index_2+1);
    second = str_sec.toInt();
    //Serial.print("str sec: "); Serial.println(second);
    //Serial.println(".................");

   tmstamp.tm_year=year(parse_log.date);
   tmstamp.tm_mon=month(parse_log.date);
   tmstamp.tm_mday=day(parse_log.date);
   tmstamp.tm_hour=hour; 
   tmstamp.tm_min=minute;
   tmstamp.tm_sec=second;
   parse_log.last_timestamp = mktime(&tmstamp);  // convert struct tm to time_t, store to member variable
}

time_t Log::parseDate(String str)
{
    tm start;
    time_t date_t;
    String month, day, year;
    
    // First parse the date  
    String line = str;
    int index_dot=line.indexOf(".");
    String date=line.substring(0, index_dot);
    //Serial.print("Parsed date: "); Serial.println(date);

    int index=date.indexOf("-");
    String str_mo = date.substring(0, index); // month is before "-" 
    //Serial.print("month: "); Serial.println(str_mo);
    index+=1;


    int index_2 = date.indexOf("-", index); // looking for next occurrence of "-", then read before that which is day
    String str_day = date.substring(index, index_2);
    //Serial.print("str day: "); Serial.println(str_day);
    index_2+=1;

    String str_yr = date.substring(index_2);
    //Serial.print("str yr: "); Serial.println(str_yr);
    //Serial.println(F("................."));

   start.tm_year=str_yr.toInt();
   start.tm_mon=str_mo.toInt();
   start.tm_mday=str_day.toInt();
   start.tm_hour=0;
   start.tm_min=0;
   start.tm_sec=0;
   date_t = mktime(&start);  // convert struct tm to time_t
   parse_log.date = date_t;

   return date_t;
}

void Log::skipLine()
{
  //while(!readFile.available()){}
  String line = readLine(readFile); // consumes a line and moves to next line
  //Serial.print(F("Discarding: ")); Serial.println(line);
}

time_t Log::parseStartTime(String line)
{
    tm start;
    time_t start_t;
    int hour, minute, second;
    int index_2;
    int index=line.indexOf(":");
    String parse_time = line.substring(index+1); // Removing "Start:" 
    
    index=parse_time.indexOf(":");
    String str_hour = parse_time.substring(1, index); // hour is before ":" 
    hour = str_hour.toInt();
    //Serial.print("str hour: "); Serial.println(str_hour);
    index+=1;


    index_2 = parse_time.indexOf(":", index); // looking for next occurrence of ":", then read before that which is minutes
    String str_min = parse_time.substring(index, index_2);
    minute = str_min.toInt();
    //Serial.print("str min: "); Serial.println(str_min);
    index = index_2;

    index_2 = parse_time.indexOf(":", index); // looking for next occurrence of ":", seconds
    String str_sec = parse_time.substring(index_2+1);
    second=str_sec.toInt();
    //Serial.print("str sec: "); Serial.println(str_sec);


    //start.setTime(hour, minute, second, day(parse_log.date), month(parse_log.date), year(parse_log.date));
   start.tm_year=year(parse_log.date);
   start.tm_mon=month(parse_log.date);
   start.tm_mday=day(parse_log.date);
   start.tm_hour=hour;
   start.tm_min=minute;
   start.tm_sec=second;
   start_t = mktime(&start);
   parse_log.start_time=start_t;
   //Serial.println(F("................."));

    return start_t;
}
