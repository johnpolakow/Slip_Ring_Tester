#include "Display.h"

// Pin Definitions
#define TFT_DC  10  // pin on microcontroller wired to D/C pin on TFT display. Must be cable of SPI chip select functionality
#define TFT_CS 9    // Chip Select Pin used microcontroller to display ( must be capable of SPI functionality)
#define TFT_RESET 6 // Reset Pin for display. Any digital out pin on microcontroller can be used

uint16_t Nav_pixel_data[4225]; // Allocating memory to store partial screen snapshot. Make the size of rectangle you are storing (pixel width*pixel height)
uint16_t Marlin_top_pdata[2000]; // memory allocation for snapshot. declare to size of the rectangle you are storing (pixel width*pixel height)
uint16_t Marlin_time_pdata[1600]; // this is memory allocatino for partial snapshot of Marlin image that is overdrawn by seconds on tft display

// Declare instance of TFT display library.
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RESET);

point origin = {93, 194}; // Fixed Origin of rangefinder icon triangle
struct vertices triangles [20]; // holds calculated vertices of icon positions

Display::Display(){}    // constructor

void Display::begin()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLUE); // Loading Screen
  tft.setTextColor(ILI9341_YELLOW);
  Display::initialize(); // Initialize tft Display
}

void Display::initialize()
{
    // DISPLAY SETUP
  tft.useFrameBuffer(true);   // use direct memory access to refresh screen quickly
  calcTriangleVertices();     // calculate points for drawing the triangle icon in various positions
  tft.setRotation(3);   // Set orientation of display
  tft.fillScreen(ILI9341_BLACK);    // clear screen
  delay(10);    // delay 10 ms
  Display::drawMarlin();
  Display::drawNavIcon();
  Display::printTemplate();
  Display::printNavText();
  Display::printDegreeSymbol(157, 57);   //degree symbol for position
  Display::printDegreeSymbol(217, 47);   // degree symbol for speed
  Display::printPower();
  tft.updateScreen();     // display everything before capturing snapshots
  
  // Take snapshots of drawn icons, Stores pixel data for redraw for when text overwrites the icons
  tft.readRect(60, 170, 65, 25, Nav_pixel_data); //stores screenshot of boat icon
  tft.readRect(135, 0, 100, 20, Marlin_top_pdata); // store screenshot of top of Marlin
  tft.readRect(165, 30, 100, 16, Marlin_time_pdata);
}

void Display::updateTime(unsigned long ms_running_time)
{
  int seconds = ms_running_time/1000; // ms_running time is milliseconds argument provide by caller. Is time since program execution started
  int minutes = seconds/60;
  int hours = minutes/60;
  int days = hours/24;
  int months = days/30;
  
  tft.setClipRect(INDENT, 30, 235, 15);   // limit refresh region to only the time section
  tft.fillRect(INDENT, 30, 200, 20, ILI9341_BLACK);
  tft.writeRect(165, 30, 100, 16, Marlin_time_pdata); // redraw erased part of Marlin image
  tft.setCursor(INDENT, 32);
  tft.setFont(Arial_12_Bold);
  tft.setTextColor(TIME_COLOR);
  tft.print(months); tft.print("M ");
  tft.print(days); tft.print("D ");
  tft.print(hours); tft.print("H ");
  tft.print(minutes%60); tft.print("min ");
  tft.print(seconds%60); tft.print("sec ");
  tft.updateScreen();   // redraws the changes
}

void Display::update_Cycles(int count)
{
  tft.setClipRect(INDENT+75,5,150,15);  // limit screen redraw to cycle count
  tft.fillRect(INDENT+75, 5, 230, 15, ILI9341_BLACK); // erase old count
  tft.writeRect(135, 0, 100, 20, Marlin_top_pdata);   // draw part of Marlin that was erased
  tft.setTextColor(COUNT_COLOR);
  tft.setFont(Arial_14_Bold);
  tft.setCursor(INDENT+75, 5);
  tft.println(count);
  tft.updateScreen();
}

void Display::printTemplate()
{
  tft.setTextColor(tft.color565(255,255,255));  
  tft.setFont(Arial_14_Bold);
  tft.setCursor(INDENT, 5);
  tft.println("Cycles: ");

  tft.setFont(Arial_12_Bold);
  tft.setCursor(INDENT+13, 80);
  //tft.print("MX Serial: ");
  tft.setTextColor(UP_COLOR);
  //tft.print("UP");
  
  tft.setTextColor(tft.color565(255,255,255));  
  tft.setCursor(INDENT, 100);
  tft.print("MCU Serial: ");
  tft.setTextColor(UP_COLOR);
  tft.print("UP");
  
  tft.setTextColor(tft.color565(255,255,255));
  tft.setCursor(INDENT+18, 120);
  //tft.print("Ethernet: ");
  tft.setTextColor(UP_COLOR);
  //tft.print("UP");
}

// Placeholder for live updated information about power consumption
void Display::printPower()
{
  tft.setFont(Arial_12_Bold);
  tft.setCursor(INDENT, 55);
  tft.println("48V / 1.5A / 72W");
}


//Coordinates to print degree superscript symbol
void Display::printDegreeSymbol(int xpos, int ypos)
{
   int h = 9;
   int w = 7;
   int buffidx=0;
  for (int row=0+xpos; row<h+xpos; row++) { // For each scanline...
    for (int col=0+ypos; col<w+ypos; col++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required.
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col, row, pgm_read_word(degree2 + buffidx));
      buffidx++;
    } // end pixel
  }
}

