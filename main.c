

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <stdlib.h>


/*******************************************************************************
* Macros
*******************************************************************************/

#define COMMAND_SIZE                    (20u)

#define ASCII_RETURN_CARRIAGE               (0x0D)

#define UART_TIMEOUT_MS                     (1u)

#define GPIO_INTERRUPT_PRIORITY (7u)

/* LED3 */
#define PWM_FREQUENCY (0.5f)

#define PWM_DUTY_CYCLE (40.0f)

/* LED4 */
#define PWM_FREQUENCY1 (2.0f)

#define PWM_DUTY_CYCLE1 (100.0f)


typedef enum
{
    MESSAGE_ENTER_NEW,
    MESSAGE_On_LED,
	MESSAGE_Off_LED,
	MESSAGE_DS,
	MESSAGE_Fq,
    MESSAGE_NOT_READY,
	MESSAGE_NOT_READY_Fq,
	MESSAGE_NOT_READY_DS,
	MESSAGE_HELP,
	MESSAGE_Blinky
} message_status_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void pwm_LED3(float DS, float Fq);
void pwm_LED4(float DS, float Fq);
cy_rslt_t Enter_MSG(uint8_t message[], uint8_t msg_size);
static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);

/*******************************************************************************
* Global Variables
*******************************************************************************/
bool timer_interrupt_flag = false;
bool led_blink_active_flag = true;

volatile bool gpio_intr_flag = false;

/* Variable for storing character read from terminal */
uint8_t uart_read_value;
uint8_t user_led;


cyhal_pwm_t pwm_led_control;
cyhal_pwm_t pwm_led_control1;

