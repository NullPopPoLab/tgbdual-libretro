/* † MKKKKKS † */
/*!	@brief  Quick Path
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#include "./quick_path.h"
#include <string.h>

bool g_qpath_backslashable=false;

static const char* empty="";

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void qpath_clear(QPathInfo* dst){

	if(!dst)return;

	qtext_ref_q(&dst->scheme,empty,0);
	qtext_ref_q(&dst->base,empty,0);
	qtext_ref_q(&dst->route,empty,0);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void qpath_setup_q(QPathInfo* dst,const QTextRef* path){

	if(!dst)return;

	qpath_clear(dst);
	
	if(!qtext_bgn(path))return;
	if(qtext_len(path)<1)return;

	const char* bp=qtext_bgn(path);
	const char* cp=NULL;
	for(const char* p=bp;p<qtext_end(path);++p){
		if(*p=='\0'){break;}
		if(*p==':'){cp=p;break;}
		if(*p=='/'){break;}
		if(g_qpath_backslashable && *p=='\\'){break;}
	}

	if(cp){
		qtext_ref_q(&dst->scheme,bp,cp-bp);
		bp=cp+1;
	}

	const char* sp=NULL;
	for(const char* p=bp;p<qtext_end(path);++p){
		if(*p=='\0'){break;}
		if(*p=='/'){sp=p+1;continue;}
		if(g_qpath_backslashable && *p=='\\'){sp=p+1;continue;}
		break;
	}

	if(sp){
		qtext_ref_q(&dst->base,bp,sp-bp);
		bp=sp;
	}

	qtext_ref_q(&dst->route,bp,qtext_end(path)-bp);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
bool qpath_extension_q(QTextRef* dst,const QTextRef* src,bool polydot){

	if(!dst)return false;
	qtext_clear(dst);
	if(!src)return false;

	const char* dp=NULL;
	for(const char* p=qtext_end(src);p>qtext_bgn(src);){
		--p;
		if(*p=='\0')return false;
		if(*p=='.'){
			dp=p;
			if(polydot)continue;
			else break;
		}
		if(*p=='/'){break;}
		if(g_qpath_backslashable && *p=='\\'){break;}
	}

	if(!dp)return false;
	qtext_ref_q(dst,dp+1,qtext_end(src)-dp-1);
	return true;
}

/* ----------------------------------------------------------------------- */
bool qpath_dirname_q(QTextRef* dst,const QTextRef* src){

	if(!dst)return false;
	qtext_clear(dst);
	if(!src)return false;

	const char* sp=NULL;
	for(const char* p=qtext_end(src);p>qtext_bgn(src);){
		--p;
		if(*p=='\0')return false;
		if(*p=='/'){sp=p;continue;}
		if(g_qpath_backslashable && *p=='\\'){sp=p;continue;}
		if(sp)break;
	}

	if(!sp){
		qtext_ref_q(dst,empty,0);
	}
	else{
		qtext_ref_q(dst,qtext_bgn(src),sp-qtext_bgn(src));
	}
	return true;
}

/* ----------------------------------------------------------------------- */
bool qpath_filename_q(QTextRef* dst,const QTextRef* src){

	if(!dst)return false;
	qtext_clear(dst);
	if(!src)return false;

	const char* sp=NULL;
	for(const char* p=qtext_end(src);p>qtext_bgn(src);){
		--p;
		if(*p=='\0')return false;
		if(*p=='/'){sp=p;break;}
		if(g_qpath_backslashable && *p=='\\'){sp=p;break;}
	}

	if(!sp){
		*dst=*src;
	}
	else{
		qtext_ref_q(dst,sp+1,qtext_end(src)-sp-1);
	}
	return true;
}

/* ----------------------------------------------------------------------- */
bool qpath_filename_noext_q(QTextRef* dst,const QTextRef* src,bool polydot){

	if(!qpath_filename_q(dst,src))return false;

	QTextRef ext;
	if(!qpath_extension_q(&ext,dst,polydot))return true;

	dst->len-=ext.len+1;
	return true;
}
