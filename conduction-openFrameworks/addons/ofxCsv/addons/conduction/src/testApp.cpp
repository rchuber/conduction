#include "testApp.h"

int bio_reading_count = 1;
int bio_reading_offset = 5000; // number of microseconds of readings to ignore
float relative_eda;
float relative_bpm;
float next_beat_at = 0;
int beat_count = 0; // the sensor seems to have picked up 3x actual heart rate, so need to filter

const int num_volume_readings = 10;
float volume_readings[num_volume_readings];      // the readings
int volume_read_index = 0;              // the index of the current reading
float volume_total = 0;                  // the running total
float volume_average = 0;                // the average

const int num_relative_eda_readings = 10;
float relative_eda_readings[num_relative_eda_readings];      // the readings
int relative_eda_read_index = 0;              // the index of the current reading
float relative_eda_total = 0;                  // the running total
float relative_eda_average = 0;                // the average


const float trackDuration = 153;

const size_t NUM_ACTOR = 1;
vector<ofxBvh> bvh;

ofSoundPlayer player;
ofVec3f center;

//float elapsed_time = 0; // current time in track

class Particle
{
public:
	
	ofVec3f pos;
	ofVec3f vel;
	ofVec3f force;
    
    int red;
    int green;
    int blue;
    
    int point_size;
};

class Tracker
{
public:
	
	const ofxBvhJoint *joint, *root;
	deque<ofVec3f> samples;
	
	void setup(const ofxBvhJoint *o, const ofxBvhJoint *r)
	{
		joint = o;
		root = r;
	}
	
	void update(vector<Particle>& particles)
	{
		const ofVec3f &p = joint->getPosition();
		
		// update sample
		{
			samples.push_front(joint->getPosition());
			while (samples.size() > 10)
				samples.pop_back();
		}
		
		// update particle force
		{
			float n = 2.0;
			const float A = 0.4;
			const float m = 1.1;
			const float B = 1.6;
            
            //n = 2.0 + relative_eda;
			
			for (int i = 0; i < particles.size(); i++)
			{
				Particle &o = particles[i];
				ofVec3f dist = (o.pos - p);
				float r = dist.lengthSquared();
				
				if (r > 0 && r < 30*30)
				{
					r = sqrt(r);
					dist /= r;
					
					o.force += ((A / pow(r, n)) - (B / pow(r, m))) * dist * 2;
				}
			}
		}
	}
	
	float length()
	{
		if (samples.empty()) return 0;
		
		float v = 0;
		for (int i = 0; i < samples.size() - 1; i++)
			v += samples[i].distance(samples[i + 1]);
		
		return v;
	}
	
	float dot()
	{
		if (samples.empty()) return 0;
		
		float v = 0;
		
		for (int i = 1; i < samples.size() - 1; i++)
		{
			const ofVec3f &v0 = samples[i - 1];
			const ofVec3f &v1 = samples[i];
			const ofVec3f &v2 = samples[i + 1];
			
			if (v0.squareDistance(v1) == 0) continue;
			if (v1.squareDistance(v2) == 0) continue;
			
			const ofVec3f d0 = (v0 - v1).normalized();
			const ofVec3f d1 = (v1 - v2).normalized();
			
			v += (d0).dot(d1);
		}
		
		return v / ((float)samples.size() - 2);
	}
	
	void draw()
	{
        // draws trace lines
		/*
         float len = length();
		len = ofMap(len, 30, 40, 0, 1, true);
		
		float d = dot();
		d = ofMap(d, 1, 0, 255, 0, true);
		
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < samples.size(); i++)
		{
			float a = ofMap(i, 0, samples.size() - 1, 1, 0, true);
			ofSetColor(d * len, 140 * a);
			glVertex3fv(samples[i].getPtr());
		}
		glEnd();
         */
	}
};

class ParticleShape
{
public:
	
	ofxBvh *bvh;
	
	vector<Tracker*> tracker;
	
	vector<Particle> particles;
	int particle_index;
	
	void setup(ofxBvh &o)
	{
		bvh = &o;
		
		for (int i = 1; i < o.getNumJoints(); i++)
		{
			if (bvh->getJoint(i)->getName().find("Chest") == string::npos)
			{
				Tracker *t = new Tracker;
				t->setup(bvh->getJoint(i), bvh->getJoint(0));
				tracker.push_back(t);
			}
		}
		
		particle_index = 0;
		particles.resize(15000);
		for (int i = 0; i < particles.size(); i++)
		{
			Particle &p = particles[i];
			p.pos.set(0, 0, 0);
			p.vel.set(0, 0, 0);
            
            p.red = 255;
            p.green = 255;
            p.blue = 255;
		}
	}
	
