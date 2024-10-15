/* † MKKKKKS † */
/*!	@brief  Quick Text Utilities
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#ifndef QUICK_TEXT_H__
#define QUICK_TEXT_H__

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//! quick text reference 
typedef struct QTextRef_ QTextRef;

struct QTextRef_{
	const char* bgn;
	size_t len;
};

//! callback by text extracted
/*!	@param user  any user pointer
	@param src  source text reference
	@param attention  the text is not terminated and don't use for standard C string @n
		use qtext_alloc_q(src)
	@retval false  the caller function will aborted
	@retval true  the caller function is continueable
*/
typedef bool (*QTextExtracted)(void* user,const QTextRef* src);

//! check a QTextRef empty 
inline bool qtext_is_empty(const QTextRef* src){
	if(!src)return true;
	return src->len<1;
}
//! get length of a QTextRef 
inline size_t qtext_len(const QTextRef* src){
	if(!src)return 0;
	return src->len;
}
//! get beginside of a QTextRef 
inline const char* qtext_bgn(const QTextRef* src){
	if(!src)return NULL;
	return src->bgn;
}
//! get endside of a QTextRef 
inline const char* qtext_end(const QTextRef* src){
	if(!src)return NULL;
	return src->bgn+src->len;
}

#ifdef __cplusplus
extern "C" {
#endif

//! clear an instance 
void qtext_clear(QTextRef* dst);

//! create reference from a pointer and length 
void qtext_ref_q(QTextRef* dst,const char* bgn,size_t len);
//! create reference from standard C string 
void qtext_ref_c(QTextRef* dst,const char* src);

//! create a standard C string from memory range 
/*!	@param bgn  first letter pointer of target text
	@param len  text length
	@return  pointer of copied standard C string
*/
char* qtext_copy(const char* bgn,size_t len);

//! create a standard C string with combining QText 
#define qtext_combine_q(...) qtext_combine_q_(NULL,##__VA_ARGS__,NULL)
char* qtext_combine_q_(const void* dmy,/*const QTextRef* src*/.../*,always NULL*/);

//! create a standard C string with combining standard C strings 
#define qtext_combine_c(...) qtext_combine_c_(NULL,##__VA_ARGS__,NULL)
char* qtext_combine_c_(const void* dmy,/*const char* src*/.../*,always NULL*/);

//! free allocated string 
/*!	@param instp  pointer of stored from qtext_alloc()
	@note  free target memory and instance pointer be NULL
*/
void qtext_free(char** instp);

//! extract lines from a QText 
/*!	@param user  any user pointer
	@param src  source text
	@param cbext  call by each extracted line
	@return  pointer of processed in the function
*/
const char* qtext_ext_lines_q(void* user,const QTextRef* src,QTextExtracted cbext);

//! trim from fore of a string 
void qtext_trim_fore_q(QTextRef* target,const QTextRef* letters);
//! trim from back of a string 
void qtext_trim_back_q(QTextRef* target,const QTextRef* letters);

#ifdef __cplusplus
}
#endif

//! create a standard C string from a QText 
/*!	@param src  source text
	@return  pointer of copied standard C string
*/
inline char* qtext_alloc_q(const QTextRef* src){
	if(!src)return NULL;
	return qtext_copy(qtext_bgn(src),qtext_len(src));
}

//! create a standard C string from a QText 
/*!	@param src  source text
	@return  pointer of copied standard C string
*/
inline char* qtext_alloc_c(const char* src){
	if(!src)return NULL;
	return qtext_copy(src,strlen(src));
}

//! extract lines from a standard C string 
/*!	@param user  any user pointer
	@param src  source text
	@param cbext  call by each extracted line
	@return  pointer of processed in the function
*/
inline const char* qtext_ext_lines_c(void* user,const char* src,QTextExtracted cbext){
	QTextRef src_q;
	qtext_ref_c(&src_q,src);
	return qtext_ext_lines_q(user,&src_q,cbext);
}

//! trim from fore and back of a string 
inline void qtext_trim_both_q(QTextRef* target,const QTextRef* letters){
	qtext_trim_fore_q(target,letters);
	qtext_trim_back_q(target,letters);
}

inline void qtext_trim_fore_c(QTextRef* target,const char* letters){
	QTextRef letters_q;
	qtext_ref_c(&letters_q,letters);
	qtext_trim_fore_q(target,&letters_q);
}

inline void qtext_trim_back_c(QTextRef* target,const char* letters){
	QTextRef letters_q;
	qtext_ref_c(&letters_q,letters);
	qtext_trim_back_q(target,&letters_q);
}

inline void qtext_trim_both_c(QTextRef* target,const char* letters){
	QTextRef letters_q;
	qtext_ref_c(&letters_q,letters);
	qtext_trim_both_q(target,&letters_q);
}

#endif // QUICK_TEXT_H__
