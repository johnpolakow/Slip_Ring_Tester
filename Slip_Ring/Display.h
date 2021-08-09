//
//  Functions and Definitions for SLIP_RING 
//  
#include "Images/BlueMarlin3.h"
#include "Images/degree2.h"
#include "Images/Base5.h"
#include "ILI9341_t3n.h"
#include <ili9341_t3n_font_ArialBold.h>
//#include "Log.h"

//Color Definitions
#define COUNT_COLOR tft.color565(25, 255, 25)
#define BEAM_COLOR tft.color565(237, 232, 34)
#define TIME_COLOR tft.color565(240, 250, 34)
#define PAUSE_COLOR tft.color565(255, 30, 15)
#define ERASE tft.color565(0,0,0)
#define UP_COLOR tft.color565(0,160,30)
#define DEG2RAD .0174532925 // converting degrees to radians
#define FOV 30      // Field of View (for rangefinder)
#define RADIUS 31   // radius of triangle
#define INDENT 10   // indent 10 pixels for text

struct point{
  int x;
  int y;
};

struct vertices{
  int x0 = 93;    // x position of base of yellow triangle
  int y0 = 194;   // y position of base of yellow triangle
  int x1;
  int y1;         // other two vertices of the triangle
  int x2;
  int y2;
};

class Display
{
  public:
    Display();

    
    void begin();
    void initialize();
    void updateTime(unsigned long ms_running_time);
    void update_Cycles(int count);
    void printTemplate();
    void printPower();
    void printDegreeSymbol(int xpos, int ypos);
    void printNavText();
    void drawNavIcon();
    void drawMarlin();
    void update_Icon(int oldIndex, int newIndex);
    void update_Angle(int angle);
    void update_Speed(int ang_velocity);
    void show_Pause();
    void Refresh();
    float degrees2radians(int angle){ return (3.14159/180)*angle; }
    void calcTriangleVertices();
};
