#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1 // for fopencookie hack in serialize_size
#endif

#include <stdlib.h>

#include "libretro.h"
#include "../gb_core/gb.h"
#include "dmy_renderer.h"
#include "../mk5s/advanced_m3u.h"
#include "../mk5s/quick_loader.h"
#include "../mk5s/quick_path.h"

#define CUSTOM_VERSION "+NC38"

#define RETRO_MEMORY_GAMEBOY_1_SRAM ((1 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_GAMEBOY_1_RTC ((2 << 8) | RETRO_MEMORY_RTC)
#define RETRO_MEMORY_GAMEBOY_2_SRAM ((3 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_GAMEBOY_2_RTC ((3 << 8) | RETRO_MEMORY_RTC)

#define RETRO_GAME_TYPE_GAMEBOY_LINK_2P 0x101

#define MAX_ROM_VARIANT 8

#define RETRO_DEVICE_JOYPAD_NORMAL   RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_JOYPAD, 0 )
#define RETRO_DEVICE_JOYPAD_DUAL     RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_JOYPAD, 1 )

static const struct retro_variable vars_single[] = {
    { "tgbdual_gblink_enable", "Link cable emulation (reload); disabled|enabled" },
    { "tgbdual_turbo_speed_a", "Turbo Speed for Button A; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_b", "Turbo Speed for Button B; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_start", "Turbo Speed for Button Start; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_select", "Turbo Speed for Button Select; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_ratio", "Turbo Button Pressing Ratio; 5|6|7|8|9|10|11|12|13|14|15|1|2|3|4" },
    { NULL, NULL },
};

static const struct retro_variable vars_dual[] = {
    { "tgbdual_gblink_enable", "Link cable emulation (reload); disabled|enabled" },
    { "tgbdual_screen_placement", "Screen layout; left-right|top-down" },
    { "tgbdual_switch_screens", "Switch player screens; normal|switched" },
    { "tgbdual_single_screen_mp", "Show player screens; both players|player 1 only|player 2 only" },
    { "tgbdual_audio_output", "Audio output; Game Boy #1|Game Boy #2|Both Mix|Muted" },
    { "tgbdual_turbo_speed_a", "Turbo Speed for Button A; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_b", "Turbo Speed for Button B; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_start", "Turbo Speed for Button Start; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_speed_select", "Turbo Speed for Button Select; 5|6|7|8|9|10|11|12|13|14|15|16|1|2|3|4" },
    { "tgbdual_turbo_ratio", "Turbo Button Pressing Ratio; 5|6|7|8|9|10|11|12|13|14|15|1|2|3|4" },
    { NULL, NULL },
};

static const struct retro_subsystem_memory_info gb1_memory[] = {
    { "srm", RETRO_MEMORY_GAMEBOY_1_SRAM },
    { "rtc", RETRO_MEMORY_GAMEBOY_1_RTC },
};

static const struct retro_subsystem_memory_info gb2_memory[] = {
    { "srm", RETRO_MEMORY_GAMEBOY_2_SRAM },
    { "rtc", RETRO_MEMORY_GAMEBOY_2_RTC },
};

static const struct retro_subsystem_rom_info gb_roms[] = {
    { "GameBoy #1", "gb|gbc", false, false, false, gb1_memory, 1 },
    { "GameBoy #2", "gb|gbc", false, false, false, gb2_memory, 1 },
};

   static const struct retro_subsystem_info subsystems[] = {
      { "2 Player Game Boy Link", "gb_link_2p", gb_roms, 2, RETRO_GAME_TYPE_GAMEBOY_LINK_2P },
      { NULL },
};

enum mode{
    MODE_SINGLE_GAME,
    MODE_SINGLE_GAME_DUAL,
    MODE_DUAL_GAME
};

static enum mode mode = MODE_SINGLE_GAME;

gb *g_gb[2];
dmy_renderer *render[2];
QLoaded* g_gb_rom[MAX_ROM_VARIANT]={NULL};

retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

extern bool _screen_2p_vertical;
extern bool _screen_switched;
extern int _show_player_screens;
static size_t _serialize_size[2]         = { 0, 0 };
static bool gblinked[2]={false,false};

