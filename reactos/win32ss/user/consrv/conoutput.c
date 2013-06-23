/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/conoutput.c
 * PURPOSE:         General Console Output Functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "console.h"
#include "include/conio.h"
#include "conio.h"
#include "conoutput.h"
#include "handle.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/


/* PUBLIC SERVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvInvalidateBitMapRect(IN PCONSOLE Console,
                           IN PCONSOLE_SCREEN_BUFFER Buffer,
                           IN PSMALL_RECT Region);
CSR_API(SrvInvalidateBitMapRect)
{
    NTSTATUS Status;
    PCONSOLE_INVALIDATEDIBITS InvalidateDIBitsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.InvalidateDIBitsRequest;
    PCONSOLE_SCREEN_BUFFER Buffer;

    DPRINT("SrvInvalidateBitMapRect\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   InvalidateDIBitsRequest->OutputHandle,
                                   &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvInvalidateBitMapRect(Buffer->Header.Console,
                                        Buffer,
                                        &InvalidateDIBitsRequest->Region);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           OUT PCONSOLE_CURSOR_INFO CursorInfo);
CSR_API(SrvGetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvGetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     CursorInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleCursorInfo(Buffer->Header.Console,
                                        Buffer,
                                        &CursorInfoRequest->Info);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN PCONSOLE_CURSOR_INFO CursorInfo);
CSR_API(SrvSetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     CursorInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCursorInfo(Buffer->Header.Console,
                                        Buffer,
                                        &CursorInfoRequest->Info);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorPosition(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN PCOORD Position);
CSR_API(SrvSetConsoleCursorPosition)
{
    NTSTATUS Status;
    PCONSOLE_SETCURSORPOSITION SetCursorPositionRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorPositionRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleCursorPosition\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetCursorPositionRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCursorPosition(Buffer->Header.Console,
                                            Buffer,
                                            &SetCursorPositionRequest->Position);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

CSR_API(SrvCreateConsoleScreenBuffer)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PCONSOLE_CREATESCREENBUFFER CreateScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CreateScreenBufferRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    PVOID ScreenBufferInfo = NULL;
    TEXTMODE_BUFFER_INFO TextModeInfo = {{80, 25},
                                         DEFAULT_SCREEN_ATTRIB,
                                         DEFAULT_POPUP_ATTRIB ,
                                         TRUE,
                                         CSR_DEFAULT_CURSOR_SIZE};
    GRAPHICS_BUFFER_INFO GraphicsInfo;
    GraphicsInfo.Info = CreateScreenBufferRequest->GraphicsBufferInfo; // HACK for MSVC

    DPRINT("SrvCreateConsoleScreenBuffer\n");

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_TEXTMODE_BUFFER)
    {
        ScreenBufferInfo = &TextModeInfo;

        /*
        if (Console->ActiveBuffer)
        {
            TextModeInfo.ScreenBufferSize = Console->ActiveBuffer->ScreenBufferSize;
            if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 80;
            if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 25;

            TextModeInfo.ScreenAttrib = Console->ActiveBuffer->ScreenBuffer.TextBuffer.ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Console->ActiveBuffer->ScreenBuffer.TextBuffer.PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Console->ActiveBuffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Console->ActiveBuffer->CursorInfo.dwSize;
        }
        */

        /*
         * This is Windows' behaviour
         */

        /* Use the current console size. Regularize it if needed. */
        TextModeInfo.ScreenBufferSize = Console->ConsoleSize;
        if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 1;
        if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 1;

        /* If we have an active screen buffer, use its attributes as the new ones */
        if (Console->ActiveBuffer && GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
        {
            PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;

            TextModeInfo.ScreenAttrib = Buffer->ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Buffer->PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Buffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Buffer->CursorInfo.dwSize;
        }
    }
    else if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        /* Get infos from the graphics buffer information structure */
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMapInfo,
                                      1,
                                      CreateScreenBufferRequest->GraphicsBufferInfo.dwBitMapInfoLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }

        ScreenBufferInfo = &GraphicsInfo;

        /* Initialize shared variables */
        CreateScreenBufferRequest->GraphicsBufferInfo.hMutex   = GraphicsInfo.Info.hMutex   = INVALID_HANDLE_VALUE;
        CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap = GraphicsInfo.Info.lpBitMap = NULL;

        /* A graphics screen buffer is never inheritable */
        CreateScreenBufferRequest->Inheritable = FALSE;
    }

    Status = ConDrvCreateScreenBuffer(&Buff,
                                      Console,
                                      CreateScreenBufferRequest->ScreenBufferType,
                                      ScreenBufferInfo);
    if (!NT_SUCCESS(Status)) goto Quit;

    /* Insert the new handle inside the process handles table */
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    Status = ConSrvInsertObject(ProcessData,
                                &CreateScreenBufferRequest->OutputHandle,
                                &Buff->Header,
                                CreateScreenBufferRequest->Access,
                                CreateScreenBufferRequest->Inheritable,
                                CreateScreenBufferRequest->ShareMode);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    if (!NT_SUCCESS(Status)) goto Quit;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)Buff;
        /*
         * Initialize the graphics buffer information structure
         * and give it back to the client.
         */
        CreateScreenBufferRequest->GraphicsBufferInfo.hMutex   = Buffer->ClientMutex;
        CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap = Buffer->ClientBitMap;
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleActiveScreenBuffer(IN PCONSOLE Console,
                                   IN PCONSOLE_SCREEN_BUFFER Buffer);
CSR_API(SrvSetConsoleActiveScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_SETACTIVESCREENBUFFER SetScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferRequest;
    PCONSOLE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleActiveScreenBuffer\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetScreenBufferRequest->OutputHandle,
                                   &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleActiveScreenBuffer(Buffer->Header.Console,
                                                Buffer);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}


/* CSR THREADS FOR WriteConsole ***********************************************/

static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOL CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
WriteConsoleThread(IN PLIST_ENTRY WaitList,
                   IN PCSR_THREAD WaitThread,
                   IN PCSR_API_MESSAGE WaitApiMessage,
                   IN PVOID WaitContext,
                   IN PVOID WaitArgument1,
                   IN PVOID WaitArgument2,
                   IN ULONG WaitFlags)
{
    NTSTATUS Status;

    DPRINT("WriteConsoleThread - WaitContext = 0x%p, WaitArgument1 = 0x%p, WaitArgument2 = 0x%p, WaitFlags = %lu\n", WaitContext, WaitArgument1, WaitArgument2, WaitFlags);

    /*
     * If we are notified of the process termination via a call
     * to CsrNotifyWaitBlock triggered by CsrDestroyProcess or
     * CsrDestroyThread, just return.
     */
    if (WaitFlags & CsrProcessTerminating)
    {
        Status = STATUS_THREAD_IS_TERMINATING;
        goto Quit;
    }

    Status = DoWriteConsole(WaitApiMessage,
                            WaitThread,
                            FALSE);

Quit:
    if (Status != STATUS_PENDING)
    {
        WaitApiMessage->Status = Status;
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOL CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    PVOID Buffer;
    DWORD Written = 0;
    ULONG Length;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(ClientThread->Process), WriteConsoleRequest->OutputHandle, &Buff, GENERIC_WRITE, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    // if (Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION))
    if (Console->PauseFlags && Console->UnpauseEvent != NULL)
    {
        if (CreateWaitBlock)
        {
            if (!CsrCreateWait(&Console->WriteWaitQueue,
                               WriteConsoleThread,
                               ClientThread,
                               ApiMessage,
                               NULL,
                               NULL))
            {
                /* Fail */
                ConSrvReleaseScreenBuffer(Buff, FALSE);
                return STATUS_NO_MEMORY;
            }
        }

        /* Wait until we un-pause the console */
        Status = STATUS_PENDING;
    }
    else
    {
        if (WriteConsoleRequest->Unicode)
        {
            Buffer = WriteConsoleRequest->Buffer;
        }
        else
        {
            Length = MultiByteToWideChar(Console->OutputCodePage, 0,
                                         (PCHAR)WriteConsoleRequest->Buffer,
                                         WriteConsoleRequest->NrCharactersToWrite,
                                         NULL, 0);
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
            if (Buffer)
            {
                MultiByteToWideChar(Console->OutputCodePage, 0,
                                    (PCHAR)WriteConsoleRequest->Buffer,
                                    WriteConsoleRequest->NrCharactersToWrite,
                                    (PWCHAR)Buffer, Length);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        if (Buffer)
        {
            if (NT_SUCCESS(Status))
            {
                Status = ConioWriteConsole(Console,
                                           Buff,
                                           Buffer,
                                           WriteConsoleRequest->NrCharactersToWrite,
                                           TRUE);
                if (NT_SUCCESS(Status))
                {
                    Written = WriteConsoleRequest->NrCharactersToWrite;
                }
            }

            if (!WriteConsoleRequest->Unicode)
                RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
        }

        WriteConsoleRequest->NrCharactersWritten = Written;
    }

    ConSrvReleaseScreenBuffer(Buff, FALSE);
    return Status;
}


/* TEXT OUTPUT APIS ***********************************************************/

NTSTATUS NTAPI
ConDrvReadConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN BOOL Unicode,
                        OUT PCHAR_INFO CharInfo/*Buffer*/,
                        IN PCOORD BufferSize,
                        IN PCOORD BufferCoord,
                        IN OUT PSMALL_RECT ReadRegion);
CSR_API(SrvReadConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUT ReadOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvReadConsoleOutput\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadOutputRequest->CharInfo,
                                  ReadOutputRequest->BufferSize.X * ReadOutputRequest->BufferSize.Y,
                                  sizeof(CHAR_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ReadOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvReadConsoleOutput(Buffer->Header.Console,
                                     Buffer,
                                     ReadOutputRequest->Unicode,
                                     ReadOutputRequest->CharInfo,
                                     &ReadOutputRequest->BufferSize,
                                     &ReadOutputRequest->BufferCoord,
                                     &ReadOutputRequest->ReadRegion);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutput(IN PCONSOLE Console,
                         IN PTEXTMODE_SCREEN_BUFFER Buffer,
                         IN BOOL Unicode,
                         IN PCHAR_INFO CharInfo/*Buffer*/,
                         IN PCOORD BufferSize,
                         IN PCOORD BufferCoord,
                         IN OUT PSMALL_RECT WriteRegion);
CSR_API(SrvWriteConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvWriteConsoleOutput\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteOutputRequest->CharInfo,
                                  WriteOutputRequest->BufferSize.X * WriteOutputRequest->BufferSize.Y,
                                  sizeof(CHAR_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     WriteOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvWriteConsoleOutput(Buffer->Header.Console,
                                      Buffer,
                                      WriteOutputRequest->Unicode,
                                      WriteOutputRequest->CharInfo,
                                      &WriteOutputRequest->BufferSize,
                                      &WriteOutputRequest->BufferCoord,
                                      &WriteOutputRequest->WriteRegion);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

CSR_API(SrvWriteConsole)
{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;

    DPRINT("SrvWriteConsole\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&WriteConsoleRequest->Buffer,
                                  WriteConsoleRequest->BufferSize,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = DoWriteConsole(ApiMessage,
                            CsrGetClientThread(),
                            TRUE);

    if (Status == STATUS_PENDING)
        *ReplyCode = CsrReplyPending;

    return Status;
}

NTSTATUS NTAPI
ConDrvReadConsoleOutputString(IN PCONSOLE Console,
                              IN PTEXTMODE_SCREEN_BUFFER Buffer,
                              IN CODE_TYPE CodeType,
                              OUT PVOID StringBuffer,
                              IN ULONG NumCodesToRead,
                              IN PCOORD ReadCoord,
                              OUT PCOORD EndCoord,
                              OUT PULONG CodesRead);
CSR_API(SrvReadConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputCodeRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    ULONG CodeSize;

    DPRINT("SrvReadConsoleOutputString\n");

    switch (ReadOutputCodeRequest->CodeType)
    {
        case CODE_ASCII:
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadOutputCodeRequest->pCode.pCode,
                                  ReadOutputCodeRequest->NumCodesToRead,
                                  CodeSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ReadOutputCodeRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvReadConsoleOutputString(Buffer->Header.Console,
                                           Buffer,
                                           ReadOutputCodeRequest->CodeType,
                                           ReadOutputCodeRequest->pCode.pCode,
                                           ReadOutputCodeRequest->NumCodesToRead,
                                           &ReadOutputCodeRequest->ReadCoord,
                                           &ReadOutputCodeRequest->EndCoord,
                                           &ReadOutputCodeRequest->CodesRead);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutputString(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN CODE_TYPE CodeType,
                               IN PVOID StringBuffer,
                               IN ULONG NumCodesToWrite,
                               IN PCOORD WriteCoord /*,
                               OUT PCOORD EndCoord,
                               OUT PULONG CodesWritten */);
CSR_API(SrvWriteConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_WRITEOUTPUTCODE WriteOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputCodeRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    ULONG CodeSize;

    DPRINT("SrvWriteConsoleOutputString\n");

    switch (WriteOutputCodeRequest->CodeType)
    {
        case CODE_ASCII:
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteOutputCodeRequest->pCode.pCode,
                                  WriteOutputCodeRequest->Length,
                                  CodeSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     WriteOutputCodeRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvWriteConsoleOutputString(Buffer->Header.Console,
                                            Buffer,
                                            WriteOutputCodeRequest->CodeType,
                                            WriteOutputCodeRequest->pCode.pCode,
                                            WriteOutputCodeRequest->Length, // NumCodesToWrite,
                                            &WriteOutputCodeRequest->Coord /*, // WriteCoord,
                                            &WriteOutputCodeRequest->EndCoord,
                                            &WriteOutputCodeRequest->NrCharactersWritten */);

    // WriteOutputCodeRequest->NrCharactersWritten = Written;

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvFillConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN CODE_TYPE CodeType,
                        IN PVOID Code,
                        IN ULONG NumCodesToWrite,
                        IN PCOORD WriteCoord /*,
                        OUT PULONG CodesWritten */);
CSR_API(SrvFillConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FillOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    USHORT CodeType = FillOutputRequest->CodeType;

    DPRINT("SrvFillConsoleOutput\n");

    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     FillOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvFillConsoleOutput(Buffer->Header.Console,
                                     Buffer,
                                     CodeType,
                                     &FillOutputRequest->Code,
                                     FillOutputRequest->Length, // NumCodesToWrite,
                                     &FillOutputRequest->Coord /*, // WriteCoord,
                                     &FillOutputRequest->NrCharactersWritten */);

    // FillOutputRequest->NrCharactersWritten = Written;

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

#if 0000

CSR_API(SrvGetConsoleScreenBufferInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSCREENBUFFERINFO ScreenBufferInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScreenBufferInfoRequest;
    // PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    PCONSOLE_SCREEN_BUFFER_INFO pInfo = &ScreenBufferInfoRequest->Info;

    DPRINT("SrvGetConsoleScreenBufferInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), ScreenBufferInfoRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // Console = Buff->Header.Console;

    pInfo->dwSize = Buff->ScreenBufferSize;
    pInfo->dwCursorPosition = Buff->CursorPosition;
    pInfo->wAttributes      = Buff->ScreenDefaultAttrib;
    pInfo->srWindow.Left    = Buff->ViewOrigin.X;
    pInfo->srWindow.Top     = Buff->ViewOrigin.Y;
    pInfo->srWindow.Right   = Buff->ViewOrigin.X + Buff->ViewSize.X - 1;
    pInfo->srWindow.Bottom  = Buff->ViewOrigin.Y + Buff->ViewSize.Y - 1;
    pInfo->dwMaximumWindowSize = Buff->ScreenBufferSize; // TODO: Refine the computation

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleTextAttribute)
{
    NTSTATUS Status;
    PCONSOLE_SETTEXTATTRIB SetTextAttribRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetTextAttribRequest;
    PTEXTMODE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleTextAttribute\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetTextAttribRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Buff->ScreenDefaultAttrib = SetTextAttribRequest->Attrib;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleScreenBufferSize)
{
    NTSTATUS Status;
    PCONSOLE_SETSCREENBUFFERSIZE SetScreenBufferSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferSizeRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetScreenBufferSizeRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Status = ConioResizeBuffer(Console, Buff, SetScreenBufferSizeRequest->Size);
    if (NT_SUCCESS(Status)) ConioResizeTerminal(Console);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

CSR_API(SrvScrollConsoleScreenBuffer)
{
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScrollScreenBufferRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    SMALL_RECT SrcRegion;
    SMALL_RECT DstRegion;
    SMALL_RECT UpdateRegion;
    SMALL_RECT ScrollRectangle;
    SMALL_RECT ClipRectangle;
    NTSTATUS Status;
    HANDLE OutputHandle;
    BOOLEAN UseClipRectangle;
    COORD DestinationOrigin;
    CHAR_INFO FillChar;

    DPRINT("SrvScrollConsoleScreenBuffer\n");

    OutputHandle = ScrollScreenBufferRequest->OutputHandle;
    UseClipRectangle = ScrollScreenBufferRequest->UseClipRectangle;
    DestinationOrigin = ScrollScreenBufferRequest->DestinationOrigin;
    FillChar = ScrollScreenBufferRequest->Fill;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    ScrollRectangle = ScrollScreenBufferRequest->ScrollRectangle;

    /* Make sure source rectangle is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buff->ScreenBufferSize.Y - 1, Buff->ScreenBufferSize.X - 1);
    if (!ConioGetIntersection(&SrcRegion, &ScreenBuffer, &ScrollRectangle))
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_SUCCESS;
    }

    /* If the source was clipped on the left or top, adjust the destination accordingly */
    if (ScrollRectangle.Left < 0)
    {
        DestinationOrigin.X -= ScrollRectangle.Left;
    }
    if (ScrollRectangle.Top < 0)
    {
        DestinationOrigin.Y -= ScrollRectangle.Top;
    }

    if (UseClipRectangle)
    {
        ClipRectangle = ScrollScreenBufferRequest->ClipRectangle;
        if (!ConioGetIntersection(&ClipRectangle, &ClipRectangle, &ScreenBuffer))
        {
            ConSrvReleaseScreenBuffer(Buff, TRUE);
            return STATUS_SUCCESS;
        }
    }
    else
    {
        ClipRectangle = ScreenBuffer;
    }

    ConioInitRect(&DstRegion,
                  DestinationOrigin.Y,
                  DestinationOrigin.X,
                  DestinationOrigin.Y + ConioRectHeight(&SrcRegion) - 1,
                  DestinationOrigin.X + ConioRectWidth(&SrcRegion) - 1);

    if (!ScrollScreenBufferRequest->Unicode)
        ConsoleAnsiCharToUnicodeChar(Console, &FillChar.Char.UnicodeChar, &FillChar.Char.AsciiChar);

    ConioMoveRegion(Buff, &SrcRegion, &DstRegion, &ClipRectangle, FillChar);

    if ((PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
    {
        ConioGetUnion(&UpdateRegion, &SrcRegion, &DstRegion);
        if (ConioGetIntersection(&UpdateRegion, &UpdateRegion, &ClipRectangle))
        {
            /* Draw update region */
            ConioDrawRegion(Console, &UpdateRegion);
        }
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

#endif

/* EOF */
