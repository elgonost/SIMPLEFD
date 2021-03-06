#include <pebble.h>

#define NUM_SAMPLES 25

static Window *s_window;	
static int s_battery_level;
static TextLayer *s_battery_layer;
static TextLayer *s_time_layer;
static TextLayer *fall_layer;
AppTimer *appTimer;
DictionaryIterator *iter;
int simpleSum[NUM_SAMPLES];
int meanSimpleSum = 0;
int maxSimpleSum = -1;
int minSimpleSum = 100000000;
int maxMinDifference;
int state[2]; // state[0]:Asleep | state[1]:Awake
int previousState = 0; // 0 for asleep | 1 for awake
int currentState = 0;
int counter = 0;
bool highPeak = false;
bool possibleFall = false;
bool highlyPossibleFall = false;
int dummy = 10;
int timestampFirstHalf;
int timestampSecondHalf;
int fallThreshold = 5000;
int maxMinThreshold = 450;
int pointThreshold = 1200;
int pointStep = 20;


// Keys for AppMessage Dictionary
// These should correspond to the values you defined in appinfo.json/Settings
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  FALL_KEY = 2,
  TIME_KEY0 = 3,
  TIME_KEY1 = 4,
  TYPE_KEY = 5,
  THRESHOLD_KEY = 6,
  SENSITIVITY_KEY = 7,
  POINT_THRESHOLD_KEY = 8,
  STEP_KEY = 9
};

// Write message to buffer & send
static void send_message(void){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_KEY, "I'm a Pebble!");
	
	dict_write_end(iter);
  app_message_outbox_send();
}

void sendEmail(){
  app_message_outbox_begin(&iter);
  dict_write_int(iter,FALL_KEY,&dummy,sizeof(int),true);
  dict_write_end(iter);
  app_message_outbox_send();
}

void sendStateChange(int firstHalf,int secondHalf,int currentState){
  app_message_outbox_begin(&iter);
  dict_write_int(iter,TIME_KEY0,&firstHalf,sizeof(int),true);
  dict_write_int(iter,TIME_KEY1,&secondHalf,sizeof(int),true);
  dict_write_int(iter,TYPE_KEY,&currentState,sizeof(int),true);
  dict_write_end(iter);
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_DEBUG,"message from sendStateChange function");
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
}

static void battery_callback(BatteryChargeState state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"battery level is %d ",s_battery_level);
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  //layer_mark_dirty(s_battery_layer);
  static char s_buffer[128];
   
  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer),"Battery: %d%%",s_battery_level);
  text_layer_set_text(s_battery_layer, s_buffer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // A single click has just occured
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Select key pressed");
  app_timer_cancel(appTimer);
  text_layer_set_text(fall_layer, " ");
  APP_LOG(APP_LOG_LEVEL_DEBUG,"AppTimer was Canceled");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Down key pressed");
  vibes_short_pulse();
  sendEmail();
}

static void click_config_provider(void *context) {
  // Subcribe to button click events here
  ButtonId id_select = BUTTON_ID_SELECT;  // The Select button
  ButtonId id_down = BUTTON_ID_DOWN;
  window_single_click_subscribe(id_select, select_click_handler);
  window_single_click_subscribe(id_down, down_click_handler);
}

