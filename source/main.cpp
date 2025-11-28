#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

static PadState pad;
static HidSixAxisSensorHandle handles[4];

static HidSixAxisSensorState g_sixState;
std::string error_message;
std::string info;
float sensors[10];


typedef struct {
	float x0;
	float y0;
	float x;
	float y;
	float vx;
	float vy;
} Dot;

// TODO: Make configurable
const tsl::Color dotColor = {0xF, 0xF, 0xF, 0x0F};
const int dotSize = 10;
const int dotsPerHorizontal = 10;
const int dotsPerVertical = 5;
const int dotCount = dotsPerVertical*2+dotsPerHorizontal*2-4; // corners are counted twice

const float ACCEL_GAIN = 100000.0f;
const float GYRO_GAIN = 10000.0f;    // tweak this
// const float DAMPING = 8.0f;
// const float SPRING = 5.0f;
const float DAMPING = 50.0f;
const float SPRING = 400.0f;
const float MAX_DIST = 30.0f;     // max pixel offset

Dot dots[dotCount];



void updateDots(float dt) {
    // float gx = g_sixState.angular_velocity.x;
    // float gy = g_sixState.angular_velocity.y;
	float ax = sensors[0];
	float ay = -sensors[1];
	float az = sensors[2];


	// remove gravity using a slow filter (low-pass)
	static float gxFilt = 0.0f, gyFilt = 0.0f, gzFilt = 1.0f;
	const float alpha = 0.2f; // tweak (lower = smoother)
	gxFilt = gxFilt * (1.0f - alpha) + ax * alpha;
	gyFilt = gyFilt * (1.0f - alpha) + ay * alpha;
	gzFilt = gzFilt * (1.0f - alpha) + az * alpha;

	// Subtract gravity component
	ax -= gxFilt;
	ay -= gyFilt;
	az -= gzFilt;
	
	info = std::format(
		"linear Accel: x={:.4f} y={:.4f} z={:.4f}\nFilt: x={:.4f} y={:.4f} z={:.4f}",
		ax,
		ay,
		az,
		gxFilt,
		gyFilt,
		gzFilt
	);

    for (int i = 0; i < dotCount; ++i) {
    	Dot &d = dots[i];

		// Scale and invert: device moves right → dot moves left
		float fx = -ax * ACCEL_GAIN - (d.x - d.x0) * SPRING - d.vx * DAMPING;
		float fy = -ay * ACCEL_GAIN - (d.y - d.y0) * SPRING - d.vy * DAMPING;

        d.vx += fx * dt;
        d.vy += fy * dt;

        d.x += d.vx * dt;
        d.y += d.vy * dt;

    	// limit movement
        float dx = d.x - d.x0;
        float dy = d.y - d.y0;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > MAX_DIST) {
            float scale = MAX_DIST / dist;
            d.x = d.x0 + dx * scale;
            d.y = d.y0 + dy * scale;
        }
    }
}

void updateInput() {
	padUpdate(&pad);

    	HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&pad);
		HidSixAxisSensorHandle handle;

	// Read from the correct sixaxis handle depending on the current input style
        if (style_set & HidNpadStyleTag_NpadHandheld)
				handle = handles[0];
        else if (style_set & HidNpadStyleTag_NpadFullKey)
				handle = handles[1];
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)

				handle = handles[2];
            else if (attrib & HidNpadAttribute_IsRightConnected)
				handle = handles[3];
        }
        hidGetSixAxisSensorStates(handle, &sixaxis, 1);

    	g_sixState = sixaxis; // store globally for the drawer to use
	


		HidSevenSixAxisSensorState state[4];
		size_t total_out = 0;

		hidGetSevenSixAxisSensorStates(state, 1, &total_out);
		//hidIsSevenSixAxisSensorAtRest(&res);
		//info = std::format("{}", state[0].unk_x18[0]);
		//sensors = state[0].unk_x18;
		for(int i=0;i<10;i++) {
				sensors[i] = state[0].unk_x18[i];
		}

}

