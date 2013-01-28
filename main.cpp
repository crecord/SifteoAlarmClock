/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

// METADATA

static Metadata M = Metadata()
    .title("Pairing Example")
    .package("com.sifteo.sdk.pairing", "0.1")
    .icon(Icon)
    .cubeRange(1, 2);

// GLOBALS

static VideoBuffer vbuf[2]; // one video-buffer per cube
static TiltShakeRecognizer motion[2];

static CubeSet activeCubes; // cubes showing the active scene

static bool inRunMode = false;

static bool musicInitialized = false;
static bool resumeMusic = false;

static float hours = 0;
static float minutes = 0;

static bool gotStartTime = false;
static SystemTime startTime;
static TimeDelta totalTime = TimeDelta(0.0);

static bool alarm = false;
static bool countDown = false;

static int count = 0;

static float convertedSec = 0;
static float convertedMin = 0;
static float displayHour = 0;
static float displayMin = 0;
static float displaySec = 0;

// FUNCTIONS

static void playSfx(const AssetAudio& sfx) {
    static int i=0;
    AudioChannel(i).play(sfx);
    i = 1 - i;
}

static void drawText(CubeID cid, String<128> str, int x, int y) {
	vbuf[cid].initMode(BG0_ROM);
	vbuf[cid].bg0rom.text(vec(x,y), str);
}

static void activateCube(CubeID cid) {
    // mark cube as active and render its canvas
    activeCubes.mark(cid);
   
    String<128> str;
    if (inRunMode) {
		str << "I am cube #" << cid << "\n";
    	drawText(cid, str, 1, 1);
    }
    else {
    	str << "In set mode...\n";
    	drawText(cid, str, 1, 1);
    }
}

static bool isActive(NeighborID nid) {
    // Does this nid indicate an active cube?
    return nid.isCube() && activeCubes.test(nid);
}

static void onNeighborAdd(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
	LOG("in add\n");
    // update art on active cubes (not loading cubes or base)
    if (inRunMode) {
    	LOG("running\n");
    	if (isActive(cube0)) {
    		LOG("isActive\n");
    		Neighborhood nb(cube0);
    		if ((cube0 == 0 && (nb.cubeAt(RIGHT)).isDefined()) ||
    			(cube0 == 1 && (nb.cubeAt(LEFT).isDefined()))) {
    			AudioTracker::pause();
    			resumeMusic = true;
    			alarm = false;
    			LOG("connection!\n");
    		}
    	}
    }
}

static void onNeighborRemove(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
    // update art on active cubes (not loading cubes or base)
    if (inRunMode) {
    	if (isActive(cube0)) {
    		Neighborhood nb(cube0);
    		if ((cube0 == 0 && !((nb.cubeAt(RIGHT)).isDefined())) ||
    			(cube0 == 1 && !((nb.cubeAt(LEFT)).isDefined()))) {
    			if (resumeMusic && alarm) {
    				resumeMusic = false;
    				AudioTracker::resume();
    				//LOG("broken connection!\n");
    			}
    		}
    	}
    }
}

void onTouch(void* ctxt, unsigned id) {
	//LOG("on touch\n");
	if (!inRunMode) {
		//LOG("now in run mode!!!!!\n");
		inRunMode = true;
    	
    	if (!gotStartTime) {
    		startTime = SystemTime::now();
    		totalTime = TimeDelta(((hours*60.0f) + minutes)*60.0f);
    		gotStartTime = true;
    		//countDown = true;
    	}
	}
}

void onAccelChange(void* ctxt, unsigned id) {
	if (!inRunMode) {
		CubeID cube(id);

		unsigned changeFlags = motion[id].update();
		if (changeFlags) {
			auto tilt = motion[id].tilt;
		
			if (tilt.y == -1) {
				if (cube == 0) hours++;
				else if (cube == 1) minutes++;
			}
			if (tilt.y == 1) {
				if (cube == 0) hours--;
				else if (cube == 1) minutes--;
			}
			
			if (hours < 0) {
				hours = 60;
			}
			if (hours > 60) {
				hours = 0;
			}
			if (minutes < 0) {
				minutes = 60;
			}
			if (minutes > 60) {
				minutes = 0;
			}
		}
	}
}

void main() {

    // subscribe to events
    Events::neighborAdd.set(onNeighborAdd);
    Events::neighborRemove.set(onNeighborRemove);
    
    Events::cubeTouch.set(onTouch);
    Events::cubeAccelChange.set(onAccelChange);
      
    for(CubeID cid : CubeSet::connected()) {
       	vbuf[cid].attach(cid);
       	motion[cid].attach(cid);
        activateCube(cid);
    }
    
    AudioTracker::setVolume(0.2f * AudioChannel::MAX_VOLUME);
    
    //if (inRunMode) LOG("run mode\n");
    //else LOG("set mode\n");
    
    // run loop
    while(1) {
        System::paint();
               
        if (count > 1800 /*&& countDown*/) {
        	LOG("in count conditional \n");
        	SystemTime curTime = SystemTime::now();
        	TimeDelta timePast = curTime - startTime;
        	if (timePast > totalTime) {
    			if (!musicInitialized) {
    				musicInitialized = true;
    				AudioTracker::play(Music);
    				alarm = true;
    				//countDown = false;
    			}
        	}
        	convertedSec = timePast.seconds();
        	convertedMin = convertedSec / 60;
        	displayHour = convertedMin / 60;
        	displayMin = (int)convertedMin % (int)60;
        	displaySec = convertedSec - (convertedMin * 60);
        	count = 0;
        }
        //LOG_FLOAT(displaySec);
        count++;
        
        //if (inRunMode) LOG("run mode\n");
    	//else LOG("set mode\n");
        
        for(CubeID cid : CubeSet::connected()) {
        	activeCubes.mark(cid);
        	String<128> str;
    		if (inRunMode) {
    			if (alarm) {
    				if (cid == 0) str << "0:\n";
    				else if (cid == 1) str << ":00\n";
    				drawText(cid, str, 1, 1);
    			}
    			else {
    				if (cid == 0) str << Fixed(hours-displayHour, 3) << ":\n";
					else if (cid == 1) str << ":" << Fixed(minutes-displayMin, 3) << "\n";
					drawText(cid, str, 1, 1) ;
    			}
    		}
    		else {
				if (cid == 0) str << "Hours: " << Fixed(hours, 3) << "\n";
				else if (cid == 1) str << "Minutes: " << Fixed(minutes, 3) << "\n";
				drawText(cid, str, 1, 1) ;
    		}
    	}
        
    }
}
