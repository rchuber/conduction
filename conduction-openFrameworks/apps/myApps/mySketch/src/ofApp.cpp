#include "ofApp.h"

int bio_reading_count = 1;
int bio_reading_offset = 0; // number of microseconds of readings to ignore
float relative_eda;
float relative_bpm;
float next_beat_at = 0;
int beat_count = 0; // the sensor seems to have picked up 3x actual heart rate, so need to filter

float bvh_start_offset = 2.5;
bool audio_playing = false;

const int num_volume_readings = 10;
float volume_readings[num_volume_readings];      // the readings
int volume_read_index = 0;              // the index of the current reading
float volume_total = 0;                  // the running total
float volume_average = 0;
float max_volume = 0;   // the max
float relative_volume = .5;

const int num_relative_eda_readings = 10;
float relative_eda_readings[num_relative_eda_readings];      // the readings
int relative_eda_read_index = 0;              // the index of the current reading
float relative_eda_total = 0;                  // the running total
float relative_eda_average = 0;                // the average

int point_size;

const float trackDuration = 158;

const size_t NUM_ACTOR = 1;
vector<ofxBvh> bvh;

ofSoundPlayer player;
ofVec3f center;

class Particle
{
public:
    
    ofVec3f pos;
    ofVec3f vel;
    ofVec3f force;
    
    int red;
    int green;
    int blue;
    int alpha;
    
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
            p.alpha = 40;
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
            int red = 255; //50 + (205 * relative_volume);
            int green = 255; //50 + (205 * relative_volume);
            int blue = 255; //50 + (205 * relative_volume);
            int alpha = 25 + (25.0*relative_volume);

            
            if(next_beat_at > (elapsed_time*1000 + bio_reading_offset) ){
                beat_count++;
                if(beat_count == 3){ // ignore all but every third heart beat
                    red = 255;
                    green = 0;
                    blue = 0;
                    beat_count = 0;
                }
                next_beat_at = 0; // wait for next beat in csv
            }
            
            
            for (int i = 0; i < tracker.size(); i++)
            {
                // update force
                tracker[i]->update(particles);
                
                // affect the magnitude of the vector using the volume
                const ofVec3f &p = tracker[i]->joint->getPosition();
                
                // emit 10 particle every frame
                for (int k = 0; k < 10; k++)
                {
                    
                    particles[particle_index].pos.set(p);

                    particles[particle_index].point_size = point_size;
                    
                    particles[particle_index].red = red;
                    particles[particle_index].green = green;
                    particles[particle_index].blue = blue;
                    particles[particle_index].alpha = alpha;
                    
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
            p.vel *= .99; // causes particles to slow down as they progress (.98)
            
            p.vel += p.force * 0.9;
            p.pos += p.vel * 0.9;
            
        }
    }
    
    void draw()
    {
        // this will draw bvh bones (good for debugging)
        //bvh->draw();
        
        for (int i = 0; i < tracker.size(); i++)
        {
            tracker[i]->draw();
        }
        
        
        for (int i = 0; i < particles.size(); i++)
        {
            Particle &p = particles[i];
            glPointSize(p.point_size); // has to be outside of glBegin
            ofSetColor(p.red, p.green, p.blue, p.alpha);
            glBegin(GL_POINTS);
            
            glVertex3fv(p.pos.getPtr());
            glEnd();
        }
        
    }
};

vector<ParticleShape> particle_shapes;

