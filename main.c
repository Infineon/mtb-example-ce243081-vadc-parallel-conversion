/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for VADC parallel conversion code 
*              example for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
Cy_VADC_RESULT_SIZE_t adc_result_G0Ch3 = 0;
Cy_VADC_RESULT_SIZE_t adc_result_G1Ch3 = 0;

/*******************************************************************************
* Function Name: nvic_config
********************************************************************************
* Summary:
* Configures the NVIC for the VADC queue complete interrupt (IRQ17). Sets the
* interrupt priority and enables the IRQ.
*
* Parameters:
*  none
*
* Return:
*  none
*
*******************************************************************************/
void nvic_config(void)
{
    /* Set IRQ17 priority level (1 = high priority for responsive ADC servicing) */
    NVIC_SetPriority(IRQ17_IRQn, 1u);
    /* Enable IRQ17 in the NVIC controller */
    NVIC_EnableIRQ(IRQ17_IRQn);
}

/*******************************************************************************
* Function Name: IRQ17_Handler
********************************************************************************
* Summary:
* Interrupt handler for VADC Group 0 queue complete event (IRQ17). Reads the
* conversion results from both groups and clears the queue event flag.
*
* Parameters:
*  none
*
* Return:
*  none
*
*******************************************************************************/
void IRQ17_Handler(void)
{
    adc_result_G0Ch3 = Cy_VADC_GROUP_GetResult(VADC_G0, 0);
    adc_result_G1Ch3 = Cy_VADC_GROUP_GetResult(VADC_G1, 1);

    /* Clear queue request-source event flag to acknowledge this interrupt and allow retriggering. */
    Cy_VADC_GROUP_QueueClearReqSrcEvent(vadc_0_group_0_HW);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* Entry point of the application. Initializes board peripherals and retarget-IO
* UART, configures VADC groups, sets up the queue interrupt, and enters the
* main loop printing conversion results every 100 ms.
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
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_HW);

    /* VADC Group 1 Initialization
     * Note: init_cycfg_peripherals() initializes the VADC global block and configures
     * channels, but does NOT initialize the group or set power mode. These must be
     * done explicitly for the group to become operational. */
    Cy_VADC_GROUP_Init(vadc_0_group_0_HW, &vadc_0_group0_init_config);
    Cy_VADC_GROUP_InputClassInit(vadc_0_group_0_HW, vadc_0_0_iclass_0, CY_VADC_GROUP_CONV_STD, 0U);
    Cy_VADC_GROUP_InputClassInit(vadc_0_group_0_HW, vadc_0_0_iclass_1, CY_VADC_GROUP_CONV_STD, 1U);
    /* Power up the analog converter; without this, no conversions occur. */
    Cy_VADC_GROUP_SetPowerMode(vadc_0_group_0_HW, CY_VADC_GROUP_POWERMODE_NORMAL);

    /* VADC Group 1 Initialization
     * Note: init_cycfg_peripherals() initializes the VADC global block and configures
     * channels, but does NOT initialize the group or set power mode. These must be
     * done explicitly for the group to become operational. */
    Cy_VADC_GROUP_Init(vadc_0_group_1_HW, &vadc_0_group1_init_config);
    Cy_VADC_GROUP_InputClassInit(vadc_0_group_1_HW, vadc_0_1_iclass_0, CY_VADC_GROUP_CONV_STD, 0U);
    Cy_VADC_GROUP_InputClassInit(vadc_0_group_1_HW, vadc_0_1_iclass_1, CY_VADC_GROUP_CONV_STD, 1U);
    /* Power up the analog converter; without this, no conversions occur. */
    Cy_VADC_GROUP_SetPowerMode(vadc_0_group_1_HW, CY_VADC_GROUP_POWERMODE_NORMAL);

    /* Connect queue request-source event to Group SR0
     * This event is raised when the queue completes conversion of all queued entries. */
    Cy_VADC_GROUP_QueueSetReqSrcEventInterruptNode(vadc_0_group_0_HW, (Cy_VADC_SR_t) CY_VADC_SR_GROUP_SR0);

    /* Map VADC Group 0 Service Request SR0 to NVIC IRQ17 via SCU interrupt control */
    Cy_SCU_SetInterruptControl(IRQ17_IRQn, CY_SCU_IRQCTRL_VADC0_G0SR0_IRQ17);

    /* Configure NVIC: set priority and enable IRQ17 */
    nvic_config();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("** PSOC C1 : VADC Parallel Conversion **\r\n\n");

    for (;;)
    {
        printf("G0CH1: %u, G1CH0: %u\r\n", adc_result_G0Ch3, adc_result_G1Ch3);
        Cy_Delay(100);
    }
}

/* [] END OF FILE */
