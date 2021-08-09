
/*
TODO:
take serial input and print to MCU
basic console input commands to program
watchdog and reboot from raspberry pi
pulling files from teensy onto raspberry pi
schematics of entire setup
document individual components
how time is documented
filenames limited to 8 characters
explain F macro
explain time delays
parse logs
explain real time clock and associated libraries
explain drawing library
explain tft.color565
explain dma and screen updates
screen coordinates
how to convert images to pixel header files
explain setClipRect
fonts and conversion
flushing and closing log
cutting usb power line
get clock time from Pi 

*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#include "Log.h"
#include "Display.h"
using namespace std;

#ifdef __arm__
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

// set this to the hardware serial port on teensy you wish to use
#define MCU Serial1

#define POINT_A  25500
#define POINT_B  34500
#define NUM_POSITIONS 18  // Numer of triangle icon positions on tft display
#define PAUSE_BUTTON  20  // Button connected on Teensy pin #20. Pushing button==connecting to 3.3V, triggers interrupt
#define MAX_RESPONSE_TIME 500 //milliseconds

Log* cycle_log; 
bool write_SD_card = false;
bool receivedResponse;
String completeResponse = "";
int maximumLineLength = 128;
char *lineBuffer = (char *)malloc(sizeof(char) * maximumLineLength);  // dynamic memory allocation to store string of line read

unsigned long startm=0;
unsigned long finishm=0;
unsigned long timeout =0;

int Marlin_Position=0;  // last parsed position from MCU, in degrees, 0-90
int New_Position=1;     // most recent parsed position, in degrees, 0-90
int Marlin_Speed=0;     // last parsed speed of motor base in degrees/sec
int New_Speed=0;        // most recent parsed speed of motor base in degrees/sec
int Marlin_Index = 0;   // corresponding index 1-18 of motor base angle 0-90
int New_Index = 0;      // most recent corresponding index

//flags set by interrupt service routines
volatile bool PAUSE_MOTION = false;
volatile bool UNPAUSE = false;
volatile boolean pause_flag;
volatile boolean pause_ON;
volatile unsigned long pause_time;

String Date;

unsigned long start_millis;   // milliseconds since program began running during this power on session
int Set_Speed = 3000; // max speed for motor base to move, degrees per second
int Cycle_Count = 0;    // For current logging session


boolean DEBUG = false;  // whether to print messages to console
boolean LOGGING = true;
boolean MEM_DEBUG = false;
int MEM_AVAILABLE =0;

Display display = Display();  // instance of class that controls TFT display

void setup() {
  display.begin();
	Serial.begin(38400);
	MCU.begin(38400, SERIAL_8N1);   // Settings required for Marlin MCU- 38400 baud
  
  while(!Serial){}  // waits for serial console to come up before beginning execution
  
  pinMode(PAUSE_BUTTON, INPUT_PULLDOWN); // input and output pins must be declared in setup
  
  //On Teensy 3.x, interrupt 0 maps to pin 0, interrupt 1 maps to pin 1, and so on
  // Setup Interrupts, possible modes: RISING/HIGH/CHANGE/LOW/FALLING
  attachInterrupt (PAUSE_BUTTON, ISR_PAUSE, RISING);  // attach PAUSE_BUTTON interrupt handler to function ISR_PAUSE, trigger on rising pulse

  if(LOGGING) cycle_log = new Log();  // Setup log for motor base movements

  Serial.print("Initial Memory Available: "); Serial.println(freeMemory());
  if(LOGGING)cycle_log->printLog();  
  Cycle_Count+=54204;
  start_millis = millis();    // Start time of program execution
}

void loop() {
  Check_Interrupts(); // see if any interrupt flags have been set, if so handle them
  updateDisplay();
  //MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("Main Loop", MEM_AVAILABLE);
  // conditional statement to print to file and console if memory drops below a certain point
  
  int incomingByte;
  char c_usb;

	if (Serial.available() > 0) {
	  incomingByte = Serial.read();
    c_usb = incomingByte;
    if(incomingByte != 0)
    {
        Serial.print(c_usb);
        MCU.print(c_usb);
    }
	}

  if(!PAUSE_MOTION)
  {
      if(!At_Start_Position()){ Go_To_Start(); }
      Do_Cycle();
      delay(10);  // delay 15 ms
  }
  if(UNPAUSE)
  {
    display.Refresh();  // remove "PAUSE" message from display
    display.update_Angle(New_Position);   // Update Angle Position on Display
    display.update_Speed(New_Speed);  
    UNPAUSE=false;      // clear the flag
    Serial.println(F("UNPAUSED"));
  }
  delay(5);
}

void Check_Interrupts()
{
  if(PAUSE_MOTION && !pause_ON) // PAUSE_MOTION means pause button has been pressed. , pause_ON is message shown on display
  {
    int t1 = millis();
    display.show_Pause(); // Show "PAUSE" on TFT display
    Serial.print(F("PAUSE TIME: ")); Serial.println(millis()-t1);
    pause_ON = true;  // message has been shown
  }
  if(UNPAUSE)
  {
    display.Refresh();  // remove "PAUSE" message from display
    display.update_Angle(New_Position);   // Update Angle Position on Display
    display.update_Speed(New_Speed);  
    UNPAUSE=false;      // clear the flag
  }
}
// updating time on display, do frequently to accurately update seconds
void updateDisplay()
{
    unsigned long running_time = millis()-start_millis;
    display.updateTime(running_time);
}

// // ******* Interrupt Service Routine  ********
// NOTE: millis() does not increment/update within interrupt service routines
// Interrupt Routines should not do work, just set flags to avoid blocking other functions
// This function is triggered when the pause button is pressed
void ISR_PAUSE ()
{
  if((millis()-pause_time)>500)  // Mandating .5 second delay before changing state to account for debouncing of switch
  {
      if(PAUSE_MOTION==true)  // unpause if button has been pressed again
      {
        PAUSE_MOTION = false;
        pause_ON = false;
        UNPAUSE = true;
        Serial.println(F("UNPAUSE"));
      }
      else  // set pause if button has been pressed first time
      {
          PAUSE_MOTION = true;
          Serial.println(F("PAUSE"));
      }
      pause_time=millis();
  }
}  // ******* end of Interrupt Service Routine  ********

// An example of the string received is: {ma:{x:{a:34456,s:649}
float Get_Position()  // Returns a float value of current position in degrees 0-90
{
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("Get Position", MEM_AVAILABLE); }
    int index_x, index_a, index_s, index_comma, index_colon, index_bracket; // indexes in parse string
    String parse_string;
    String position;
    String line;
    
    SEND_MCU_POSITION_QUERY();  // sends "?ma.x" to MCU, a position query
    // Serial.println("Get position: sent query");

    int start_time = millis();
    receivedResponse = false;    
    while(receivedResponse!=true && ((millis()-start_time)<MAX_RESPONSE_TIME))
    {
          //cycle_log->write_String("Get Position, in while loop"); 
          //cycle_log->write_String(completeResponse);
      if(MCU.available()>0)
      {
        receivedResponse=readMCU();
        delay(5); // delay necessary for stable loop operation
      }
    }
    
    Serial.print("Received response from Get_Position(). Response is: ");
    Serial.println(completeResponse);
    updateDisplay();
    line = completeResponse;
    cycle_log->write_String("GetPos, response: "); cycle_log->write_String(line);
    //  {ma:{x:{a:25499,s:0}}    an example of what is stored in line
    if(!(line.length()>=1)){ return 0; }
    index_x= line.lastIndexOf('x');
    parse_string = line.substring(index_x); // x to end of line. Now is: x:{a:25499,s:0}}
    index_a = parse_string.indexOf('a');  // position of a
    index_colon = parse_string.indexOf(':', index_a); // looking for colon after position of 'a' (colon is right before value we want: 25499)
    index_comma = parse_string.indexOf(',');  // position of comma before 's'
    position = parse_string.substring(index_colon+1, index_comma);  // getting the string representation of 25499
    completeResponse = "";
    float pos=((position.toFloat()) - POINT_A)/100; // converting value to float degrees, subtracting POINT_A gives a range 0-90. (MCU sends results in hundreths of a degree)
   // Serial.print("GETPOS: "); Serial.println(position);
   float copy = pos;
    cycle_log->write_String("Get Position, float pos"); 
    cycle_log->write_String((String)copy);
    updateDisplay();
    Check_Interrupts();
   // Serial.println("Exiting Get Position");
    return pos;

// Return Whether Motor Base Position is at Point A ( 0 degrees )
}
// Must be at POINT_A to begin a cycle
bool At_Start_Position()
{
    //cycle_log->write_String("In Start Position Function");
   // Serial.println("At start position");
    SEND_MCU_POSITION_QUERY();
    RECEIVE_QUERY_RESPONSE();
    Check_Interrupts();
    if(!At_PointA() ){ Go_To_PointA(); }

    return At_PointA();
}

void Go_To_Start(){ Go_To_PointA(); }

void Go_To_PointA()
{
    cycle_log->write_Mem_Avail("GO TO POINT A", MEM_AVAILABLE); }
    //Serial.print("Go to Point A");
    updateDisplay();
    Check_Interrupts();
    delay(5);
    SEND_MCU_MOVE_COMMAND(POINT_A); // Sending command '?ma.x.a=2550,s=3000'
    delay(5);
    RECEIVE_MCU_RESPONSE();         // confirm MCU received the command
    
    timeout = millis();   // timeout in case MCU goes to wrong destination or hiccups. transit time shouldnt take more than 3 seconds
    while(!At_PointA() )  // this loop gets position and speed updates while in transit to POINT_A
    {
      SEND_MCU_POSITION_QUERY();
      RECEIVE_QUERY_RESPONSE();
      Check_Interrupts(); // only check interrupts after receiving a response. Handling interrupts can take 20-30ms
      
      cycle_log->write_String("In point A loop, not at point A yet"); 
      if(!At_PointA() && Marlin_Speed==0 && (millis()-timeout)>3500){ cycle_log->write_String("*****Timeout in A Loop ******"); return; } // Occassionally MCU will overshoot destination and lock up control loop, timeout breaks out of loop
    }
}

void Go_To_PointB()
{
    
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("GO TO POINT B", MEM_AVAILABLE); }
    //Serial.println("Go to point B");
    updateDisplay();
    Check_Interrupts();
    delay(5);
    //cycle_log->write_String("Sending move command");
    SEND_MCU_MOVE_COMMAND(POINT_B); // Sending command '?ma.x.a=34500,s=3000'
    //cycle_log->write_String("move command has been sent");
    RECEIVE_MCU_RESPONSE();         // confirm MCU received the command

    timeout=millis();               // timeout in case MCU goes to wrong destination or hiccups
    while(!At_PointB() )
    {
      SEND_MCU_POSITION_QUERY();
      RECEIVE_QUERY_RESPONSE();
      Check_Interrupts(); // only check interrupts after receiving a response. Handling interrupts can take 20-30ms
      
      cycle_log->write_String("In point B loop"); 
      if(!At_PointB() && Marlin_Speed==0 && ((millis()-timeout)>3500)){ // will breakout of loop after 3.5 seconds
        Serial.println(F("timeout"));
        cycle_log->write_String("*****Timeout in B Loop ******"); 
        return; } // Occassionally MCU will overshoot destination and lockup control loop, timeout fixes that
    }
    //Serial.println("arrived B");
}

void SEND_MCU_POSITION_QUERY(){ delay(5); MCU.println("?ma.x"); }

void RECEIVE_QUERY_RESPONSE()
{
 // Serial.println("Entering receive");
 
    int start_time = millis();
    receivedResponse = false;
    while(receivedResponse!=true && ((millis()-start_time)<MAX_RESPONSE_TIME))
  {
    if(MCU.available()>0){ receivedResponse=readMCU(); }  // continues to grab characters until end of line is reached. (Meaning end of MCU response)
    delay(10); // delay necessary for stable loop operation
    
      cycle_log->write_String("In receive query loop, completeResponse so far: "); 
      cycle_log->write_String(completeResponse); 
  }
  updateDisplay();
  Check_Interrupts();
  parsePosition(completeResponse);  // the character read from readMCU() are appended to member variable completeResponse
  if(DEBUG){ Serial.println(F("QUERY RESPONSE: "));Serial.print(completeResponse); }
  else delay (15);
  completeResponse = "";  // clear the line after parsing it

  //Serial.println("Leaving Receive");
}

void SEND_MCU_MOVE_COMMAND(int destination)
{
    cycle_log->write_String("Send move command, destination: ");
    cycle_log->write_String((String)destination); 
    delay(5);
    String send;
    send = "?ma.x.a=";
    send.append(destination);
    send.append(",s=");
    send.append(Set_Speed);
    //Serial.print(F("destination: ")); Serial.println(destination);
    //Serial.print(F("MOVE COMMAND: ")); Serial.println(send);
    MCU.println(send);  // writes the command to serial out
    MCU.flush();
}

void RECEIVE_MCU_RESPONSE()
{
    receivedResponse = false;
    while(receivedResponse!=true)
    {
      if(MCU.available()>0)
      {
        receivedResponse=readMCU();
        delay(10); // delay necessary for stable loop operation
      }  
      
      cycle_log->write_String("In receive mcu response loop, receive response==");
      //cycle_log->write_String(receivedResponse); 
       cycle_log->write_String(completeResponse); 
    }
    updateDisplay();
    //Serial.print(F("COMMAND RESPONSE: "));Serial.println(completeResponse);
    parsePosition(completeResponse);
    completeResponse = "";  
    Check_Interrupts();
}

void Do_Cycle()
{
  
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("DO CYCLE", MEM_AVAILABLE); }
    //Serial.println("Do Cycle");
    updateDisplay();
    int time1 = millis(); // start time of the cycle
    float start_pos = Get_Position(); // gets beginning position of cycle
    Check_Interrupts();
    // We are already at Point A, so go to B
    Go_To_PointB();  
    float end_pos = Get_Position(); // end of cycle position (at B, then goes back to A)
    Go_To_PointA();   // Go back to A to complete cycle
    int loop_time = millis()-time1; // cycle time in milliseconds from start to finish
    
    Get_Position();
    Cycle_Count++;
    //cycle_log->write_String("Memory Available: ");
    //MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("Do Cycle", MEM_AVAILABLE);
    display.update_Cycles(Cycle_Count); // update the TFT display
    if(LOGGING) cycle_log->writeCycle(Cycle_Count, loop_time, start_pos, end_pos, Set_Speed/100); // write cycle metrics to file
    cycle_log->write_String((String)Cycle_Count + " " + (String)loop_time + " " + (String)start_pos + " " + (String)end_pos);
    //Serial.print(F("*** CYCLE COUNT: ")); Serial.print(Cycle_Count); Serial.println(F(" ***"));
    if((Cycle_Count%10)==0){ delay(400); } // briefly rest every 10 cycles
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail(" END DO CYCLE", MEM_AVAILABLE); }
}

// Extracts Position and Speed from string response to ?ma.x command
// Example string this function parses: {ma:{x:{a:34456,s:649},y:{a:9,s:0}}}
void parsePosition(String line)
{
    
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("parsePosition", MEM_AVAILABLE); }
    //Serial.println("In parsePosition()");
    int index_x, index_a, index_s, index_comma, index_colon, index_bracket;
    String parse_string;
    String position;
    String speed;
    //if(!(line.length()>=1)){ Serial.println("returning"); return; }  // return if string is empty
    //Serial.println("line.length"); Serial.println(line.length());
    
    if(line.indexOf("rror")!=-1)  // If the line contains 'rror' it means an error string was received so discard this line
    {
      Serial.println(F("Received 'Error wrong query' "));
      return;
    }
    //Serial.print("Parse position: ");Serial.println(line);  // string to parse
    index_x= line.lastIndexOf('x');
    parse_string = line.substring(index_x); // x to end of line, removing "ma"
    index_a = parse_string.indexOf('a');
    index_colon = parse_string.indexOf(':', index_a); // looking for colon after position of a
    index_comma = parse_string.indexOf(',');
    position = parse_string.substring(index_colon+1, index_comma);

    //Serial.print("first parse: "); Serial.println(position);
    index_s = parse_string.indexOf('s');
    index_colon = parse_string.indexOf(':', index_s); // looking for colon after s
    index_bracket = parse_string.indexOf('}', index_s); // looking for end bracket after s
    speed = parse_string.substring(index_colon+1, index_bracket);
    
    Marlin_Position = New_Position;
    New_Position=((position.toInt()) - POINT_A)/100; // converting to degrees, in the range 0-90. MCU sends results in hundreths of a degree
    //Serial.print("Parse function: new position: "); Serial.println(New_Position);
    Marlin_Speed=New_Speed;    
    New_Speed=(speed.toInt())/100;  // converting to degrees, MCU sends results in hundreths of a degree
    if(New_Speed<0){ New_Speed*=-1; } // speed is negative if rotating counterclockwise
     //cycle_log->write_String("ParsePosition"); 
      //cycle_log->write_String((String)New_Position+" "+(String)New_Speed); 
     if(New_Position==-255) New_Position=0;
    Check_Interrupts();
    if(New_Position > 90){ cycle_log->write_String("New Position is greater than 90"); }
    update_Icon();    // Update Rangefinder Icon
    cycle_log->write_String("New Position: ");
    cycle_log->write_String((String)New_Position);
    cycle_log->write_String("New Speed: ");
    cycle_log->write_String((String)New_Speed);
    display.update_Angle(New_Position);   // Update Angle Position on Display
    display.update_Speed(New_Speed);      // Update Speed on Display
    //Serial.println("Leaving parse position");
}

void update_Icon()  // updates the "triangle" rangefinder icon on the display
{
    cycle_log->write_String("UpdateIcon, oldPos NewPos ");
    int oldIndex = Position_to_Index(Marlin_Position);    // converting 0-90 degrees to rangefinder icon location of 1-18
    int newIndex = Position_to_Index(New_Position);          // new icon position
    cycle_log->write_String(Marlin_Position + " " + New_Position);
    display.update_Icon(oldIndex, newIndex);  // redraw the icon
}

// there are 18 possible drawing positions of the triangle icon, and a transit range of 0-90 degrees
// 90/18 = 5, so divide any angle by 5 to get its associated index of icon drawing position
int Position_to_Index(int angular_position)
{
    if(angular_position>=0 && angular_position<=90){ return angular_position/5; }
    else
    { // Serial.println(F("POSITION OUT OF RANGE"));
      return 0; }
}

bool At_PointA()  // At 0 degrees (or 25500 according to MCU)
{
  //Serial.print("At_PointA. Position: "); Serial.println(New_Position);
  if(-1<New_Position && New_Position<=1){ return true; }  // close enough if within 1 degree
  else { return false; }
}

bool At_PointB() // At 90 degrees (or 34500 according to MCU)
{
  //Serial.print("At_PointB(). Position: "); Serial.println(New_Position);
  if(89<=New_Position && New_Position<=91){ return true; }
  else { return false; }
}

// Returns true end of a line is read, which happens if reading character with ascii decimal value of 10 or 13
bool readMCU(){
    if(LOGGING && MEM_DEBUG){   MEM_AVAILABLE = freeMemory(); cycle_log->write_Mem_Avail("readMCU Start", MEM_AVAILABLE); }
    int ascii;
    bool endOfLine = false;
    if (lineBuffer == NULL) {
        Serial.print(F("Error null buffer."));
        exit(1);
    }

    char ch;
    int count = 0;
    if(startm==0){ startm=millis(); }
    while (MCU.available() && count<=maximumLineLength) {
        char ch = MCU.read();
        //Serial.print(ch);
        ascii = ch; // storing char byte to an int for ascii comparison
        if( (int)ch==13 )   // 10 and 13 are line feed and carriage return
        {   
              finishm = millis();
              endOfLine=true; 
              break; // End of Response has been reached if 13 is read
        }     
        lineBuffer[count] = ch;
        
        if(ch!='"') // Filtering out the quotes in response string
        {
          count++;  // only advance index if character read is not a quote, cleaning up the input string
        }
    }
    lineBuffer[count] = '\0'; // adding null terminator for c strings
    char line[count + 1];
    strncpy(line, lineBuffer, (count + 1));
     cycle_log->write_String("readMcu return value: ");
      cycle_log->write_String(lineBuffer);  
    completeResponse.append(line);
    
    return endOfLine; // boolean value
}

// This function used for debugging memory issues. Calling it returns the free memory left on the heap
int freeMemory() {
char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}



