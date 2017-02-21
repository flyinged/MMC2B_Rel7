/*******************************************************************************
 * (c) Copyright 2011 Microsemi S0C Group.  All rights reserved.
 * 
 *  Branch to application startup code.
 *
 * SVN $Revision:$
 * SVN $Date:$
 */
    .text
    .global bx_3rd_eNVM_Image
    .code 16
    .syntax unified
    .type bx_3rd_eNVM_Image, function
 
/*==============================================================================
 * Vector table location
 *------------------------------------------------------------------------------
 * Modify the value below to specify where the application to be started is
 * located.
 */
.equ	VECTOR_TABLE_ADDR, 0x00020000


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
 * Theory of operation:
 *  Step 1:
 *      Load the address of the vector table into R0.
 *  Step 2:
 *      Dereference the address stored into R0 to load the application's initial
 *      stack pointer value into R1.
 *  Step 3:
 *      Dereference the address stored in R0 + 0x04 to load the applications's
 *      reset address into the Link Register. The location at address R0 + 0x04
 *      is the reset vector, R0 being the base address of the vector table.
 *  Step 4:
 *      Load the content of R1 into the Main Stack Pointer register.
 *  Step 5:
 *      Return from the function by branching to the Link Register. This causes
 *      to branch to the application's startup code.
 */
    .align 4
bx_3rd_eNVM_Image:
    ldr r0,=VECTOR_TABLE_ADDR   /* Step 1. */
    ldr r1,[r0,#0x0]            /* Step 2. */

    movw r2,#0xED08
    movt r2,#0xE000
    str	r0, [r2, #0]

    ldr lr,[r0,#0x4]            /* Step 3. */


    msr msp,r1                  /* Step 4. */
    bx lr                       /* Step 5. */

    .end