	void update()
	{
		bvh->update();
        float elapsed_time = (player.getPosition() * trackDuration);
		
		for (int i = 0; i < particles.size(); i++)
		{
			Particle &p = particles[i];
			p.force.set(0, 0, 0);
		}
		
		if (bvh->isFrameNew())
		{
            // have we passed a beat mark?
            int red = 255;
            int green = 255;
            int blue = 255;
            if(next_beat_at > (elapsed_time*1000 + bio_reading_offset) ){
                beat_count++;
                if(beat_count == 3){ // ignore all but ever third heart beat
                    red = 255;
                    green = 0;
                    blue = 0;
                    beat_count = 0;
                }
                next_beat_at = 0; // wait for next beat in csv
            }
            
            int point_size;
            point_size = volume_average * 2000 * 500;
            if(point_size > 2500){
                point_size = 2500;
            }
            if(point_size < 1500){
                point_size = 1500;
            }
            
			for (int i = 0; i < tracker.size(); i++)
			{
				// update force
				tracker[i]->update(particles);
				
				const ofVec3f &p = tracker[i]->joint->getPosition();
				

				// emit 10 particle every frame
				for (int i = 0; i < 10; i++)
				{
                    particles[particle_index].pos.set(p);
					
                    particles[particle_index].point_size = point_size;
                    
                    // Update particle color
                    //int color_adjustor = (int)(255.0 - (relative_eda * 200.0));
                    
                    particles[particle_index].red = red;
                    particles[particle_index].green = green;
                    particles[particle_index].blue = blue;
 
                    particle_index++;
					if (particle_index > particles.size())
						particle_index = 0;
				}
			}
		}
		
		// update particle position
		for (int i = 0; i < particles.size(); i++)
		{
			Particle &p = particles[i];
			
			//p.force.y += 0.5; // simulates gravity on particles when -.1
            //p.force.z += .2;
			p.vel *= .98; // causes particles to slow down as they progress (.98)
			
			p.vel += p.force * 0.9;
			p.pos += p.vel * 0.9;
			
            // This sets the floor for the particles (y 0 is floor, z > -200ish is back)
			
           /*  if (p.pos.z >= 0)
			{
				p.pos.z = 0;
				p.vel *= 0.95;
			}
            */
            

            
		}
	}
	
	void draw()
	{
		//bvh->draw();

		for (int i = 0; i < tracker.size(); i++)
		{
			tracker[i]->draw();
		}
		
        //int color_adjustor = (int)(255.0 - (relative_bpm * 150.0));
        //cout << relative_bpm << endl;
        //cout << color_adjustor << endl;
		//ofSetColor(255, color_adjustor, color_adjustor, 25);
        
        
		
		for (int i = 0; i < particles.size(); i++)
		{
            Particle &p = particles[i];
            glPointSize(p.point_size); // has to be outside of glBegin
            ofSetColor(p.red, p.green, p.blue, 25);
            glBegin(GL_POINTS);
            
                //p.point_size = 2000;
            
                //cout << "POINT SIZE:" << endl;
                //cout << p.point_size << endl;
                glVertex3fv(p.pos.getPtr());
            glEnd();
		}
		
	}
};

vector<ParticleShape> particle_shapes;

//--------------------------------------------------------------
void testApp::setup()
{
    
    // Load Biodata CSV
    csv.loadFile(ofToDataPath("biodata.csv"));
    
    //cout << "Print out a specific CSV value" << endl;
    //cout << csv.data[1][1] << endl;
    // also you can write...
    //cout << csv.data[0].at(1) << endl;
    
    //cout << "Print out the first value" << endl;
    //cout << csv.data[0].front() << endl;
    
    //cout << "Maximum Size:";
    //cout << csv.data[0].max_size() << endl;
    
    // create an array of all of the milliseconds in the show
    /*
     for (long i=0; i<trackDuration*1000; i++){
        
    }
    */
    
    // This collects readings and updates the average for smoothing
    // preintialize
    for (int this_volume_reading = 0; this_volume_reading < num_volume_readings; this_volume_reading++) {
        volume_readings[this_volume_reading] = 0;
    }
    
    
	ofSetFrameRate(30);
	ofSetVerticalSync(false);
	
	ofBackground(0);
	
	bvh.resize(NUM_ACTOR);
	particle_shapes.resize(NUM_ACTOR);
	
	
	bvh[0].load("bvhfiles/secondsong-centered-aachentest35.bvh");
	//bvh[1].load("bvhfiles/kashiyuka.bvh");
	//bvh[1].load("bvhfiles/nocchi.bvh");
	
	for (int i = 0; i < NUM_ACTOR; i++)
	{
		bvh[i].setFrame(1);
		particle_shapes[i].setup(bvh[i]);
	}
	
	player.loadSound("carminaburana.wav");
	player.play();
    
    //cout << particle_shapes[0] << endl;
    
}

