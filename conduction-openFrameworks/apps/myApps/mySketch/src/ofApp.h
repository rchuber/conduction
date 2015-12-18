#pragma once

#include "ofMain.h"
#include "ofxBvh.h"
#include "ofxCsv.h"
#include "ofxGui.h"

using namespace wng;

class testApp : public ofBaseApp
{
    
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ofSoundPlayer player;
    ofEasyCam cam;
    
    ofxCsv csv;
    ofxCsv csvRecorder;
    
    bool bHide;
    ofxVec3Slider sliderRotation;
    
    ofxFloatSlider  val1;
    ofxFloatSlider  val2;
    ofxFloatSlider  val3;
    ofxFloatSlider  val4;
    ofxFloatSlider  val5;
    ofxFloatSlider  val6;
    ofxFloatSlider  val7;
    ofxFloatSlider  val8;
    ofxFloatSlider  val9;
    ofxFloatSlider  max_point_size;
    ofxFloatSlider  min_point_size;
    ofxFloatSlider  attenuationx;
    ofxFloatSlider  attenuationy;
    ofxFloatSlider  attenuationz;

    
    ofxFloatSlider  rotationX;
    ofxColorSlider color;
    ofxVec2Slider center;
    ofxIntSlider circleResolution;
    ofxToggle filled;
    ofxButton twoCircles;
    ofxButton ringButton;
    ofxLabel screenSize;
    
    ofxPanel gui;
    
};