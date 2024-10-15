/* † MKKKKKS † */
/*!	@brief  Quick Path Parser
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#ifndef QUICK_PATH_H__
#define QUICK_PATH_H__

#include "./quick_text.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct QPathInfo_ QPathInfo;

struct QPathInfo_{
	QTextRef scheme;
	QTextRef base;
	QTextRef route;
};

//! setting of split by backslash 
/*!	@note for Winfdows, set true
*/
extern bool g_qpath_backslashable;

//! check a path is empty 
/*!	@return the path is empty.
*/
inline bool qpath_is_empty(const QPathInfo* src){
	if(!src)return true;
	if(!qtext_is_empty(&src->scheme))return false;
	if(!qtext_is_empty(&src->base))return false;
	if(!qtext_is_empty(&src->route))return false;
	return true;
}

//! check a path is absolute 
/*!	@return the path is located absolutely.
	@note scheme is optional.
*/
inline bool qpath_is_absolute(const QPathInfo* src){
	if(!src)return false;
	if(qtext_len(&src->base)<1)return false;
	return true;
}

//! check a path is relative 
/*!	@return the path is relative.
	@note it means connectable to base directory.
*/
inline bool qpath_is_relative(const QPathInfo* src){
	if(!src)return false;
	if(qtext_len(&src->scheme)>0)return false;
	if(qtext_len(&src->base)>0)return false;
	return true;
}

#ifdef __cplusplus
extern "C" {
#endif

//! clear a QPathInfo 
void qpath_clear(QPathInfo* dst);

//! setup a QPathInfo 
/*!	@param dst  setup target
	@param path  source path
*/
void qpath_setup_q(QPathInfo* dst,const QTextRef* path);

//! extract extension from a path route
bool qpath_extension_q(QTextRef* dst,const QTextRef* src,bool polydot);
//! extract dirname from a path route
bool qpath_dirname_q(QTextRef* dst,const QTextRef* src);
//! extract filename from a path route
bool qpath_filename_q(QTextRef* dst,const QTextRef* src);
//! extract filename from a path route without extension
bool qpath_filename_noext_q(QTextRef* dst,const QTextRef* src,bool polydot);

#ifdef __cplusplus
}
#endif

inline void qpath_setup_c(QPathInfo* dst,const char* path){
	QTextRef path_q;
	qtext_ref_c(&path_q,path);
	qpath_setup_q(dst,&path_q);
}

inline bool qpath_extension_c(QTextRef* dst,const char* src,bool polydot){
	QTextRef src_q;
	qtext_ref_c(&src_q,src);
	return qpath_extension_q(dst,&src_q,polydot);
}

inline bool qpath_dirname_c(QTextRef* dst,const char* src){
	QTextRef src_q;
	qtext_ref_c(&src_q,src);
	return qpath_dirname_q(dst,&src_q);
}

inline bool qpath_filename_c(QTextRef* dst,const char* src){
	QTextRef src_q;
	qtext_ref_c(&src_q,src);
	return qpath_filename_q(dst,&src_q);
}

inline bool qpath_filename_noext_c(QTextRef* dst,const char* src,bool polydot){
	QTextRef src_q;
	qtext_ref_c(&src_q,src);
	return qpath_filename_noext_q(dst,&src_q,polydot);
}

#endif // QUICK_PATH_H__
