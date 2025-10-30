#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

static PadState pad;
static HidSixAxisSensorHandle handles[4];

static HidSixAxisSensorState g_sixState;
std::string error_message;
std::string info;
float sensors[10];


typedef struct {
	int x;
	int y;
	int xSpeed;
	int ySpeed;
} Circle;

const int circleCount = 6;
Circle circles[circleCount];

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
		size_t* total_out;

		hidGetSevenSixAxisSensorStates(state, 1, total_out);
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




void renderDots(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
	r->disableScissoring();

	// auto color = tsl::style::color::ColorHighlight;
	tsl::Color red = tsl::Color{0xF, 0x0, 0x0, 0xF};
	tsl::Color blue = tsl::Color{0x0, 0x0, 0xF, 0xF};
	tsl::Color white = tsl::Color{0xF, 0xF, 0xF, 0xF};
	

	float gx = g_sixState.angular_velocity.x;
	float gy = g_sixState.angular_velocity.y;
	float gz = g_sixState.angular_velocity.z;
	
	float anx = g_sixState.angle.x;
	float any = g_sixState.angle.y;
	float anz = g_sixState.angle.z;

	float ax = g_sixState.acceleration.x;
	float ay = g_sixState.acceleration.y;
	float az = g_sixState.acceleration.z;



	auto font_size = 20;

//auto info = (&pad);

	//char buf[64];
    	//snprintf(buf, sizeof(buf), "Gyro X: %.2f", g_sixState.angular_velocity.x);
		//r->drawString(buf, false, x + 20, y + 40, 20, r->a(white));
		
	//auto text_string = std::format("X pos: {}, Y pos: {}\n Gyro: x={:.4f} y={:.4f} z={:.4f} \n Accel: x={:.4f} y={:.4f} z={:.4f} \n Angle: x={:.4f} y={:.4f} z={:.4f}\nError: {}, \info: {} ", x, y, gx, gy, gz, ax, ay, az, anx, any, anz, error_message, info);

	auto text_string = std::format("Gyro: x={:.4f} y={:.4f} z={:.4f}\nAccel: x={:.4f} y={:.4f} z={:.4f}\nAngle: x={:.4f} y={:.4f} z={:.4f}\nError: {}\nx: {} y: {}", gx, gy, gz, ax, ay, az, anx, any, anz, error_message, x, y);
	//
	auto sensor_string = std::format("Accel x: {:.4f}\nAccel y: {:.4f}\nAccel z: {:.4f}\nGyro x: {:.4f}\nGyro y: {:.4f}\nGyro z: {:.4f}\nAngle x: {:.4f}\nAngle y: {:.4f}\nAngle z: {:.4f}\n9: {:.4f}", 
					sensors[0], // accel x
					sensors[1], // accel y
					sensors[2], // accel z
					sensors[3], // gyro x
					sensors[4], // gyro y
					sensors[5], // gyro z
					sensors[6], // angle x
					sensors[7], // angle y
					sensors[8], // angle z
					sensors[9]);// ?

	const char* text = text_string.c_str();

    std::string circleText;

	r->drawRect(0, 0, 1280, 100, r->a(blue));
	r->drawCircle(tsl::cfg::FramebufferWidth/2, tsl::cfg::FramebufferHeight/2, 20, true, r->a(white));
    for (Circle circle : circles) {
        circleText += std::format("x: {}, y: {}\n", circle.x, circle.y);
	    r->drawCircle(circle.x, circle.y, 20, true, r->a(white));
    }


	const char* c_text = circleText.c_str();
	r->drawString(c_text, false, 800, y+font_size, font_size, r->a(white));

	r->drawString(text, false, x + 0, y + font_size, font_size, r->a(white));

	const char* text2 = sensor_string.c_str();
	r->drawString(text2, false, x + 0, y + font_size+200, font_size, r->a(white));

    r->enableScissoring(0, 0, 0, 0);
}

class EmptyOverlayFrame: public tsl::elm::OverlayFrame {
		public:
				EmptyOverlayFrame() : tsl::elm::OverlayFrame("", "") {}
				virtual void draw(tsl::gfx::Renderer *renderer) override {
						
						//this->setBoundaries(0, 0, 100, 200);
						//renderer->disableScissoring();

						renderer->fillScreen(a({0x0, 0x0, 0x0, 0xD}));
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
        int initialX = padding;
        int initialY = padding;
        int width = tsl::cfg::FramebufferWidth - padding;
        int height = tsl::cfg::FramebufferHeight - padding;

        int perimeter = width*2 + height*2;
        s32 circleOffset = perimeter / circleCount;

        for (int i = 0; i < circleCount; ++i) {
            float dist = i * circleOffset;
            float px, py;

            if (dist <= width) { // top edge
                px = initialX + dist;
                py = initialY;
            } else if (dist <= width + height) { // right edge
                px = initialX + width;
                py = initialY + (dist - width) - padding;
            } else if (dist <= 2 * width + height) { // bottom edge
                px = initialX + width - (dist - (width + height));
                py = initialY + height;
            } else { // left edge
                px = initialX;
                py = initialY + height - (dist - (2 * width + height));
            }

            circles[i].x = px;
            circles[i].y = py;
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
		return true;
    }
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {
		//tsl::hlp::requestForeground(false);
		initInput();
	}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {
		hidFinalizeSevenSixAxisSensor();
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
