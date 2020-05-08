
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_CONF_H
#define __STM32F4xx_CONF_H

#include <stdint.h>

#define HSI_VALUE 16000000
#define HSE_VALUE 8000000


#define VECT_TAB_OFFSET  0x0 

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Uncomment the line below to expanse the "assert_param" macro in the 
   Standard Peripheral Library drivers code */
#define USE_FULL_ASSERT    1

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT

/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *   which reports the name of the source file and the source
  *   line number of the call that failed. 
  *   If expr is true, it returns no value.
  * @retval None
  */
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #ifndef assert_param
    #define assert_param(expr) ((void)0)
  #endif
#endif /* USE_FULL_ASSERT */

#endif /* __STM32F4xx_CONF_H */