static struct retro_disk_control_ext2_callback dskcb;
static unsigned diskidx=0;

bool gblink_enable                       = false;
bool dual_control                        = false;
int audio_2p_mode = 1; // player 1 side 
bool audio_2p_mode_switched=false; // indicate controlled in renderer 
// used to make certain core options only take effect once on core startup
bool already_checked_options             = false;
bool libretro_supports_persistent_buffer = false;
bool libretro_supports_bitmasks          = false;
struct retro_system_av_info *my_av_info  = (retro_system_av_info*)malloc(sizeof(*my_av_info));

TurboConfig turboConfig[TURBO_BUTTONS]={
	{0x2800,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_X,"tgbdual_turbo_speed_a"},
	{0x2800,RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_Y,"tgbdual_turbo_speed_b"},
	{0x2800,RETRO_DEVICE_ID_JOYPAD_L,RETRO_DEVICE_ID_JOYPAD_L2,"tgbdual_turbo_speed_l"},
	{0x2800,RETRO_DEVICE_ID_JOYPAD_R,RETRO_DEVICE_ID_JOYPAD_R2,"tgbdual_turbo_speed_r"},
	{0x2800,RETRO_DEVICE_ID_JOYPAD_START,RETRO_DEVICE_ID_JOYPAD_G2,"tgbdual_turbo_speed_start"},
	{0x2800,RETRO_DEVICE_ID_JOYPAD_SELECT,RETRO_DEVICE_ID_JOYPAD_G1,"tgbdual_turbo_speed_select"},
};
unsigned turbo_ratio=0x8000;

static AdvancedM3U *g_am3u=NULL;
static AdvancedM3UDevice *g_am3u_rom=NULL;

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name     = "TGB Dual";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v0.8.3" GIT_VERSION CUSTOM_VERSION;
   info->need_fullpath    = false;
   info->valid_extensions = "gb|dmg|gbc|cgb|sgb|m3u";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   int w = 160, h = 144;
   info->geometry.max_width = w * 2;
   info->geometry.max_height = h * 2;

   if (/*g_gb[1] &&*/ _show_player_screens == 2)
   {
      // screen orientation for dual gameboy mode
      if(_screen_2p_vertical)
         h *= 2;
      else
         w *= 2;
   }

   info->timing.fps            = 4194304.0 / 70224.0;
   info->timing.sample_rate    = 44100.0f;
   info->geometry.base_width   = w;
   info->geometry.base_height  = h;
   info->geometry.aspect_ratio = float(w) / float(h);
   memcpy(my_av_info, info, sizeof(*my_av_info));
}

static bool gb_startup(int slot, byte* data,size_t size,bool persistent)
{
   if(!render[slot]) render[slot] = new dmy_renderer(slot);
   if(!g_gb[slot]) g_gb[slot]   = new gb(render[slot], true, true);
	if(!g_gb[slot]->load_rom(data, size, NULL, 0, persistent))return false;

	if(gblink_enable && g_gb[1-slot]){
		gblinked[slot]=true;
		g_gb[slot]->set_target(g_gb[1-slot]);
		if(!gblinked[1-slot]){
			gblinked[1-slot]=true;
			g_gb[1-slot]->set_target(g_gb[slot]);
		}
	}

	return true;
}

static void gb_shutdown(int slot,bool reset)
{
	if (!g_gb[slot]) return;

	if(gblinked[1-slot]){
		gblinked[1-slot]=false;
		g_gb[1-slot]->set_target(NULL);
	}
	if(gblinked[slot]){
		gblinked[slot]=false;
		g_gb[slot]->set_target(NULL);
	}

	delete g_gb[slot];
	g_gb[slot] = NULL;
	delete render[slot];
	render[slot] = NULL;

	if(!reset)return;

   if(!render[slot]) render[slot] = new dmy_renderer(slot);
   if(!g_gb[slot]) g_gb[slot]   = new gb(render[slot], true, true);
}