//--------------------------------------------------------------
void testApp::update()
{
	
    float elapsed_time = (player.getPosition() * trackDuration);
	
    //bio_reading_count
    //cout << "Biodata!" << endl;
    //cout << bio_reading_count << endl;
    //cout << csv.numRows << endl;
    //cout << csv.data[1][5] << endl;
    
    // Increment the counter until we reach a time that is past the current time (then use the previous reading)
     while ( (bio_reading_count < csv.numRows) && (csv.getInt(bio_reading_count,4) < ((elapsed_time*1000 + bio_reading_offset)))) {
        
        relative_bpm = csv.getFloat(bio_reading_count,5);
        relative_eda = csv.getFloat(bio_reading_count,6);
         
         // Smooth relative EDA
         // subtract the last reading:
         relative_eda_total = relative_eda_total - relative_eda_readings[relative_eda_read_index];
         // read from the sensor:
         relative_eda_readings[relative_eda_read_index] = relative_eda;
         // add the reading to the total:
         relative_eda_total = relative_eda_total + relative_eda_readings[relative_eda_read_index];
         // advance to the next position in the array:
         relative_eda_read_index = relative_eda_read_index + 1;
         // if we're at the end of the array...
         if (relative_eda_read_index >= num_relative_eda_readings) {
             // ...wrap around to the beginning:
             relative_eda_read_index = 0;
         }
         // calculate the average:
         relative_eda_average = relative_eda_total / num_relative_eda_readings;
         
         
        bio_reading_count++;
        next_beat_at = csv.getInt(bio_reading_count,4); // whenever this is set, will pulse
    }

    
    cout << "Next beat at:" << endl;
    cout << next_beat_at << endl;
    
   /* cout << "Total" << endl;
    cout << volume_total << endl;

    cout << "Last reading" << endl;
    cout << volume_readings[volume_read_index] << endl;
    */
    
    // subtract the last reading:
    volume_total = volume_total - volume_readings[volume_read_index];
    
    // read from the sensor:
    float* f = ofSoundGetSpectrum(1);
    volume_readings[volume_read_index] = f[0];
    
    cout << "Current reading!" << endl;
    cout << f[0] << endl;

    // add the reading to the total:
    volume_total = volume_total + volume_readings[volume_read_index];
    // advance to the next position in the array:
    volume_read_index = volume_read_index + 1;
    
    // if we're at the end of the array...
    if (volume_read_index >= num_volume_readings) {
        // ...wrap around to the beginning:
        volume_read_index = 0;
    }
    
    cout << "Index" << endl;
    cout << volume_read_index << endl;
    
    
    // calculate the average:
    volume_average = volume_total / num_volume_readings;
    
    cout << "Average" << endl;
    cout << volume_average << endl;
    
    
    /*
     cout << "Biodata!" << endl;
     cout << relative_bpm << endl;
     cout << relative_eda << endl;
    */
    
    // time, isbeat, bpm, edr, time after start, relative bpm, relative edr
    //cout << t << endl;
    
	ofVec3f avg;
	
	for (int i = 0; i < NUM_ACTOR; i++)
	{
		ofxBvh *o = particle_shapes[i].bvh;
		
		o->setPosition(elapsed_time / o->getDuration());
		particle_shapes[i].update();
		
		avg += o->getJoint(0)->getPosition();
	}
	
	avg /= 1;
	
	center += (avg - center) * .1;
    
}

//--------------------------------------------------------------
void testApp::draw()
{
    int point_size;
    
	glDisable(GL_DEPTH_TEST);
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	
	// smooth particle
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	
	static GLfloat distance[] = {0.0, 0.0, 1.0};
	glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, distance);

    
    //cout << pointSize << endl;
    //cout << volume_average << endl;
/*
    int i = 0;
    int height = ofGetHeight();
    while(i<512) {
        ofRect(i*2, height - (f[i] * 1000), 2, f[i] * 1000);
        i++;
    }
*/
    
    glLineWidth(2);
    glPointSize(1000);

    
	cam.begin();
    
    // Zoom camera
    float scale = exp(relative_eda_average); // maxes out at e
    cout << "SCALE:" << endl;
    cout << scale << endl;
    
    ofScale(scale,scale,scale);
    
    // Rotates camera
	//ofRotateY(ofGetElapsedTimef() * 20);
    ofRotateY(180);
    
    // moves objects towards collective center
	ofTranslate(-center);
	
    
	for (int i = 0; i < NUM_ACTOR; i++)
	{
		particle_shapes[i].draw();
	}
	
    
    /*
     glPointSize(1000);
    for (int i = 0; i < NUM_ACTOR; i++)
    {
        particle_shapes[i].draw();
    }
     */
    cam.end();
    
    
}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
	if (player.getSpeed() > 0)
		player.setSpeed(0);
	else
		player.setSpeed(1);
}

//--------------------------------------------------------------
void testApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y)
{

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo)
{

}