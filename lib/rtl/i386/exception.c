/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           User-mode exception support for IA-32
 * FILE:              lib/rtl/i386/exception.c
 * PROGRAMERS:        Alex Ionescu (alex@relsoft.net)
 *                    Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
STDCALL
RtlpGetStackLimits(PULONG_PTR StackBase,
                   PULONG_PTR StackLimit);

PEXCEPTION_REGISTRATION_RECORD
STDCALL
RtlpGetExceptionList(VOID);

VOID
STDCALL
RtlpSetExceptionList(PEXCEPTION_REGISTRATION_RECORD NewExceptionList);

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
RtlGetCallersAddress(OUT PVOID *CallersAddress,
                     OUT PVOID *CallersCaller)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN PCONTEXT Context)
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationFrame, NestedFrame = NULL;
    PEXCEPTION_REGISTRATION_RECORD DispatcherContext;
    EXCEPTION_RECORD ExceptionRecord2;
    EXCEPTION_DISPOSITION ReturnValue;
    ULONG_PTR StackLow, StackHigh;
    ULONG_PTR RegistrationFrameEnd;
    DPRINT1("RtlDispatchException(): %p, %p \n", ExceptionRecord, Context);

    /* Get the current stack limits and registration frame */
    RtlpGetStackLimits(&StackLow, &StackHigh);
    RegistrationFrame = RtlpGetExceptionList();
    DPRINT1("RegistrationFrame is 0x%X\n", RegistrationFrame);

    /* Now loop every frame */
    while (RegistrationFrame != EXCEPTION_CHAIN_END)
    {
        /* Find out where it ends */
        RegistrationFrameEnd = (ULONG_PTR)RegistrationFrame +
                                sizeof(*RegistrationFrame);

        /* Make sure the registration frame is located within the stack */
        if ((RegistrationFrameEnd > StackHigh) ||
            ((ULONG_PTR)RegistrationFrame < StackLow) ||
            ((ULONG_PTR)RegistrationFrame & 0x3))
        {
            /* Check if this happened in the DPC Stack */
            if (RtlpHandleDpcStackException(RegistrationFrame,
                                            RegistrationFrameEnd,
                                            &StackLow,
                                            &StackHigh))
            {
                /* Use DPC Stack Limits and restart */
                continue;
            }

            /* Set invalid stack and return false */
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            DPRINT1("Invalid exception frame\n");
            return FALSE;
        }

        /* Check if logging is enabled */
        DPRINT1("Checking for logging\n");
        RtlpCheckLogException(ExceptionRecord,
                              Context,
                              RegistrationFrame,
                              sizeof(*RegistrationFrame));

        /* Call the handler */
        DPRINT1("Executing handler: %p\n", RegistrationFrame->Handler);
        ReturnValue = RtlpExecuteHandlerForException(ExceptionRecord,
                                                     RegistrationFrame,
                                                     Context,
                                                     &DispatcherContext,
                                                     RegistrationFrame->Handler);
        DPRINT1("Handler returned: %lx\n", ReturnValue);

        /* Check if this is a nested frame */
        if (RegistrationFrame == NestedFrame)
        {
            /* Mask out the flag and the nested frame */
            ExceptionRecord->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;
            NestedFrame = NULL;
        }

        /* Handle the dispositions */
        if (ReturnValue == ExceptionContinueExecution)
        {
            /* Check if it was non-continuable */
            if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
            {
                /* Set up the exception record */
                ExceptionRecord2.ExceptionRecord = ExceptionRecord;
                ExceptionRecord2.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord2.NumberParameters = 0;

                /* Raise the exception */
                DPRINT1("Non-continuable\n");
                RtlRaiseException(&ExceptionRecord2);
            }
            else
            {
                /* Return to caller */
                return TRUE;
            }
        }
        else if (ReturnValue == ExceptionNestedException)
        {
            /* Turn the nested flag on */
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;

            /* Update the current nested frame */
            if (NestedFrame < DispatcherContext) NestedFrame = DispatcherContext;
        }
        else if (ReturnValue == ExceptionContinueSearch)
        {
        }
        else
        {
            /* Set up the exception record */
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.NumberParameters = 0;

            /* Raise the exception */
            RtlRaiseException(&ExceptionRecord2);
        }

        /* Go to the next frame */
        RegistrationFrame = RegistrationFrame->Next;
    }

    /* Unhandled, return false */
    DPRINT1("FALSE\n");
    return FALSE;
}

/*
 * @implemented
 */