static bool set_drive_eject_state(unsigned drv, bool ejected)
{
	if(!g_am3u_rom)return false;

	if(ejected){
		gb_shutdown(drv,true);
		am3u_device_remove_media(g_am3u_rom,drv);
	}
	else{
		am3u_device_set_media(g_am3u_rom,drv,diskidx);

		int romidx=g_am3u_rom->slot_tbl[drv];
		if(!gb_startup(drv, (byte*)qloaded_bgn(g_gb_rom[romidx]), g_gb_rom[romidx]->readsize, libretro_supports_persistent_buffer))return false;
	}
	return true;
}

bool get_drive_eject_state(unsigned drv)
{
	if(!g_am3u_rom)return false;

   return !am3u_device_get_media(g_am3u_rom,drv);
}

static unsigned get_image_index(void)
{
   return diskidx;
}

static bool set_image_index(unsigned index)
{
   diskidx = index;
   return true;
}

static unsigned get_num_drives(void)
{
   return 2;
}

static unsigned get_num_images(void)
{
	if(!g_am3u_rom)return 1;

   return g_am3u_rom->changee_used;
}

static bool disk_get_image_path(unsigned index, char *path, size_t len)
{
   if (len < 1) return false;
	if(!g_am3u_rom)return false;

	AdvancedM3UMedia* media=&g_am3u_rom->changee_tbl[index];
	if(!media)return false;
	if(!media->path)return false;

    strncpy(path, media->path, len);
    return true;
}

static bool disk_get_image_label(unsigned index, char *label, size_t len)
{
   if (len < 1) return false;
	if(!g_am3u_rom)return false;

	AdvancedM3UMedia* media=&g_am3u_rom->changee_tbl[index];
	if(!media)return false;
	char* src=media->label;
	if(!src)src=media->path;
	if(!src)return false;

    strncpy(label, src, len);
    return true;
}

static int disk_get_drive_image_index(unsigned drive)
{
	if(drive>=get_num_drives())return -1;
	if(get_drive_eject_state(drive))return -1;
	if(!g_am3u_rom)return -1;

	return g_am3u_rom->slot_tbl[drive];	
}

void attach_disk_swap_interface(void)
{
   memset(&dskcb,0,sizeof(dskcb));
   dskcb.set_drive_eject_state = set_drive_eject_state;
   dskcb.get_drive_eject_state = get_drive_eject_state;
   dskcb.set_image_index = set_image_index;
   dskcb.get_image_index = get_image_index;
   dskcb.get_num_drives  = get_num_drives;
   dskcb.get_num_images  = get_num_images;
   dskcb.get_image_path = disk_get_image_path;
   dskcb.get_image_label = disk_get_image_label;
   dskcb.get_drive_image_index = disk_get_drive_image_index;

   environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT2_INTERFACE, &dskcb);
}

void retro_init(void)
{
   unsigned level = 4;
   struct retro_log_callback log;

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   attach_disk_swap_interface();
}

void retro_deinit(void)
{
   libretro_supports_bitmasks          = false;
}

