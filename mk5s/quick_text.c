/* † MKKKKKS † */
/*!	@brief  Quick Text Utilities
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#include "./quick_text.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <stdio.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void qtext_clear(QTextRef* dst){

	if(!dst)return;
	dst->bgn=NULL;
	dst->len=0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void qtext_ref_q(QTextRef* dst,const char* bgn,size_t len){

	if(!dst)return;
	dst->bgn=bgn;
	dst->len=len;
}

/* ----------------------------------------------------------------------- */
void qtext_ref_c(QTextRef* dst,const char* src){

	if(!dst)return;
	dst->bgn=src;
	dst->len=strlen(src);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
char* qtext_copy(const char* bgn,size_t len){

	char* inst=NULL;
	const char* end=bgn+len;
	const char* cur;

	if(len<0)return NULL;
	inst=(char*)malloc(len+1);
	for(cur=bgn;cur<end;++cur)inst[cur-bgn]=*cur;
	inst[len]=0;
	return inst;
}

/* ----------------------------------------------------------------------- */
char* qtext_combine_q_(const void* dmy,...){

	size_t len=0;

	// count length of source strings 
	va_list ap1;
	va_start(ap1, dmy);
	while(1){
		const QTextRef* p=va_arg(ap1, const QTextRef*);
		if(!p)break;
		len+=p->len;
	}
	va_end(ap1);

	if(len<0)return NULL;
	char* inst=(char*)malloc(len+1);
	char* q=inst;

	// copy from source strings 
	va_list ap2;
	va_start(ap2, dmy);
	while(1){
		const QTextRef* p=va_arg(ap2, const QTextRef*);
		if(!p)break;
		int i;
		for(i=0;i<p->len;++i,++q)*q=*(p->bgn+i);
	}
	va_end(ap2);

	inst[len]=0;
	return inst;
}

/* ----------------------------------------------------------------------- */
char* qtext_combine_c_(const void* dmy,...){

	size_t len=0;

	// count length of source strings 
	va_list ap1;
	va_start(ap1, dmy);
	while(1){
		const char* p=va_arg(ap1, const char*);
		if(!p)break;
		len+=strlen(p);
	}
	va_end(ap1);

	if(len<0)return NULL;
	char* inst=(char*)malloc(len+1);
	char* q=inst;

	// copy from source strings 
	va_list ap2;
	va_start(ap2, dmy);
	while(1){
		const char* p=va_arg(ap2, const char*);
		if(!p)break;
		for(;*p;++p,++q)*q=*p;
	}
	va_end(ap2);

	inst[len]=0;
	return inst;
}

/* ----------------------------------------------------------------------- */
void qtext_free(char** instp){

	if(!*instp)return;
	free(*instp);
	*instp=NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
const char* qtext_ext_lines_q(void* user,const QTextRef* src,QTextExtracted cbext)
{
	const char* crp=NULL;
	const char* lfp=NULL;
	const char* bgn=qtext_bgn(src);
	const char* end=qtext_end(src);
	const char* cur;
	QTextRef line;

	// NOP 
	if(!cbext)return end;

	for(cur=bgn;cur<end;++cur){
		if(*cur=='\r'){
			if(crp){
				// CR...CR 
				qtext_ref_q(&line,bgn,crp-bgn);
				if(!cbext(user,&line))return crp+1;
				bgn=crp+1;
			}
			else{
				// single CR needs postpone
			}
			crp=cur;
		}
		else if(*cur=='\n'){
			lfp=cur;
			if(crp){
				if(crp+1<lfp){
					// CR...LF 
					qtext_ref_q(&line,bgn,crp-bgn);
					if(!cbext(user,&line))return crp+1;
					bgn=crp+1;
					qtext_ref_q(&line,bgn,lfp-bgn);
					if(!cbext(user,&line))return lfp+1;
					bgn=lfp+1;
				}
				else{
					// CRLF 
					qtext_ref_q(&line,bgn,crp-bgn);
					if(!cbext(user,&line))return lfp+1;
					bgn=lfp+1;
				}
				crp=NULL;
			}
			else{
				// LF 
				qtext_ref_q(&line,bgn,lfp-bgn);
				if(!cbext(user,&line))return lfp+1;
				bgn=lfp+1;
			}
			lfp=NULL;
		}
	}

	if(crp){
		// CR...EOF 
		qtext_ref_q(&line,bgn,crp-bgn);
		if(!cbext(user,&line))return crp+1;
		bgn=crp+1;
	}
	if(bgn<end){
		// ...EOF 
		qtext_ref_q(&line,bgn,end-bgn);
		cbext(user,&line);
	}

	return end;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void qtext_trim_fore_q(QTextRef* target,const QTextRef* letters){

	if(!target)return;
	if(!letters)return;

	while(1){
		bool done=false;
		for(const char* c=qtext_bgn(letters);target->len>0 && c<qtext_end(letters);++c){
			if(*c!=*qtext_bgn(target))continue;
			++target->bgn;
			--target->len;
			done=true;
		}
		if(!done)break;
	}
}

/* ----------------------------------------------------------------------- */
void qtext_trim_back_q(QTextRef* target,const QTextRef* letters){

	if(!target)return;
	if(!letters)return;

	while(1){
		bool done=false;
		for(const char* c=qtext_bgn(letters);target->len>0 && c<qtext_end(letters);++c){
			if(*c!=*(qtext_end(target)-1))continue;
			--target->len;
			done=true;
		}
		if(!done)break;
	}
}
