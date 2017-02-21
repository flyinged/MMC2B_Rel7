/*******************************************************************************
 * (c) Copyright 2011 Microsemi S0C Group.  All rights reserved.
 * 
 *  Branch to application startup code.
 *
 * SVN $Revision:$
 * SVN $Date:$
 */
    .text
    .global bx_flashloader_envm
    .code 16
    .syntax unified
    .type bx_flashloader_envm, function
 
/*==============================================================================
 * Vector table location
 *------------------------------------------------------------------------------
 * Modify the value below to specify where the application to be started is
 * located.
 */
.equ    VECTOR_TABLE_ADDR, 0x00008000
.equ    SCB_VTOR_ADDRESS,  0xE000ED08
.equ    THUMB_BIT_MASK,    0xFFFFFFFE

/*==============================================================================
 * bx_user_code
 *------------------------------------------------------------------------------ 
 * Purpose:
 *  Branch to an application startup code.
 *
 *------------------------------------------------------------------------------
 * Input parameters:
 *  None.
 *------------------------------------------------------------------------------ 
 * Branch to an application startup code: Theory of operation:
 *  Step 1:
 *      Load the address of the vector table into R0.
 *  Step 2:
 *      Load the application's initial stack pointer value into R1.
 *  Step 3:
 *      Load the address of the Cortex M3 vector table Offset register into R2.
 *  Step 4:
 *      Load the application's initial stack pointer to CortexM3 vector table
 *      Offset registerSCB_VTOR.
 *  Step 5:
 *      Load the applications's reset handler address into the Link Register.
 *      The location at address R0 + 0x04 is the reset vector,
 *      R0 being the base address of the vector table.
 *  Step 6:
 *      Load the application's initial stack pointer to
 *      the Main Stack Pointer register.
 *  Step 7:
 *      Return from the function by branching to the Link Register. This causes
 *      to branch to the application's startup code/Reset Handler.
 */
    .align 4
bx_flashloader_envm:
    ldr r0,=VECTOR_TABLE_ADDR   /* Step 1. load Vector Table Address */
    ldr r1,[r0,#0x0]            /* Step 2. load Initial Stack pointer = content of VTA(0)*/
    ldr r2,=SCB_VTOR_ADDRESS    /* Step 3. load actual vector table Address */
    str	r0, [r2]                /* Step 4. update initial stack pointer from VTOR */
    ldr lr,[r0,#0x4]            /* Step 5. load link register with VTOR Reset Vector Address */
    msr msp,r1                  /* Step 6. load master stack pointer with initial stack pointer*/
    bx lr                       /* Step 7. branch to link register */
    .end