static void check_variables(void)
{
   struct retro_variable var;

   // check whether link cable mode is enabled
   var.key = "tgbdual_gblink_enable";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!already_checked_options) { // only apply this setting on init
         if (!strcmp(var.value, "disabled"))
            gblink_enable = false;
         else if (!strcmp(var.value, "enabled"))
            gblink_enable = true;
      }
   }
   else
      gblink_enable = false;

   // check whether screen placement is horz (side-by-side) or vert
   var.key = "tgbdual_screen_placement";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "left-right"))
         _screen_2p_vertical = false;
      else if (!strcmp(var.value, "top-down"))
         _screen_2p_vertical = true;
   }
   else
      _screen_2p_vertical = false;

   // check whether player 1 and 2's screen placements are swapped
   var.key = "tgbdual_switch_screens";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "normal"))
         _screen_switched = false;
      else if (!strcmp(var.value, "switched"))
         _screen_switched = true;
   }
   else
      _screen_switched = false;

   // check whether to show both players' screens, p1 only, or p2 only
   var.key = "tgbdual_single_screen_mp";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "both players"))
         _show_player_screens = 2;
      else if (!strcmp(var.value, "player 1 only"))
         _show_player_screens = 0;
      else if (!strcmp(var.value, "player 2 only"))
         _show_player_screens = 1;
   }
   else
      _show_player_screens = 2;

   int screenw = 160, screenh = 144;
   if (/*gblink_enable &&*/ _show_player_screens == 2)
   {
      if (_screen_2p_vertical)
         screenh *= 2;
      else
         screenw *= 2;
   }
   my_av_info->geometry.base_width = screenw;
   my_av_info->geometry.base_height = screenh;
   my_av_info->geometry.aspect_ratio = float(screenw) / float(screenh);

   already_checked_options = true;
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, my_av_info);

   // check whether player 1 and 2's screen placements are swapped
   var.key = "tgbdual_audio_output";
   var.value = NULL;
	if(audio_2p_mode_switched){
		// apply from control at this frame 
		audio_2p_mode_switched=false;
		switch(audio_2p_mode&3){
			case 0: var.value="Muted"; break;
			case 1: var.value="Game Boy #1"; break;
			case 2: var.value="Game Boy #2"; break;
			case 3: var.value="Both Mix"; break;
		}
		environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var);
	}
	else if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		// apply from config 
		if (!strcmp(var.value, "Game Boy #1"))
		 audio_2p_mode = 1;
		else if (!strcmp(var.value, "Game Boy #2"))
		 audio_2p_mode = 2;
		else if (!strcmp(var.value, "Both Mix"))
		 audio_2p_mode = 3;
		else audio_2p_mode = 0;
	}

	// turbo speed 
	for(TurboConfig* tc=&turboConfig[0];tc<&turboConfig[TURBO_BUTTONS];++tc){
		var.key = tc->config;
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			tc->speed=atoi(var.value)*0x800;
		}
		else tc->speed=0x2800;
	}

	var.key = "tgbdual_turbo_ratio";
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		turbo_ratio=0x10000-(atoi(var.value)*0x1000)&0xffff;
	}
	else turbo_ratio=0x8000;
}

static bool am3u_error(void* user,int code,int lineloc,const QTextRef* line){

   if (!log_cb) return true;

	char* msg=qtext_alloc_q(line);

	  log_cb(RETRO_LOG_ERROR,
                      "M3U error %d in line %d: %s\n",
                      code,lineloc,msg);
	
	qtext_free(&msg);

	return true;
}

void set_input_desc()
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G1,    "Turbo Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G2,    "Turbo Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "GB#1 Toggle Audio" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "GB#2 Toggle Audio" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G1,    "Turbo Select" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G2,    "Turbo Start" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

void set_input_desc_dual()
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "GB#1 D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "GB#1 D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "GB#1 D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "GB#1 D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L0,    "GB#1 A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "GB#1 B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"GB#1 Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G1,    "GB#1 Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L4,    "GB#1 Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L5,    "GB#1 Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G5,    "GB#1 Turbo Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G3,    "GB#1 Turbo Start" },

      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "GB#2 D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "GB#2 D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "GB#2 D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "GB#2 D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R0,    "GB#2 A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "GB#2 B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "GB#2 Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G2,    "GB#2 Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R4,    "GB#2 Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R5,    "GB#2 Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G6,    "GB#2 Turbo Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_G4,    "GB#2 Turbo Start" },

      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "GB#1 Toggle Audio" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "GB#2 Toggle Audio" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

