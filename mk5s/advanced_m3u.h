/* † MKKKKKS † */
/*!	@brief  Advanced M3U Loader
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#ifndef ADVANCED_M3U_H__
#define ADVANCED_M3U_H__

#include "./quick_text.h"
#include <stddef.h>
#include <stdbool.h>

//! minimum device letter 
#define AM3U_DEVICE_MIN 'A'
//! maximum device letter 
#define AM3U_DEVICE_MAX 'Z'
//! device count 
#define AM3U_DEVICE_COUNT (AM3U_DEVICE_MAX-AM3U_DEVICE_MIN+1)

#define AM3U_ERROR_NONE 0
#define AM3U_ERROR_UNDEFINED 1
#define AM3U_ERROR_SYNTAX 2
#define AM3U_ERROR_INVALID_DEVICE 3
#define AM3U_ERROR_INVALID_SLOT 4
#define AM3U_ERROR_EMPTY_PATH 5
#define AM3U_ERROR_CHANGEE_OVER 6

//! Advanced M3U instance 
typedef struct AdvancedM3U_ AdvancedM3U;
//! each device in Advanced M3U 
typedef struct AdvancedM3UDevice_ AdvancedM3UDevice;
//! each media in Advanced M3U devices
typedef struct AdvancedM3UMedia_ AdvancedM3UMedia;

//! call by parse error 
/*!	@param user  any user pointer
	@param code  error code (AM3U_ERROR_*)
	@param lineloc  happened line number
	@param line  happened line
	@retval false  the caller function will aborted
	@retval true  the caller function is continueable
*/
typedef bool (*AdvancedM3UParseError)(void* user,int code,int lineloc,const QTextRef* line);

struct AdvancedM3UMedia_{
	bool ready;
	bool readonly;
	char* path;
	char* label;
};

struct AdvancedM3UDevice_{
	int changee_max; //!< array size of changee_tbl 
	int changee_used; //!< filled size in changee_tbl 
	AdvancedM3UMedia* changee_tbl;

	int slot_max; //!< array size of slot_tbl 
	int* slot_tbl; //!< selected changee index by slot 
};

struct AdvancedM3U_{
	int device_default; //!< device_tbl index by standard M3U line uses 
	AdvancedM3UDevice device_tbl[AM3U_DEVICE_COUNT];
};

#ifdef __cplusplus
extern "C" {
#endif

//! create an AdvancedM3U instance 
AdvancedM3U* am3u_new();
//! free target AdvancedM3U instance 
void am3u_free(AdvancedM3U** instp);
//! reset target AdvancedM3U instance 
void am3u_reset(AdvancedM3U* inst);

//! parse M3U source and put into target AdvancedM3U instance 
/*!	@param inst  target instance
	@param src  M3U source
	@param cberr  callback by each error
	@param user  any user pointer
	@retval false  procudure aborted
	@retval true  procudure completed
*/
bool am3u_setup_q(AdvancedM3U* inst,const QTextRef* src,const QTextRef* basedir,AdvancedM3UParseError cberr,void* user);

//! set default device for standard M3U lines 
/*!	@param inst  target AdvancedM3U instance
	@param device  device letter
	@retval false  failed
	@retval true  done
*/
bool am3u_set_default_device(AdvancedM3U* inst,char device);

//! get device instance 
/*!	@param inst  source AdvancedM3U instance
	@param device  device letter
	@return  device instance (or NULL)
*/
AdvancedM3UDevice* am3u_get_device(AdvancedM3U* inst,char device);

//! set media changer on target device 
/*!	@param inst  target device instance
	@param size  capacity of changees
*/
void am3u_device_set_changer(AdvancedM3UDevice* inst,int size);

//! set drive slots on target device 
/*!	@param inst  target device instance
	@param size  capacity of slots
*/
void am3u_device_set_slots(AdvancedM3UDevice* inst,int size);

//! add a media on target media changer and device slot 
/*!	@param inst  target device instance
	@param slot  set to device slot (1 origin, 0 is none)
	@param readonly  target media is read only
	@param path  media path
	@param label  custom label (nullable)
	@retval false  failed
	@retval true  done
*/
bool am3u_device_add_media(AdvancedM3UDevice* inst,int slot,bool readonly,const QTextRef* basedir,const QTextRef* path,const QTextRef* label);

//! get current media 
/*!	@param inst  target device instance
	@param slot  set to device slot (1 origin, 0 is none)
	@return  media instance
*/
AdvancedM3UMedia* am3u_device_get_media(AdvancedM3UDevice* inst,int slot);

//! set a media to a device slot 
/*!	@param inst  target device instance
	@param slot  set to device slot (1 origin, 0 is none)
	@return  is success
*/
bool am3u_device_set_media(AdvancedM3UDevice* inst,int slot,int idx);

#ifdef __cplusplus
}
#endif

inline bool am3u_setup_c(AdvancedM3U* inst,const char* src,const char* basedir,AdvancedM3UParseError cberr,void* user){
	QTextRef src_q,basedir_q;
	qtext_ref_c(&src_q,src);
	qtext_ref_c(&basedir_q,basedir);
	return am3u_setup_q(inst,&src_q,&basedir_q,cberr,user);
}

//! remove a media from a device slot 
inline bool am3u_device_remove_media(AdvancedM3UDevice* inst,int slot){
	return am3u_device_set_media(inst,slot,-1);
}

#endif // ADVANCED_M3U
