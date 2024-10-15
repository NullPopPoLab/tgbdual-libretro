/* † MKKKKKS † */
/*!	@brief  Safety Memory Allocator
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#include "./safe_alloc.h"
#include <string.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void* safe_alloc(size_t size){

	void* inst=malloc(size);
	memset(inst,0,size);	
	return inst;
}