bool retro_load_game(const struct retro_game_info *info)
{
   size_t rom_size;
   byte *rom_data;
   const struct retro_game_info_ext *info_ext = NULL;
	int romidx;

   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars_single);
   check_variables();

   unsigned i;

   if (!info)
      return false;

	set_input_desc();

	QTextRef ext;
	if(!qpath_extension_c(&ext,info->path,false));
	else if(!strcasecmp(qtext_bgn(&ext),"m3u")){
		QLoaded* img=qload(info->path,false);
		if(img){
			QTextRef m3udir;
			qpath_dirname_c(&m3udir,info->path);
			QTextRef imgref;
			qtext_ref_q(&imgref,(const char*)qloaded_bgn(img),img->readsize);
			g_am3u=am3u_new();
			am3u_set_default_device(g_am3u,'R');
			g_am3u_rom=am3u_get_device(g_am3u,'R');
			am3u_device_set_changer(g_am3u_rom,MAX_ROM_VARIANT);
			am3u_device_set_slots(g_am3u_rom,2);
			am3u_setup_q(g_am3u,&imgref,&m3udir,am3u_error,NULL);
			qunload(&img);

			for(i=0;i<MAX_ROM_VARIANT;++i){
				if(g_am3u_rom->changee_tbl[i].path)
					g_gb_rom[i]=qload(g_am3u_rom->changee_tbl[i].path,false);
				else g_gb_rom[i]=NULL;
			}
			for(i=0;i<2;++i){
				if(i>=g_am3u_rom->changee_used)continue;
				if(log_cb)log_cb(RETRO_LOG_INFO,
                      "ROM slot %d: %s\n",i+1,g_am3u_rom->changee_tbl[i].path);
			}			
		}
	}

   for (i = 0; i < 2; i++)
   {
      g_gb[i]   = NULL;
      render[i] = NULL;
   }

   for (i = 0; i < 2; i++)
      _serialize_size[i] = 0;

	if(g_am3u_rom){
	      mode      = MODE_DUAL_GAME;
	      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars_dual);

		romidx=g_am3u_rom->slot_tbl[0];
		if(romidx>=0)
		{
			if(!gb_startup(0, (byte*)qloaded_bgn(g_gb_rom[romidx]), g_gb_rom[romidx]->readsize, libretro_supports_persistent_buffer))return false;
		}
		romidx=g_am3u_rom->slot_tbl[1];
		if(romidx>=0)
		{
			if(!gb_startup(1, (byte*)qloaded_bgn(g_gb_rom[romidx]), g_gb_rom[romidx]->readsize, libretro_supports_persistent_buffer))return false;
		}
	}
	else{
	   if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext) &&
	       info_ext->persistent_data)
	   {
	      rom_data                            = (byte*)info_ext->data;
	      rom_size                            = info_ext->size;
	      libretro_supports_persistent_buffer = true;
	   }
	   else
	   {
	      rom_data                            = (byte*)info->data;
	      rom_size                            = info->size;
	   }

		if(!gb_startup(0, rom_data, rom_size, libretro_supports_persistent_buffer))return false;
	   if (gblink_enable)
	   {
	      mode      = MODE_SINGLE_GAME_DUAL;
	      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars_dual);

			if(!gb_startup(1, rom_data, rom_size, libretro_supports_persistent_buffer))return false;
	   }
	   else
	      mode = MODE_SINGLE_GAME;
	}

   check_variables();

   return true;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num_info)
{
    if (type != RETRO_GAME_TYPE_GAMEBOY_LINK_2P)
        return false; /* all other types are unhandled for now */

   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars_dual);
   unsigned i;

	set_input_desc();

   if (!info)
      return false;

   for (i = 0; i < 2; i++)
   {
      g_gb[i]   = NULL;
      render[i] = NULL;
   }

   check_variables();

   for (i = 0; i < 2; i++)
      _serialize_size[i] = 0;

	if(!gb_startup(0, (byte*)info[0].data, info[0].size, false))return false;

   if (gblink_enable)
   {
		if(!gb_startup(1, (byte*)info[1].data, info[1].size, false))return false;
   }

   mode = MODE_DUAL_GAME;
   return true;
}


void retro_unload_game(void)
{
   unsigned i;
   for(i = 0; i < 2; ++i)
   {
	gb_shutdown(i,false);
   }
   free(my_av_info);
   libretro_supports_persistent_buffer = false;

	for(i=0;i<MAX_ROM_VARIANT;++i){
		if(g_gb_rom[i])qunload(&g_gb_rom[i]);
	}

	g_am3u_rom=NULL;
	if(g_am3u)am3u_free(&g_am3u);
}

void retro_reset(void)
{
   for(int i = 0; i < 2; ++i)
   {
      if (g_gb[i])
         g_gb[i]->reset();
   }
}

void retro_run(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();

   input_poll_cb();

   for (int line = 0;line < 154; line++)
   {
      if (g_gb[0])
         g_gb[0]->run();
      if (g_gb[1])
         g_gb[1]->run();
   }
}



