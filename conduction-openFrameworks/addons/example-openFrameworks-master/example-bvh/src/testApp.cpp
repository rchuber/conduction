#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	ofSetFrameRate(120);
	ofSetVerticalSync(true);
	
	ofBackground(0);
	
	bvh.resize(1);
	
	// setup bvh
	//bvh[0].load("reexportedsong3-centered.bvh");
	//bvh[1].load("reexportedsong3-scaleddown.bvh");
	bvh[0].load("reexportedsong3-scaledwayup.bvh");
	
	for (int i = 0; i < bvh.size(); i++)
	{
		bvh[i].play();
		bvh[i].setLoop(true);
	}
}

//--------------------------------------------------------------
void testApp::update()
{
	for (int i = 0; i < bvh.size(); i++)
	{
		bvh[i].update();
	}
}

//--------------------------------------------------------------
void testApp::draw(){
	glEnable(GL_DEPTH_TEST);
	
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);

	cam.begin();
	
	for (int i = 0; i < bvh.size(); i++)
	{
		bvh[i].draw();
	}
	
	cam.end();

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 
}
