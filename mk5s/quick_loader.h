/* † MKKKKKS † */
/*!	@brief  Quick File Loader
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#ifndef QUICK_LOADER_H__
#define QUICK_LOADER_H__

#include <stddef.h>
#include <stdbool.h>

//! QLoad header block 
typedef struct QLoadedHead_ QLoadedHead;
//! QLoad footer block 
typedef struct QLoadedFoot_ QLoadedFoot;
//! QLoad instance 
typedef QLoadedHead QLoaded;

struct QLoadedHead_{
	size_t filesize; //!< file size 
	size_t readsize; //!< read size 
};

/*!	@note loaded text file can use as standard C string
*/
struct QLoadedFoot_{
	char zero[8]; //!< fill with zero 
};

#ifdef __cplusplus
extern "C" {
#endif

//! load a file 
/*!	@param path  target file
	@param shortable  allow incompleted load
	@return  header pointer of loaded file (or NULL)
*/
QLoaded* qload(const char* path,bool shortable);

//! free QLoaded file 
/*!	@param instp  pointer of stored from qload()
	@note  free target memory and instance pointer be NULL
*/
void qunload(QLoaded** instp);

//! get first pointer of loaded body 
inline void* qloaded_bgn(QLoaded* inst){
	return inst+1;
}

//! get terminate pointer of loaded body 
inline void* qloaded_end(QLoaded* inst){
	return (char*)qloaded_bgn(inst)+inst->readsize;
}

//! get footer pointer of loaded body 
inline QLoadedFoot* qloaded_foot(QLoaded* inst){
	return (QLoadedFoot*)((char*)qloaded_bgn(inst)+inst->filesize);
}

#ifdef __cplusplus
}
#endif

#endif // QUICK_LOADER_H__