void *retro_get_memory_data(unsigned id)
{
    switch(mode)
    {
        case MODE_SINGLE_GAME:
        case MODE_SINGLE_GAME_DUAL: /* todo: hook this properly */
        {
            switch(id)
            {
                case RETRO_MEMORY_SAVE_RAM:
                    return g_gb[0]->get_rom()->get_sram();
                case RETRO_MEMORY_RTC:
                    return &(render[0]->fixed_time);
                case RETRO_MEMORY_VIDEO_RAM:
                    return g_gb[0]->get_cpu()->get_vram();
                case RETRO_MEMORY_SYSTEM_RAM:
                    return g_gb[0]->get_cpu()->get_ram();
                default:
                    break;
            }
        }
        case MODE_DUAL_GAME:
        {
            switch(id)
            {
                case RETRO_MEMORY_GAMEBOY_1_SRAM:
                    return g_gb[0]->get_rom()->get_sram();
                case RETRO_MEMORY_GAMEBOY_1_RTC:
                    return &(render[0]->fixed_time);
                case RETRO_MEMORY_GAMEBOY_2_SRAM:
                    return g_gb[1]->get_rom()->get_sram();
                case RETRO_MEMORY_GAMEBOY_2_RTC:
                    return &(render[1]->fixed_time);
                default:
                    break;
            }
        }
    }
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    switch(mode)
    {
        case MODE_SINGLE_GAME:
        case MODE_SINGLE_GAME_DUAL: /* todo: hook this properly */
        {
            switch(id)
            {
                case RETRO_MEMORY_SAVE_RAM:
                    return g_gb[0]->get_rom()->get_sram_size();
                case RETRO_MEMORY_RTC:
                    return sizeof(render[0]->fixed_time);
                case RETRO_MEMORY_VIDEO_RAM:
                    if (g_gb[0]->get_rom()->get_info()->gb_type >= 3)
                        return 0x2000*2; //sizeof(cpu::vram);
                    return 0x2000;
                case RETRO_MEMORY_SYSTEM_RAM:
                    if (g_gb[0]->get_rom()->get_info()->gb_type >= 3)
                        return 0x2000*4; //sizeof(cpu::ram);
                    return 0x2000;
                default:
                    break;
            }
        }
        case MODE_DUAL_GAME:
        {
            switch(id)
            {
                case RETRO_MEMORY_GAMEBOY_1_SRAM:
                    return g_gb[0]->get_rom()->get_sram_size();
                case RETRO_MEMORY_GAMEBOY_1_RTC:
                    return sizeof(render[0]->fixed_time);
                case RETRO_MEMORY_GAMEBOY_2_SRAM:
                    return g_gb[1]->get_rom()->get_sram_size();
                case RETRO_MEMORY_GAMEBOY_2_RTC:
                    return sizeof(render[1]->fixed_time);
                default:
                    break;
            }
        }
    }
   return 0;
}



// question: would saving both gb's into the same file be desirable ever?
// answer: yes, it's most likely needed to sync up netplay and for bsv records.
size_t retro_serialize_size(void)
{
      unsigned i;

      for(i = 0; i < 2; ++i)
      {
		_serialize_size[i] = g_am3u_rom?1:0;
         if (g_gb[i])
            _serialize_size[i] += g_gb[i]->get_state_size();
      }
   return _serialize_size[0] + _serialize_size[1];
}

bool retro_serialize(void *data, size_t size)
{
   if (size == retro_serialize_size())
   {
      unsigned i;
      uint8_t *ptr = (uint8_t*)data;

      for(i = 0; i < 2; ++i)
      {
		if(g_am3u_rom){
			*ptr=g_am3u_rom->slot_tbl[i];
			++ptr;
		}

         if (g_gb[i])
         {
            g_gb[i]->save_state_mem(ptr);
            ptr += _serialize_size[i];
         }
      }

      return true;
   }
   return false;
}