// Print Angle and Angular Velocity
void Display::printNavText()
{
    tft.setFont(Arial_12_Bold);
    tft.setTextColor(tft.color565(255,255,255));  // setting color to white
    tft.setCursor(40, 160);
    tft.print("90");  // inital value, will be overwritten by screen updates
    tft.setCursor(30, 220);
    tft.print("30 /s"); // inital value, will be overwritten by screen updates
}

void Display::drawNavIcon()
{
  int h = 65;   // height of image to draw
  int w = 65;   // width of image to draw
  int xpos = 170; // position offset from left side of screen
  int ypos = 60;  // position offset from top of screen
  int buffidx=0;  //index to read from pixel data array
  for (int row=0+xpos; row<h+xpos; row++) { // For each scanline...
    for (int col=0+ypos; col<w+ypos; col++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required. (image data is stored in flash by specific directives)
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col, row, pgm_read_word(Base5 + buffidx));
      buffidx++; // go to next index in pixel array Base5, and read the 16 bit value there
    } // end pixel
  }
}

void Display::drawMarlin()
{
  int height = 264,width = 200, row, column, buffidx=0;
  for (row=0; row<height; row++) { // For each scanline...
    for (column=0+135; column<width+135; column++) { // 135 is offset to begin drawing
      //To read from Flash Memory, pgm_read_XXX is required. (image data is stored in flash by specific directives)
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(column, row, pgm_read_word(BlueMarlin3 + buffidx));
      buffidx++; // go to next index in pixel array Base5, and read the 16 bit value there
    } 
  }
}

void Display::update_Icon(int oldIndex, int newIndex)
{
    vertices v_cur = triangles[oldIndex];             // get icon coordinates associated with currently drawn triangle
    vertices v_next = triangles[newIndex];            // icon coordinates for new triangle

    // Now update rangefinder icon 
    tft.setClipRect(65,160,60,35);  // limit screen update area to the triangle. Makes refresh much faster
    tft.fillTriangle(origin.x, origin.y, origin.x + v_cur.x1, origin.y - v_cur.y1, origin.x + v_cur.x2, origin.y - v_cur.y2, ERASE); // erase old icon position
    tft.writeRect(60, 170, 65, 25, Nav_pixel_data); // redraw erased sections of background
    tft.fillTriangle(origin.x, origin.y, origin.x + v_next.x1, origin.y - v_next.y1, origin.x + v_next.x2, origin.y - v_next.y2, BEAM_COLOR); // draw new triangle position
    tft.updateScreen();
}

void Display::update_Angle(int angle)
{

    int offset = 0; // x offset
    if(angle<10){ offset = 8; } // draw more to the right if value only has a single digit (keeps it next to degree superscript)
    tft.setFont(Arial_12_Bold);
    tft.setClipRect(30,160,70,55);
    tft.fillRect(40,160, 30, 13, ILI9341_BLACK);  // erase previous value
    printDegreeSymbol(157, 57);     // degree superscript
    tft.setTextColor(ILI9341_WHITE); // always set text color before writing. Could have been changed by previous function
    tft.setCursor(40 + offset, 160);
    tft.print(angle);   // angle value of current motor base position
    tft.updateScreen();
}

void Display::update_Speed(int ang_velocity)
{
    int offset = 0;
    if(ang_velocity<10){ offset = 8; }
    tft.setClipRect(20,220,60,35);
    tft.fillRect(30,220, 17, 13, ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setFont(Arial_12_Bold);
    tft.setCursor(30 + offset, 220);
    tft.print(ang_velocity);
    tft.updateScreen();
}

void Display::show_Pause()
{
  tft.setClipRect(INDENT+60,70,200,200);  // limit screen redraw to cycle count
  tft.setTextColor(PAUSE_COLOR);    // red color, value is #defined
  tft.setFont(Arial_40_Bold);
  tft.setCursor(INDENT+60, 70);
  tft.println("PAUSE");
  tft.updateScreen();
}

void Display::Refresh()
{
  tft.setClipRect(INDENT+60,70,200,200);  // limit screen redraw to cycle count
  tft.fillRect(INDENT+60,70,200,200, ILI9341_BLACK);  // erase "PAUSE" portion
  Display::drawMarlin();
  Display::printTemplate();
  Display::printPower();
  tft.updateScreen();
}


// Calculates the triangle vertext points for the 18 different triangle icon positions
// cant remember the exact algorithm I came up with. If using sin or cos functions be aware
// it expects arguments in radians, not degrees. The origin is fixed, only the two outer vertices of the triangle change with position
void Display::calcTriangleVertices()
{
  float xvar1, yvar1, xvar2, yvar2;
  int degrees2;
  float radians;  // angle of one vertex point in radians
  float radians2; // angle of second vertex point in radians

  for(int degrees=30; degrees<=120;degrees+=5)
  {
      degrees2 = degrees+FOV; // Field of view is "width" of triangle rangefinder beam in degrees
      radians = (3.14159/180)*degrees;
      radians2 = (3.14159/180)*degrees2;  // converting degrees to radians
      xvar1 = round(RADIUS*cos(radians));
      yvar1 = round(RADIUS*sin(radians));
      xvar2 = round(RADIUS*cos(radians2));
      yvar2 = round(RADIUS*sin(radians2));
      triangles[(degrees-30)/5].x1=xvar1;   // dividing by 5 to give icon positions in range 1-18
      triangles[(degrees-30)/5].y1=yvar1;
      triangles[(degrees-30)/5].x2=xvar2;
      triangles[(degrees-30)/5].y2=yvar2;
        }
}