void appTimerCallback(void *data){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppTimer not cancelled. Sending email.");
  vibes_short_pulse();
  text_layer_set_text(fall_layer, " ");
  sendEmail();
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  
  if (counter==0){
    timestampFirstHalf = data[0].timestamp / 100000000;
    timestampSecondHalf = data[0].timestamp % 100000000;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "initialTimestamp = %d %d",timestampFirstHalf,timestampSecondHalf);
  }
  meanSimpleSum = 0;
  maxSimpleSum = -1;
  minSimpleSum = 100000000;
  
  for(int i=0;i<NUM_SAMPLES;i++){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "x = %d, y = %d, z = %d", abs(data[i].x),data[i].x,data[i].z); 
    simpleSum[i]=abs(data[i].x)+abs(data[i].y)+abs(data[i].z);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "simpleSum[%d] = %d", i, simpleSum[i]);
    
    meanSimpleSum += simpleSum[i];
    if (simpleSum[i]>=maxSimpleSum)
      maxSimpleSum = simpleSum[i];
    if (simpleSum[i]<minSimpleSum)
      minSimpleSum = simpleSum[i];
  }
  //meanSimpleSum = meanSimpleSum / NUM_SAMPLES;
  maxMinDifference = maxSimpleSum - minSimpleSum;
  
  //SIMPLE ACTIVITY TRACKING
  if(maxMinDifference<=328){
    //User Immobile
    if(  ((state[0]+1)<pointThreshold) && ((state[1]-1)>0)  ){
      state[0]++;
      state[1]--;
    }
  }
  else{
    //User mobile
    if (  ((state[1]+20)<pointThreshold) && ((state[0]-pointStep)>0)  )  {
      state[1]=state[1]+pointStep;
      state[0]=state[0]-pointStep;
    }
      
  }   
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "mean = %d | max = %d | min = %d || difference = %d ------======------======-------",meanSimpleSum,maxSimpleSum,minSimpleSum,maxMinDifference); 
    
  //SIMPLE FALL DETECTION
  
  //stage 4
  if(highlyPossibleFall && (maxMinDifference<=maxMinThreshold)){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FALL DETECTED ");
    vibes_short_pulse();
    text_layer_set_text(fall_layer, "Are you alright?");
    appTimer = app_timer_register(30000,appTimerCallback,NULL);
  }
  highlyPossibleFall = false;
  
  //stage 3
  if(possibleFall && (maxMinDifference<=2*maxMinThreshold)){
    highlyPossibleFall = true;
  }
  possibleFall = false;
  
  //stage 2
  /*if(highPeak && (maxSimpleSum<fallThreshold)){
    possibleFall = true;  
  }
  highPeak = false;*/
  if(highPeak && (maxMinDifference<fallThreshold)){
    possibleFall = true;  
  }
  highPeak = false;
  
  //stage 1
  /*if(maxSimpleSum>=fallThreshold){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "High peak noticed %d",maxSimpleSum);
    highPeak = true;
  }*/
  if(maxMinDifference>=fallThreshold){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "High peak noticed %d",maxSimpleSum);
    highPeak = true;
  }
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "counter is %d",counter);
  counter++;
  
  if(counter==60){
    counter=0;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "reseting counter");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "asleep: %d times",state[0]);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "awake: %d times",state[1]);    
    if(state[0]>state[1]){
      APP_LOG(APP_LOG_LEVEL_DEBUG, "User is asleep/resting");
      APP_LOG(APP_LOG_LEVEL_DEBUG, "initialTimestamp = %d%d",timestampFirstHalf,timestampSecondHalf);
      if(previousState==1){
        APP_LOG(APP_LOG_LEVEL_DEBUG, "USING BLUETOOTH");
        sendStateChange(timestampFirstHalf,timestampSecondHalf,0);
      }
      previousState=0;
    }
    else{
      APP_LOG(APP_LOG_LEVEL_DEBUG, "User is awake");
      APP_LOG(APP_LOG_LEVEL_DEBUG, "initialTimestamp = %d %d",timestampFirstHalf,timestampSecondHalf);
      if(previousState==0){
        APP_LOG(APP_LOG_LEVEL_DEBUG, "USING BLUETOOTH");
        sendStateChange(timestampFirstHalf,timestampSecondHalf,1);
      }
      previousState=1;
    }
  }
  
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
	
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
  
  tuple = dict_find(received, THRESHOLD_KEY);
  if(tuple) {
    fallThreshold = atoi(tuple->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %d", fallThreshold);
	}
  
  tuple = dict_find(received, SENSITIVITY_KEY);
  if(tuple) {
    maxMinThreshold = atoi(tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %d", maxMinThreshold);
	}
  
  tuple = dict_find(received, POINT_THRESHOLD_KEY);
  if(tuple) {
    pointThreshold = atoi(tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %d", pointThreshold);
	}
  
  tuple = dict_find(received, STEP_KEY);
  if(tuple) {
    pointStep = atoi(tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %d", pointStep);
	}
  
  send_message();
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

static void out_sent_handler(DictionaryIterator *iter, void *context) {
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 45), window_bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
 
  s_battery_layer = text_layer_create(GRect(0, 90, window_bounds.size.w - 10, window_bounds.size.h-140));
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_battery_layer, "No data yet.");
  text_layer_set_overflow_mode(s_battery_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  fall_layer = text_layer_create(GRect(0, 110, window_bounds.size.w - 10, window_bounds.size.h));
  text_layer_set_font(fall_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  //text_layer_set_text(fall_layer, "Are you alright?");
  text_layer_set_overflow_mode(fall_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(fall_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(fall_layer));
    
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_time_layer);
}


static void init(void) {
	s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });  
  window_set_click_config_provider(s_window, click_config_provider);  
	window_stack_push(s_window, true);
	update_time();
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
  
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
  app_message_register_outbox_sent(out_sent_handler);
  
  accel_data_service_subscribe(NUM_SAMPLES, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 128;
  const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
  
  //initialization: 0->asleep | 1->awake 
  state[0]=pointThreshold;
  state[1]=0;

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
	app_message_deregister_callbacks();
  accel_data_service_unsubscribe();
	window_destroy(s_window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}