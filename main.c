
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <stdlib.h>

/*******************************************************************************
* Macros
*******************************************************************************/

#define COMMAND_SIZE                    (25u)

#define ASCII_RETURN_CARRIAGE               (0x0D)

#define UART_TIMEOUT_MS                     (1u)

#define GPIO_INTERRUPT_PRIORITY (7u)

/* LED3 parameters*/
#define PWM_FREQUENCY_DEF_LED3 (0.5f)
#define PWM_DUTY_CYCLE_DEF_LED3 (40.0f)

/* LED4 parameters*/
#define PWM_FREQUENCY_DEF_LED4 (2.0f)
#define PWM_DUTY_CYCLE_DEF_LED4 (100.0f)

/* Parameters for "On" command*/
#define PWM_FREQUENCY_ON (50.0f)

/* State machine */
typedef enum
{
    MESSAGE_ENTER_NEW, 		//Start to enter new command
    MESSAGE_On_LED,			//"On LED(number)"
	MESSAGE_Off_LED,		//"Off LED(number)"
    MESSAGE_NOT_READY,		//Enter new command
	MESSAGE_Blinky			//"Blink LED(number)"
} message_status_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void pwm_LEDs(cyhal_pwm_t pwm_control, float DS, float Fq); //Function to On PWN LED4

cy_rslt_t Enter_MSG(uint8_t message[], uint8_t msg_size);	//Enter message

static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event); //interrupt user_button

/*******************************************************************************
* Global Variables
*******************************************************************************/
volatile bool gpio_intr_flag = false;

uint8_t user_led;

cyhal_pwm_t pwm_led_control_LED3;
cyhal_pwm_t pwm_led_control_LED4;

CY_ALIGN(4) uint8_t message[COMMAND_SIZE];

char separator[1] = ",";


/*******************************************************************************
* Function Name: main
********************************************************************************
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

    char *istr;

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

    result = cyhal_pwm_init(&pwm_led_control_LED3, CYBSP_USER_LED, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
    	CY_ASSERT(false);
    }

    result = cyhal_pwm_init(&pwm_led_control_LED4, CYBSP_USER_LED2, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
    	CY_ASSERT(false);
    }

    /* First initialization*/
    pwm_LEDs(pwm_led_control_LED3, ((100.0f) - PWM_DUTY_CYCLE_DEF_LED3), PWM_FREQUENCY_DEF_LED3);
    pwm_LEDs(pwm_led_control_LED4, ((100.0f) - PWM_DUTY_CYCLE_DEF_LED4), PWM_FREQUENCY_DEF_LED4);

    for (;;)
    {
    	//Interrupt check
    	if(gpio_intr_flag == true)
    	{
    		gpio_intr_flag = false;
    		pwm_LEDs(pwm_led_control_LED3, ((100.0f) - PWM_DUTY_CYCLE_DEF_LED3), PWM_FREQUENCY_DEF_LED3);
    		pwm_LEDs(pwm_led_control_LED4, ((100.0f) - PWM_DUTY_CYCLE_DEF_LED4), PWM_FREQUENCY_DEF_LED4);
    		msg_status = MESSAGE_ENTER_NEW;
    	}

    	switch (msg_status)
    	{
    	case MESSAGE_ENTER_NEW:
    		msg_size = 0;
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

    			    istr = strtok((char*)message, separator);
    			    while (istr != NULL){
    			    	if ((!strcmp(istr, "Off LED3")))
    			    	    {
    			    	    	user_led = 3;
    			    	    	msg_status = MESSAGE_Off_LED;
    			    	    	break;
    			    	    	}
    			    	 else if ((!strcmp((char*)message, "Off LED4")))
    			    	   {
    			    	    	user_led = 4;
    			    	    	msg_status = MESSAGE_Off_LED;
    			    	    	break;
    			    	   }
    			    	 else if(!strcmp(istr, "On LED3")){
    			    		 user_led = 1;
    			    		 istr = strtok(NULL, separator);
    			    		 Duty_Cycle = (float) atof(istr);
    			    		 msg_status = MESSAGE_On_LED;
    			    		 break;
    			    	 }
    			    	 else if(!strcmp(istr, "On LED4")){
    			    		 user_led = 2;
    			    		 istr = strtok(NULL, separator);
    			    		 Duty_Cycle = (float) atof(istr);
    			    		 msg_status = MESSAGE_On_LED;
    			    		 break;
    			    	 }
    			    	 else if(!strcmp(istr, "Blink LED3")){
    			    		 user_led = 5;
    			    		 istr = strtok(NULL, separator);
    			    		 Duty_Cycle = (float) atof(istr);
    			    		 istr = strtok(NULL, separator);
    			    		 Freq = (float) atof(istr);
    			    		 msg_status = MESSAGE_Blinky;
    			    		 break;
    			    	 }
    			    	 else if(!strcmp(istr, "Blink LED4")){
    			    		 user_led = 6;
    			    		 istr = strtok(NULL, separator);
    			    		 Duty_Cycle = (float) atof(istr);
    			    		 istr = strtok(NULL, separator);
    			    		 Freq = (float) atof(istr);
    			    		 msg_status = MESSAGE_Blinky;
    			    		 break;
    			    	 }
    			    }
    			}
    			else
    			{
    				cyhal_uart_putc(&cy_retarget_io_uart_obj, message[msg_size]);
    				msg_size++;
    			}
    		}
    		uart_result = cyhal_uart_getc(&cy_retarget_io_uart_obj,
    	                                  &message[msg_size],
    	                                  UART_TIMEOUT_MS);
    		break;

    	case MESSAGE_On_LED:

    		if(user_led == 1) pwm_LEDs(pwm_led_control_LED3, 100.0f - Duty_Cycle, PWM_FREQUENCY_ON);
    		else if(user_led == 2) pwm_LEDs(pwm_led_control_LED4, 100.0f - Duty_Cycle, PWM_FREQUENCY_ON);
    		msg_status = MESSAGE_ENTER_NEW;

    		break;

    	case MESSAGE_Off_LED:

    		if(user_led == 3) cyhal_pwm_stop(&pwm_led_control_LED3);
    		else if(user_led == 4)  cyhal_pwm_stop(&pwm_led_control_LED4);
    		msg_status = MESSAGE_ENTER_NEW;

    		break;


    	case MESSAGE_Blinky:

    		if(user_led == 5) pwm_LEDs(pwm_led_control_LED3, 100.0f - Duty_Cycle, Freq);
    		else if(user_led == 6) pwm_LEDs(pwm_led_control_LED4, 100.0f - Duty_Cycle, Freq);
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

 void pwm_LEDs(cyhal_pwm_t pwm_control, float DS, float Fq)
 {
  	  cy_rslt_t result;
 	     /* Set the PWM output frequency and duty cycle */
 	     result = cyhal_pwm_set_duty_cycle(&pwm_control, DS, Fq);
 	     if(CY_RSLT_SUCCESS != result)
 	     {
 	         CY_ASSERT(false);
 	     }
 	     /* Start the PWM */
 	     result = cyhal_pwm_start(&pwm_control);
 	     if(CY_RSLT_SUCCESS != result)
 	     {
 	         CY_ASSERT(false);
 	     }
 }

 cy_rslt_t Enter_MSG(uint8_t message[], uint8_t msg_size)
 {
	 memset(message, 0, COMMAND_SIZE);
	 return    cyhal_uart_getc(&cy_retarget_io_uart_obj, &message[msg_size], UART_TIMEOUT_MS);
 }

/* [] END OF FILE */