void initInput() {
	padConfigureInput(8, HidNpadStyleSet_NpadStandard);

	padInitializeAny(&pad);

		Result rc=0, rc2=0;
    bool initflag=0;
    u8 *workmem = NULL;
    size_t workmem_size = 0x1000;

    error_message = "hdls example";

    rc = hiddbgInitialize();
    if (R_FAILED(rc)) {
        error_message = std::format("hiddbgInitialize(): {}", rc);
    }
    else {
        workmem = (u8 *) aligned_alloc(0x1000, workmem_size);
        if (workmem) initflag = 1;
        else error_message = std::format("workmem alloc failed");

    }

    	hidGetSixAxisSensorHandles(&handles[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    	hidGetSixAxisSensorHandles(&handles[1], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
    	hidGetSixAxisSensorHandles(&handles[2], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
    	hidStartSixAxisSensor(handles[0]);
    	hidStartSixAxisSensor(handles[1]);
    	hidStartSixAxisSensor(handles[2]);
    	hidStartSixAxisSensor(handles[3]);

		hidInitializeSevenSixAxisSensor();
		hidStartSevenSixAxisSensor();

}


// auto color = tsl::style::color::ColorHighlight;
tsl::Color red = tsl::Color{0xF, 0x0, 0x0, 0xF};
tsl::Color blue = tsl::Color{0x0, 0x0, 0xF, 0xF};
tsl::Color white = tsl::Color{0xF, 0xF, 0xF, 0xF};
int font_size = 20;


void renderDots(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
	r->disableScissoring();

	float gx = g_sixState.angular_velocity.x;
	float gy = g_sixState.angular_velocity.y;
	float gz = g_sixState.angular_velocity.z;
	
	float anx = g_sixState.angle.x;
	float any = g_sixState.angle.y;
	float anz = g_sixState.angle.z;

	float ax = g_sixState.acceleration.x;
	float ay = g_sixState.acceleration.y;
	float az = g_sixState.acceleration.z;

	auto six_sensor_string = std::format(
		"Gyro: x={:.4f} y={:.4f} z={:.4f}\nAccel: x={:.4f} y={:.4f} z={:.4f}\nAngle: x={:.4f} y={:.4f} z={:.4f}\nError: {}\nx: {} y: {}\ninfo:\n{}", 
		gx, gy, gz, ax, ay, az, anx, any, anz, error_message, x, y, info
	);

	auto seven_sensor_string = std::format(
		"Accel x: {:.4f}\nAccel y: {:.4f}\nAccel z: {:.4f}\nGyro x: {:.4f}\nGyro y: {:.4f}\nGyro z: {:.4f}\nAngle x: {:.4f}\nAngle y: {:.4f}\nAngle z: {:.4f}\n9: {:.4f}", 
		sensors[0], // accel x
		sensors[1], // accel y
		sensors[2], // accel z
		sensors[3], // gyro x
		sensors[4], // gyro y
		sensors[5], // gyro z
		sensors[6], // angle x
		sensors[7], // angle y
		sensors[8], // angle z
		sensors[9]  // ?
	);

	// Best solution is probably a spring
	// Also movement should be based on delta time
	// To have less speed than 1, we can use floats and
	// accumulate the movement until it reaches 1 and then move by 1 pixel
	static u64 lastTime = 0;
	u64 now = armGetSystemTick();
	float dt = (now - lastTime) / 19200000.0f; // convert ticks to seconds
	lastTime = now;

	updateDots(dt);

	// for (int i = 0; i < dotCount; ++i) {
	// 	// Update dot speed based on gyro
	// 	dots[i].vx = static_cast<int>(sensors[0]*30);
	// 	dots[i].vy = static_cast<int>(-sensors[1]*30);

	// 	// Update dot position
	// 	int oldX = dots[i].x;
	// 	int oldY = dots[i].y;

	// 	dots[i].x += dots[i].vx;
	// 	dots[i].y += dots[i].vy;

	// 	if (dots[i].x < 0 || dots[i].x > tsl::cfg::FramebufferWidth) {
	// 		dots[i].vx = -dots[i].vx;
	// 		dots[i].x = oldX;
	// 	}

	// 	if (dots[i].y < 0 || dots[i].y > tsl::cfg::FramebufferHeight) {
	// 		dots[i].vy = -dots[i].vy;
	// 		dots[i].y = oldY;
	// 	}
		
	// 	//dots[i].y += dot.vy;
	// 	dots[i].vx -= abs(dots[i].x - dots[i].x0);

	// }
	

    std::string circleText;

	//r->drawRect(0, 0, 1280, 100, r->a(blue));

	r->drawCircle(tsl::cfg::FramebufferWidth/2, tsl::cfg::FramebufferHeight/2, 20, true, r->a(white));
    for (int i = 0; i < dotCount; ++i) {
        circleText += std::format("{}: x: {:.2f}, y: {:.2f}, sx: {:.2f}, sy: {:.2f}\n", i, dots[i].x, dots[i].y, dots[i].vx, dots[i].vy);
		float dx = dots[i].x - dots[i].x0;
		float dy = dots[i].y - dots[i].y0;
		float d_total = sqrtf(dx*dx + dy*dy);

	    r->drawCircle(dots[i].x, dots[i].y, dotSize*(1-(d_total / MAX_DIST)), true, r->a(dotColor));
    }

	// DEBUG TEXT

	// const char* c_circleText = circleText.c_str();
	// r->drawString(c_circleText, false, 800, y+font_size, font_size, r->a(white));

	// const char* c_six_sensor_string = six_sensor_string.c_str();
	// r->drawString(c_six_sensor_string, false, x + 0, y + font_size, font_size, r->a(white));

	// const char* c_seven_sensor_string = seven_sensor_string.c_str();
	// r->drawString(c_seven_sensor_string, false, x + 0, y + font_size+200, font_size, r->a(white));

    r->enableScissoring(0, 0, 0, 0);
}

class EmptyOverlayFrame: public tsl::elm::OverlayFrame {
		public:
				EmptyOverlayFrame() : tsl::elm::OverlayFrame("", "") {}
				virtual void draw(tsl::gfx::Renderer *renderer) override {
						
						//this->setBoundaries(0, 0, 100, 200);
						//renderer->disableScissoring();

						// BACKGROUN FILL, default: 0xD
						renderer->fillScreen(a({0x0, 0x0, 0x0, 0x0}));
						//renderer->drawString("hello", false, 20, 20, 20, a({0xFF, 0xFF, 0xFF, 0xFF}));
						
						if (this->m_contentElement != nullptr)
							this->m_contentElement->frame(renderer);
				}

};


class GuiTest : public tsl::Gui {
public:
    GuiTest() { 
		//tsl::hlp::requestForeground(false);

		tsl::gfx::Renderer::getRenderer().setLayerPos(0, 0);
	}
    ~GuiTest() { 
		//tsl::hlp::requestForeground(true);
	}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
	//initInput();

        int padding = 50;
        int x0 = padding;
        int y0 = padding;
        int width = tsl::cfg::FramebufferWidth - padding*2;
        int height = tsl::cfg::FramebufferHeight - padding*2;


		int index = 0;

		auto addDot = [&](int x, int y) {
			dots[index].x = x;
			dots[index].y = y;
			dots[index].x0 = x;
			dots[index].y0 = y;
			dots[index].vx = 0;
			dots[index].vy = 0;
			++index;
		};

		// top/bottom edge
		for (int i = 0; i < dotsPerHorizontal; ++i) {
			float t = static_cast<float>(i) / (dotsPerHorizontal - 1);
			addDot(x0 + static_cast<int>(std::round(t * width)), y0);
			addDot(x0 + static_cast<int>(std::round(t * width)), y0 + height);
		}

		// right/left edge
		for (int i = 1; i < dotsPerVertical; ++i) {
			float t = static_cast<float>(i) / (dotsPerVertical - 1);
			addDot(x0+width, y0 + static_cast<int>(std::round(t * height)));
			addDot(x0, y0 + static_cast<int>(std::round(t * height)));
		}

		std::string file_text;

        for (int i = 0; i < dotCount; ++i) {
			file_text += std::format("{}: x: {:.2f}, y: {:.2f}\n", i, dots[i].x, dots[i].y);
		}

		FILE *f = fopen("sdmc:/hello.txt", "w");
		const char* text = file_text.c_str();
		if (f) {
			fprintf(f, "%s", text);
			fclose(f);
		}

        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new EmptyOverlayFrame();
        //auto frame = new tsl::elm::HeaderOverlayFrame(0);

	auto drawer = new tsl::elm::CustomDrawer(renderDots);

        // A list that can contain sub elements and handles scrolling
        // auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        // list->addItem(new tsl::elm::ListItem("Default List Item"));

        // Add the list to the frame for it to be drawn
        // frame->setContent(list);
        frame->setContent(drawer);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {
	updateInput();
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		tsl::hlp::requestForeground(false);
			if((keysHeld & HidNpadButton_Minus) && (keysHeld & HidNpadButton_Plus)) {
				tsl::goBack();
				//return true;
			}
		// info = std::format("{}", padGetButtonsDown(&pad));
		return true; //TODO: CHANGE TO TRUE
    }
};

class MenuGui : public tsl::Gui {
	public:
		MenuGui() {}

		virtual tsl::elm::Element* createUI() override {
			auto frame = new tsl::elm::OverlayFrame("NX-Dots", "Menu");

			auto list = new tsl::elm::List();
			auto start_item = new tsl::elm::ListItem("Start Test Overlay");
			start_item->setClickListener([](u64 keys){
				if (keys & HidNpadButton_A) {
					framebufferWidth = 1280;
					framebufferHeight = 720;

					tsl::changeTo<GuiTest>();
					return true;
				}
				return false;
			});
			list->addItem(start_item);

			frame->setContent(list);

			return frame;
		}
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {
		//tsl::hlp::requestForeground(false);
		initInput();
    	fsdevMountSdmc();
	}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {
		hidFinalizeSevenSixAxisSensor();
		fsdevUnmountAll();
		//tsl::hlp::requestForeground(true);
	}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }

};

int main(int argc, char **argv) {
	framebufferWidth = 1280;
	framebufferHeight = 720;

    return tsl::loop<OverlayTest>(argc, argv);
}
