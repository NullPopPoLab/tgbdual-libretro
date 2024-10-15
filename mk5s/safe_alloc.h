/* † MKKKKKS † */
/*!	@brief  Safety Memory Allocator
	@author  NullPopPo
	@sa  https://github.com/NullPopPoLab/MKKKKKS
*/
#ifndef SAFE_ALLOC_H__
#define SAFE_ALLOC_H__

#include <stddef.h>
#include <stdlib.h>

//! safety memory freer 
/*!	@param instp  target var pointer of stored instance pointer
	@note  free target memory and instance pointer be NULL
*/
#define safe_free(instp) { \
		if(*instp){ \
			free(*instp); \
			*instp=NULL; \
		} \
	}


#ifdef __cplusplus
extern "C" {
#endif

//! safety mamory allocator 
/*!	@param size  memory size
	@return  allocated memory block (or NULL)
	@note  allocate a memory block and clear to zero
*/
void* safe_alloc(size_t size);

#ifdef __cplusplus
}
#endif

#endif // SAFE_ALLOC_H__