//--------------------------------------------------------------
void testApp::setup()
{
    
    gui.setup(); // most of the time you don't need a name
    gui.add(val1.setup("x rotate",88.2,-180.0,180.0));
    gui.add(val2.setup("y rotate",0.0,-180.0,180.0));
    gui.add(val3.setup("z rotate",95,-180.0,180.0));
    gui.add(val4.setup("x translate",100.0,-1000,1000));
    gui.add(val5.setup("y translate",260.0,-1000,1000));
    gui.add(val6.setup("z translate",142.0,-180.0,180.0));
    gui.add(val7.setup("x camera target",0.0,-1000,1000));
    gui.add(val8.setup("z camera start",660.0,-1000,1000));
    gui.add(min_point_size.setup("min point size",900.0,0.0,10000.0));
    gui.add(max_point_size.setup("max point size",8000.0,0.0,10000.0));



    bHide = false;
    
    // Load Biodata CSV
    csv.loadFile(ofToDataPath("biodata.csv"));
    
    // This collects readings and updates the average for smoothing
    // preintialize
    for (int this_volume_reading = 0; this_volume_reading < num_volume_readings; this_volume_reading++) {
        volume_readings[this_volume_reading] = 0;
    }
    
    
    ofSetFrameRate(60);
    ofSetVerticalSync(false);
    
    ofBackground(0);
    
    bvh.resize(NUM_ACTOR);
    particle_shapes.resize(NUM_ACTOR);
    
    
    bvh[0].load("bvhfiles/secondsong.bvh");
    
    for (int i = 0; i < NUM_ACTOR; i++)
    {
        bvh[i].setFrame(1);
        particle_shapes[i].setup(bvh[i]);
    }
    
    player.loadSound("carminaburana.wav");
    player.play();
    
    
}

//--------------------------------------------------------------
void testApp::update()
{
    
    float elapsed_time = (player.getPosition() * trackDuration);
    
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
    
    // subtract the last reading:
    volume_total = volume_total - volume_readings[volume_read_index];
    
    // read from the sensor:
    float* f = ofSoundGetSpectrum(1);
    if(f[0] > max_volume){
        max_volume = f[0];
    }
    volume_readings[volume_read_index] = f[0];
    
    // add the reading to the total:
    volume_total = volume_total + volume_readings[volume_read_index];
    // advance to the next position in the array:
    volume_read_index = volume_read_index + 1;
    
    // if we're at the end of the array...
    if (volume_read_index >= num_volume_readings) {
        // ...wrap around to the beginning:
        volume_read_index = 0;
    }
    
    // calculate the average:
    volume_average = volume_total / num_volume_readings;

    // .133758 is the maximum volume
    relative_volume = volume_average / .133758;
    point_size = min_point_size + relative_volume * (max_point_size - min_point_size);
    
    ofVec3f avg;
    int nextFrame = (elapsed_time - bvh_start_offset)*30;

    for (int i = 0; i < NUM_ACTOR; i++)
    {
        ofxBvh *o = particle_shapes[i].bvh;
        
        o->setFrame(nextFrame);
        
        particle_shapes[i].update();
        
        avg += o->getJoint(0)->getPosition();
    }
    
    avg /= 1;
    
    //center += (avg - center) * .1;
    
}

//--------------------------------------------------------------
void testApp::draw()
{
    
    //glDisable(GL_DEPTH_TEST);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    
    // smooth particle
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    static GLfloat distance[] = {0.2, 0.2, 0.2};
    
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, distance);
    
    glLineWidth(2);
    glPointSize(1000);
    
    cam.begin();
    
    // Slow gradual rotation of camera
    ofRotateY(ofGetElapsedTimef() * -10);
    
    float tweenvalue = (ofGetElapsedTimeMillis() % 2000) /2000.f; // this will slowly change from 0.0f to 1.0f, resetting every 2 seconds
    
    ofQuaternion startQuat;
    ofQuaternion targetQuat;
    ofVec3f startPos;
    ofVec3f targetPos;
    
    // we define the camera's startorientation here:
    startQuat.makeRotate(0, 0, 0, 1);			// zero rotation.
    
    // we define the camera's start and end-position here:
    startPos.set(0,0,val8);
    targetPos.set(0,0,val9);
    
    cam.setGlobalPosition(startPos);
    
    cam.setOrientation(startQuat);

    
    ofPushMatrix();
    {
        // get the bvh into the proper orientation
        ofRotate(val1, 1, 0, 0);
        ofRotate(val2, 0, 1, 0);
        ofRotate(val3, 0, 0, 1);
        
         ofTranslate(val4, val5, val6);
        
        
        for (int i = 0; i < NUM_ACTOR; i++)
        {
            particle_shapes[i].draw();
        }
    }
    ofPopMatrix();
    
    cam.end();
    
    // should the gui control be hiding?
    if( bHide ){
        gui.draw();
    }
    
}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
    if( key == 'h' ){
        bHide = !bHide;
    }
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