VOID
STDCALL
RtlUnwind(PVOID RegistrationFrame OPTIONAL,
          PVOID ReturnAddress OPTIONAL,
          PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
          PVOID EaxValue)
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationFrame2, OldFrame;
    PEXCEPTION_REGISTRATION_RECORD DispatcherContext;
    EXCEPTION_RECORD ExceptionRecord2, ExceptionRecord3;
    EXCEPTION_DISPOSITION ReturnValue;
    ULONG_PTR StackLow, StackHigh;
    ULONG_PTR RegistrationFrameEnd;
    CONTEXT LocalContext;
    PCONTEXT Context;
    DPRINT1("RtlUnwind(). RegistrationFrame 0x%X\n", RegistrationFrame);

    /* Get the current stack limits */
    RtlpGetStackLimits(&StackLow, &StackHigh);

    /* Check if we don't have an exception record */
    if (!ExceptionRecord)
    {
        /* Overwrite the argument */
        ExceptionRecord = &ExceptionRecord3;

        /* Setup a local one */
        ExceptionRecord3.ExceptionFlags = 0;
        ExceptionRecord3.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord3.ExceptionRecord = NULL;
        ExceptionRecord3.ExceptionAddress = RtlpGetExceptionAddress();
        ExceptionRecord3.NumberParameters = 0;
    }

    /* Check if we have a frame */
    if (RegistrationFrame)
    {
        /* Set it as unwinding */
        ExceptionRecord->ExceptionFlags |= EXCEPTION_UNWINDING;
    }
    else
    {
        /* Set the Exit Unwind flag as well */
        ExceptionRecord->ExceptionFlags |= (EXCEPTION_UNWINDING |
                                            EXCEPTION_EXIT_UNWIND);
    }

    /* Now capture the context */
    Context = &LocalContext;
    LocalContext.ContextFlags = CONTEXT_INTEGER |
                                CONTEXT_CONTROL |
                                CONTEXT_SEGMENTS;
    RtlpCaptureContext(Context);

    /* Pop the current arguments off */
    LocalContext.Esp += sizeof(RegistrationFrame) +
                        sizeof(ReturnAddress) +
                        sizeof(ExceptionRecord) +
                        sizeof(ReturnValue);

    /* Set the new value for EAX */
    LocalContext.Eax = (ULONG)EaxValue;

    /* Get the current frame */
    RegistrationFrame2 = RtlpGetExceptionList();

    /* Now loop every frame */
    while (RegistrationFrame2 != EXCEPTION_CHAIN_END)
    {
        DPRINT1("RegistrationFrame is 0x%X\n", RegistrationFrame2);

        /* If this is the target */
        if (RegistrationFrame2 == RegistrationFrame)
        {
            /* Continue execution */
            ZwContinue(Context, FALSE);
        }

        /* Check if the frame is too low */
        if ((RegistrationFrame) && ((ULONG_PTR)RegistrationFrame <
                                    (ULONG_PTR)RegistrationFrame2))
        {
            /* Create an invalid unwind exception */
            ExceptionRecord2.ExceptionCode = STATUS_INVALID_UNWIND_TARGET;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;

            /* Raise the exception */
            DPRINT1("Frame is invalid\n");
            RtlRaiseException(&ExceptionRecord2);
        }

        /* Find out where it ends */
        RegistrationFrameEnd = (ULONG_PTR)RegistrationFrame2 +
                                sizeof(*RegistrationFrame2);

        /* Make sure the registration frame is located within the stack */
        if ((RegistrationFrameEnd > StackHigh) ||
            ((ULONG_PTR)RegistrationFrame < StackLow) ||
            ((ULONG_PTR)RegistrationFrame & 0x3))
        {
            /* Check if this happened in the DPC Stack */
            if (RtlpHandleDpcStackException(RegistrationFrame,
                                            RegistrationFrameEnd,
                                            &StackLow,
                                            &StackHigh))
            {
                /* Use DPC Stack Limits and restart */
                continue;
            }

            /* Create an invalid stack exception */
            ExceptionRecord2.ExceptionCode = STATUS_BAD_STACK;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;

            /* Raise the exception */
            DPRINT1("Frame has bad stack\n");
            RtlRaiseException(&ExceptionRecord2);
        }
        else
        {
            /* Call the handler */
            DPRINT1("Executing unwind handler: %p\n", RegistrationFrame2->Handler);
            ReturnValue = RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                      RegistrationFrame2,
                                                      Context,
                                                      &DispatcherContext,
                                                      RegistrationFrame2->Handler);
            DPRINT1("Handler returned: %lx\n", ReturnValue);

            /* Handle the dispositions */
            if (ReturnValue == ExceptionContinueSearch)
            {
                /* Get out of here */
                break;
            }
            else if (ReturnValue == ExceptionCollidedUnwind)
            {
                /* Get the previous frame */
                RegistrationFrame2 = DispatcherContext;
            }
            else
            {
                /* Set up the exception record */
                ExceptionRecord2.ExceptionRecord = ExceptionRecord;
                ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
                ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord2.NumberParameters = 0;

                /* Raise the exception */
                RtlRaiseException(&ExceptionRecord2);
            }

            /* Go to the next frame */
            OldFrame = RegistrationFrame2;
            RegistrationFrame2 = RegistrationFrame2->Next;

            /* Remove this handler */
            RtlpSetExceptionList(OldFrame);
        }
    }

    /* Check if we reached the end */
    if (RegistrationFrame == EXCEPTION_CHAIN_END)
    {
        /* Unwind completed, so we don't exit */
        ZwContinue(Context, FALSE);
    }
    else
    {
        /* This is an exit_unwind or the frame wasn't present in the list */
        ZwRaiseException(ExceptionRecord, Context, FALSE);
    }
}

/* EOF */