CY_ALIGN(4) uint8_t message[COMMAND_SIZE];

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CM4 CPU. It sets up a timer to trigger a
* periodic interrupt. The main while loop checks for the status of a flag set
* by the interrupt and toggles an LED at 1Hz to create an LED blinky. The
* while loop also checks whether the 'Enter' key was pressed and
* stops/restarts LED blinking.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_rslt_t uart_result = CY_RSLT_SUCCESS;

    message_status_t msg_status = MESSAGE_ENTER_NEW;

    uint8_t msg_size = 0;

    float Duty_Cycle = 0.0f;
    float Freq = 0.0f;


    bool uart_status = false;
    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize the user button */
    result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                        CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);

    /* Configure GPIO interrupt */
    cyhal_gpio_register_callback(CYBSP_USER_BTN,
                                     gpio_interrupt_handler, NULL);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                                     GPIO_INTERRUPT_PRIORITY, true);

    /* Enable global interrupts */
    __enable_irq();


    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "PSoC 6 MCU: Work with LEDs, PWM, UART"
           "****************** \r\n\n");

    result = cyhal_pwm_init(&pwm_led_control, CYBSP_USER_LED, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
    	printf("API cyhal_pwm_init failed with error code: %lu\r\n", (unsigned long) result);
    	CY_ASSERT(false);
    }

    result = cyhal_pwm_init(&pwm_led_control1, CYBSP_USER_LED2, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
    	printf("API cyhal_pwm_init failed with error code: %lu\r\n", (unsigned long) result);
    	CY_ASSERT(false);
    }

	pwm_LED3(((100.0f) - PWM_DUTY_CYCLE), PWM_FREQUENCY);
	pwm_LED4(((100.0f) - PWM_DUTY_CYCLE1), PWM_FREQUENCY1);

    for (;;)
    {
    	if(gpio_intr_flag == true)
    	{
    		gpio_intr_flag = false;
    		printf("Button entered! \r\n\n");
    		pwm_LED3(((100.0f) - PWM_DUTY_CYCLE), PWM_FREQUENCY);
    		pwm_LED4(((100.0f) - PWM_DUTY_CYCLE1), PWM_FREQUENCY1);
    		msg_status = MESSAGE_ENTER_NEW;
    	}

    	switch (msg_status)
    	{
    	case MESSAGE_ENTER_NEW:
    		msg_size = 0;
    		printf("\r\nCommand:\r\n");
    		uart_result = Enter_MSG(message, msg_size);
    		msg_status = MESSAGE_NOT_READY;
    		break;

    	case MESSAGE_NOT_READY:
    		uart_status = cyhal_uart_is_rx_active(&cy_retarget_io_uart_obj);
    		if ((!uart_status) && (uart_result == CY_RSLT_SUCCESS))
    		{
    			if (message[msg_size] == ASCII_RETURN_CARRIAGE)
    			{
    				message[msg_size]='\0';
    				if((!strcmp((char*)message, "On LED3")))
    				{
    					user_led = 1;
    					msg_status = MESSAGE_DS;
    				}
    				else if ((!strcmp((char*)message, "On LED4")))
    				{
    					user_led = 2;
    					msg_status = MESSAGE_DS;
    				}
    				else if ((!strcmp((char*)message, "Off LED3")))
    				{
    					user_led = 3;
    					msg_status = MESSAGE_Off_LED;
    				}
    				else if ((!strcmp((char*)message, "Off LED4")))
    				{
    					user_led = 4;
    					msg_status = MESSAGE_Off_LED;
    				}
    				else if ((!strcmp((char*)message, "Blink LED3")))
    				{
    					user_led = 5;
    					msg_status = MESSAGE_DS;
    				}
    				else if ((!strcmp((char*)message, "Blink LED4")))
    				{
    					user_led = 6;
    					msg_status = MESSAGE_DS;
    				}
    				else
    				{
    					msg_status = MESSAGE_HELP;
    				}
    			}
    			else
    			{
    				cyhal_uart_putc(&cy_retarget_io_uart_obj, message[msg_size]);
    				msg_size++;
    				/* Check if size of the message  exceeds COMMAND_SIZE
    				 * (inclusive of the string terminating character '\0') */
    				if (msg_size > (COMMAND_SIZE - 1))
    				{
    					printf("\r\n\nMessage length exceeds 20 characters!!!"
    	                       " Please enter a shorter message\r\nor edit the macro"
    	                       " COMMAND_SIZE to suit your message size\r\n");
    					msg_status = MESSAGE_ENTER_NEW;
    					break;
    				}
    			}
    		}
    		uart_result = cyhal_uart_getc(&cy_retarget_io_uart_obj,
    	                                  &message[msg_size],
    	                                  UART_TIMEOUT_MS);
    		break;

    	case MESSAGE_DS:

    		msg_size = 0;
    		printf("\r\nDuty Cycle:\r\n");
    		uart_result = Enter_MSG(message, msg_size);
    		msg_status = MESSAGE_NOT_READY_DS;

    		break;

    	case MESSAGE_NOT_READY_DS:

    		uart_status = cyhal_uart_is_rx_active(&cy_retarget_io_uart_obj);
    		if ((!uart_status) && (uart_result == CY_RSLT_SUCCESS))
    		{
    			if (message[msg_size] == ASCII_RETURN_CARRIAGE)
    			{
    				message[msg_size] ='\0';

    				if(user_led == 1 || user_led == 2)
    				{
    					Duty_Cycle = (float) atof((char*)message);
    					printf("\nDuty Cycle = %g\n", Duty_Cycle);
    					msg_status = MESSAGE_On_LED;
    				}

    				else if(user_led == 5 || user_led == 6)
    				{
    					msg_status = MESSAGE_Fq;
    				}
    			}
    			else
    			{
    				cyhal_uart_putc(&cy_retarget_io_uart_obj, message[msg_size]);
    				msg_size++;
    	            /* Check if size of the message  exceeds COMMAND_SIZE
    	             * (inclusive of the string terminating character '\0')*/
    				if (msg_size > (COMMAND_SIZE - 1))
    				{
    					printf("\r\n\nMessage length exceeds 20 characters!!!"
    	                	   " Please enter a shorter message\r\nor edit the macro"
    	                	   " COMMAND_SIZE to suit your message size\r\n");
    					msg_status = MESSAGE_DS;
    					break;
    				}
    			}
    		}
    		uart_result = cyhal_uart_getc(&cy_retarget_io_uart_obj,
    	                	              &message[msg_size],
    	                	              UART_TIMEOUT_MS);

    		break;

    	case MESSAGE_On_LED:

    		if(user_led == 1) pwm_LED3(100.0f - Duty_Cycle, 50.0);
    		else if(user_led == 2) pwm_LED4(100.0f - Duty_Cycle, 50.0);
    		msg_status = MESSAGE_ENTER_NEW;

    		break;

    	case MESSAGE_Off_LED:

    		if(user_led == 3) cyhal_pwm_stop(&pwm_led_control);
    		else if(user_led == 4)  cyhal_pwm_stop(&pwm_led_control1);
    		msg_status = MESSAGE_ENTER_NEW;

    		break;

    	case MESSAGE_Fq:

    		msg_size = 0;
    		printf("\r\nFrequency:\r\n");
    		uart_result = Enter_MSG(message, msg_size);
    		msg_status = MESSAGE_NOT_READY_Fq;

    		break;

    	case MESSAGE_NOT_READY_Fq:

    		uart_status = cyhal_uart_is_rx_active(&cy_retarget_io_uart_obj);
    		if ((!uart_status) && (uart_result == CY_RSLT_SUCCESS))
    		{
    			if (message[msg_size] == ASCII_RETURN_CARRIAGE)
    			{
    				message[msg_size]='\0';
    				Freq = (float) atof((char*)message);
    				printf("\nFrequency = %g Hz\n", Freq);
    				msg_status = MESSAGE_Blinky;
    			}
    			else
    			{
    				cyhal_uart_putc(&cy_retarget_io_uart_obj, message[msg_size]);
    				msg_size++;
    				/* Check if size of the message  exceeds COMMAND_SIZE
    				 * (inclusive of the string terminating character '\0')*/
    				if (msg_size > (COMMAND_SIZE - 1))
    				{
    					printf("\r\n\nFq length exceeds 20 characters!!!"
    	                	   " Please enter a shorter message\r\nor edit the macro"
    	                	   " COMMAND_SIZE to suit your message size\r\n");
    					msg_status = MESSAGE_Fq;
    					break;
    				}
    			}
    		}
    		uart_result = cyhal_uart_getc(&cy_retarget_io_uart_obj,
    	                	              &message[msg_size],
    	                	              UART_TIMEOUT_MS);

    		break;

    	case MESSAGE_Blinky:

    		if(user_led == 5) pwm_LED3(100.0f - Duty_Cycle, Freq);
    		else if(user_led == 6) pwm_LED4(100.0f - Duty_Cycle, Freq);
    		msg_status = MESSAGE_ENTER_NEW;

    		break;

    	case MESSAGE_HELP:

    		printf("****************** "
    		       "HELP MENU"
    		       "****************** \r\n\n");
    		printf("****************** "
    		       "Please, enter one of these command:"
    		       "****************** \r\n\n");
    		printf("_________***********________\r\n\n "
    		       "1. *On LED3* - enable LED3;\r\n "
    			   "2. *On LED4* - enable LED4;\r\n "
    			   "3. *Off LED3* - disable LED3;\r\n "
    			   "4. *Off LED4* - disable LED4;\r\n "
    			   "5. *Blink LED3* - blink LED3;\r\n "
    			   "6. *Blink LED4* - blink LED4;\r\n "
    			   "_________***********________\r\n\n ");
    		msg_status = MESSAGE_ENTER_NEW;

    		break;

    	default:

    		break;
    	}
    }
}