bool retro_unserialize(const void *data, size_t size)
{
   if (size == retro_serialize_size())
   {
      unsigned i;
      uint8_t *ptr = (uint8_t*)data;

      for(i = 0; i < 2; ++i)
      {
		if(g_am3u_rom){
			gb_shutdown(i,true);
			g_am3u_rom->slot_tbl[i]=*ptr;
			am3u_device_set_media(g_am3u_rom,i,*ptr);
			if(*ptr>=0){
				if(!gb_startup(i, (byte*)qloaded_bgn(g_gb_rom[*ptr]), g_gb_rom[*ptr]->readsize, libretro_supports_persistent_buffer)){
				   if (log_cb)
				      log_cb(RETRO_LOG_ERROR, "unserialize: cannot start rom idx=%d\n", *ptr);
				}
			}
			++ptr;
		}

         if (g_gb[i])
         {
            g_gb[i]->restore_state_mem(ptr);
            ptr += _serialize_size[i];
         }
      }
      return true;
   }
   return false;
}



void retro_cheat_reset(void)
{
   for(int i=0; i<2; ++i)
   {
      if(g_gb[i])
         g_gb[i]->get_cheat()->clear();
   }
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
#if 1==0
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "CHEAT:  id=%d, enabled=%d, code='%s'\n", index, enabled, code);
   // FIXME: work in progress.
   // As it stands, it seems TGB Dual only has support for Gameshark codes.
   // Unfortunately, the cheat.xml that ships with bsnes seems to only have
   // Game Genie codes, which are ROM patches rather than RAM.
   // http://nocash.emubase.de/pandocs.htm#gamegeniesharkcheats
   if(false && g_gb[0])
   {
      cheat_dat cdat;
      cheat_dat *tmp=&cdat;

      strncpy(cdat.name, code, sizeof(cdat.name));

      tmp->enable = true;
      tmp->next = NULL;

      while(false)
      { // basically, iterate over code.split('+')
         // TODO: remove all non-alnum chars here
         if (false)
         { // if strlen is 9, game genie
            // game genie format: for "ABCDEFGHI",
            // AB   = New data
            // FCDE = Memory address, XORed by 0F000h
            // GIH  = Check data (can be ignored for our purposes)
            word scramble;
            sscanf(code, "%2hhx%4hx", &tmp->dat, &scramble);
            tmp->code = 1; // TODO: test if this is correct for ROM patching
            tmp->adr = (((scramble&0xF) << 12) ^ 0xF000) | (scramble >> 4);
         }
         else if (false)
         { // if strlen is 8, gameshark
            // gameshark format for "ABCDEFGH",
            // AB    External RAM bank number
            // CD    New Data
            // GHEF  Memory Address (internal or external RAM, A000-DFFF)
            byte adrlo, adrhi;
            sscanf(code, "%2hhx%2hhx%2hhx%2hhx", &tmp->code, &tmp->dat, &adrlo, &adrhi);
            tmp->adr = (adrhi<<8) | adrlo;
         }
         if(false)
         { // if there are more cheats left in the string
            tmp->next = new cheat_dat;
            tmp = tmp->next;
         }
      }
   }
   g_gb[0].get_cheat().add_cheat(&cdat);
#endif
}


// start boilerplate

unsigned retro_api_version(void) { return RETRO_API_VERSION; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void retro_set_controller_port_device(unsigned port, unsigned device) {

	// Player 1 only 
	if(port)return;

	if(device==RETRO_DEVICE_JOYPAD_DUAL){
		dual_control=true;
		set_input_desc_dual();
	}
	else{
		dual_control=false;
		set_input_desc();
	}
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }


void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_system_content_info_override content_overrides[] = {
      {
         "gb|dmg|gbc|cgb|sgb|m3u", /* extensions */
         false,    /* need_fullpath */
         true      /* persistent_data */
      },
      { NULL, false, false }
   };
   environ_cb = cb;
   cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);
   /* Request a persistent content data buffer */
   cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE,
         (void*)content_overrides);

	// controller port settiings 
   static const struct retro_controller_description port1[] = {
      { "RetroPad",              RETRO_DEVICE_JOYPAD },
      { "RetroPad Dual",         RETRO_DEVICE_JOYPAD_DUAL },
      { 0 },
   };
   static const struct retro_controller_description port2[] = {
      { "RetroPad",              RETRO_DEVICE_JOYPAD },
      { 0 },
   };

   static const struct retro_controller_info ports[] = {
      { port1, sizeof(port1)/sizeof(struct retro_controller_description) },
      { port2, sizeof(port2)/sizeof(struct retro_controller_description) },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

// end boilerplate
