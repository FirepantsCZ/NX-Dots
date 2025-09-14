#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

static PadState pad;
static HidSixAxisSensorHandle handles[4];

static HidSixAxisSensorState g_sixState;

void updateInput() {
	padUpdate(&pad);

    	HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&pad);

	// Read from the correct sixaxis handle depending on the current input style
        if (style_set & HidNpadStyleTag_NpadHandheld)
            hidGetSixAxisSensorStates(handles[0], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadFullKey)
            hidGetSixAxisSensorStates(handles[1], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)
                hidGetSixAxisSensorStates(handles[2], &sixaxis, 1);
            else if (attrib & HidNpadAttribute_IsRightConnected)
                hidGetSixAxisSensorStates(handles[3], &sixaxis, 1);
        }

    	g_sixState = sixaxis; // store globally for the drawer to use
}

void initInput() {
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);

	padInitializeDefault(&pad);

    	hidGetSixAxisSensorHandles(&handles[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    	hidGetSixAxisSensorHandles(&handles[1], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
    	hidGetSixAxisSensorHandles(&handles[2], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
    	hidStartSixAxisSensor(handles[0]);
    	hidStartSixAxisSensor(handles[1]);
    	hidStartSixAxisSensor(handles[2]);
    	hidStartSixAxisSensor(handles[3]);
}

void renderDots(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
	// auto color = tsl::style::color::ColorHighlight;
	tsl::Color red = tsl::Color{0xF, 0x0, 0x0, 0xF};
	tsl::Color blue = tsl::Color{0x0, 0x0, 0xF, 0xF};
	tsl::Color white = tsl::Color{0xF, 0xF, 0xF, 0xF};
	
	/*
	int err = 0;

	HidSixAxisSensorHandle handles[1];

	HidNpadStyleTag tag = HidNpadStyleTag_NpadHandheld;
	HidNpadIdType type = HidNpadIdType_Handheld;
	HidGyroscopeZeroDriftMode mode = HidGyroscopeZeroDriftMode_Standard;

	HidNpadHandheldState handheld_states[1];
	hidGetNpadStatesHandheld(type, &handheld_states[0], 1);

	hidGetSixAxisSensorHandles(&handles[0], 1, type, tag);

	hidStartSixAxisSensor(handles[0]);

	hidSetNpadJoyAssignmentModeSingleByDefault(type);
	hidSetGyroscopeZeroDriftMode(handles[0], mode);

	HidSixAxisSensorState states[1];
	size_t state_count = hidGetSixAxisSensorStates(handles[0], &states[0], 1);

	auto state = states[0];

	//float gx = state.angular_velocity.x;
	//float gy = state.angular_velocity.y;
	//float gz = state.angular_velocity.z;
	
	float gx = state.angle.x;
	float gy = state.angle.y;
	float gz = state.angle.z;

	float ax = state.acceleration.x;
	float ay = state.acceleration.y;
	float az = state.acceleration.z;
	*/



	auto font_size = 20;

	r->drawRect(x + 100, y + 100, 100, 100, r->a(blue));

	char buf[64];
    	snprintf(buf, sizeof(buf), "Gyro X: %.2f", g_sixState.angular_velocity.x);
    	r->drawString(buf, false, x + 20, y + 40, 20, r->a(white));
		
	auto text_string = std::format("X pos: {}, Y pos: {}\n sixaxis: {}", x, y, g_sixState.angular_velocity.x);
	//auto text_string = std::format("hello, {}", x);

	const char* text = text_string.c_str();
	r->drawString(text, false, x + 0, y + font_size, font_size, r->a(white));
}


class GuiTest : public tsl::Gui {
public:
    GuiTest() { }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
	initInput();

        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        // auto frame = new tsl::elm::OverlayFrame("Tesla Example", "v1.3.1");
        auto frame = new tsl::elm::HeaderOverlayFrame(0);

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
        return false;   // Return true here to signal the inputs have been consumed
    }
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