static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_irq_event_t event)
{
    gpio_intr_flag = true;
}

 void pwm_LED3(float DS, float Fq)
{
	 cy_rslt_t result;

	     /* Set the PWM output frequency and duty cycle */
	     result = cyhal_pwm_set_duty_cycle(&pwm_led_control, DS, Fq);
	     if(CY_RSLT_SUCCESS != result)
	     {
	         printf("API cyhal_pwm_set_duty_cycle failed with error code: %lu\r\n", (unsigned long) result);
	         CY_ASSERT(false);
	     }
	     /* Start the PWM */
	     result = cyhal_pwm_start(&pwm_led_control);
	     if(CY_RSLT_SUCCESS != result)
	     {
	         printf("API cyhal_pwm_start failed with error code: %lu\r\n", (unsigned long) result);
	         CY_ASSERT(false);
	     }
}
 void pwm_LED4(float DS, float Fq)
 {
  	  cy_rslt_t result;
 	     /* Set the PWM output frequency and duty cycle */
 	     result = cyhal_pwm_set_duty_cycle(&pwm_led_control1, DS, Fq);
 	     if(CY_RSLT_SUCCESS != result)
 	     {
 	         printf("API cyhal_pwm_set_duty_cycle failed with error code: %lu\r\n", (unsigned long) result);
 	         CY_ASSERT(false);
 	     }
 	     /* Start the PWM */
 	     result = cyhal_pwm_start(&pwm_led_control1);
 	     if(CY_RSLT_SUCCESS != result)
 	     {
 	         printf("API cyhal_pwm_start failed with error code: %lu\r\n", (unsigned long) result);
 	         CY_ASSERT(false);
 	     }
 }

 cy_rslt_t Enter_MSG(uint8_t message[], uint8_t msg_size)
 {
	 memset(message, 0, COMMAND_SIZE);
	 return    cyhal_uart_getc(&cy_retarget_io_uart_obj, &message[msg_size], UART_TIMEOUT_MS);
 }

/* [] END OF FILE */
