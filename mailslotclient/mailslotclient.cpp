#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <locale.h>
#include <iostream>

HANDLE ghWriteEvent;
HANDLE hSlot;
LPCTSTR SlotName = TEXT("\\\\.\\mailslot\\sample_mailslot");

//������
BOOL WriteSlot(HANDLE hSlot, LPCTSTR lpszMessage)
{
    BOOL fResult;
    DWORD cbWritten;

    fResult = WriteFile(hSlot,
        lpszMessage,
        (DWORD)(lstrlen(lpszMessage) + 1) * sizeof(TCHAR),
        &cbWritten,
        (LPOVERLAPPED)NULL);

    if (!fResult)
    {
        printf("WriteFile �� ������� - %d.\n", GetLastError());
        return FALSE;
    }

    printf("������: ��������� ����������\n");

    return TRUE;
}

//������
BOOL ReadSlot()
{
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    LPTSTR lpszBuffer;
    TCHAR achID[80];
    DWORD cAllMessages;
    HANDLE hEvent;
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;
    hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("ExampleSlot"));

    if (NULL == hEvent)
        return FALSE;

    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    fResult = GetMailslotInfo(hSlot, // mailslot handle
        (LPDWORD)NULL,               // no maximum message size
        &cbMessage,                  // size of next message
        &cMessage,                   // number of messages
        (LPDWORD)NULL);              // no read time-out

    if (!fResult)
    {
        printf("�������� GetMailslotInfo() ����������� %d.\n", GetLastError());
        return FALSE;
    }
    else
        printf("������: ");

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        printf("������ ���������\n");
        return TRUE;
    }

    cAllMessages = cMessage;

    while (cMessage != 0)  // retrieve all messages
    {
        // Create a message-number string
        StringCchPrintf((LPTSTR)achID, 80, TEXT("\nMessage #%d of %d\n"), cAllMessages - cMessage + 1, cAllMessages);

        // Allocate memory for the message
        lpszBuffer = (LPTSTR)GlobalAlloc(GPTR, lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
        if (lpszBuffer == NULL)
            return FALSE;
        lpszBuffer[0] = '\0';
        fResult = ReadFile(hSlot, lpszBuffer, cbMessage, &cbRead, &ov);

        if (!fResult)
        {
            printf("ReadFile() ������ %d.\n", GetLastError());
            GlobalFree((HGLOBAL)lpszBuffer);
            return FALSE;
        }
        else
            printf("�������� ����� ���������!\n");

        // Concatenate the message and the message-number string
        StringCbCat(lpszBuffer, lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage, (LPTSTR)achID);

        // Display the message
        _tprintf(TEXT("���������� ��������: %s\n"), lpszBuffer);

        GlobalFree((HGLOBAL)lpszBuffer);

        fResult = GetMailslotInfo(hSlot,  // mailslot handle
            (LPDWORD)NULL,                // no maximum message size
            &cbMessage,                   // size of next message
            &cMessage,                    // number of messages
            (LPDWORD)NULL);               // no read time-out

        if (!fResult)
        {
            printf("GetMailslotInfo() ������ %d\n", GetLastError());
            return FALSE;
        }
        else
            printf("...\n");
    }

    if (CloseHandle(hEvent) != 0)
        printf("...\n");
    else
        printf("�� ������� ��������� %d\n", GetLastError());
    return TRUE;

}

//��������
BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{

    hSlot = CreateMailslot(lpszSlotName,
        0,                             // no maximum message size 
        MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
        (LPSECURITY_ATTRIBUTES)NULL);  // default security

    if (hSlot == INVALID_HANDLE_VALUE)
    {
        printf("�������� CreateMailslot �����������: %d\n", GetLastError());
        return FALSE;
    }
    else printf("Mailslot ������� ������\n\n");
    return TRUE;
}



//������ ������� ������ ���������� �� ������ ���������� ������.
//������ ���� (������)
//������
DWORD WINAPI Thread_1(LPVOID lpParam) {
    
    MakeSlot(SlotName);
    
    //������ ������ ������� ��� ������ ������ �������
    ghWriteEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("WriteEvent"));
    if (ghWriteEvent == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return 0;
    }

    Sleep(7000);
    ReadSlot();

    return 0;
}


//������ ������� ������ ������� ������� ������������ ������, ��������� ������ �������������� ��������� �������� ��������.
//������ ���� (������)
//������
DWORD WINAPI Thread_2(LPVOID lpParam) {

    DWORD dwWaitResult = ~WAIT_OBJECT_0;
    if (dwWaitResult != WAIT_OBJECT_0) 
    {
        dwWaitResult = WaitForSingleObject(ghWriteEvent, 1);

        HANDLE hFile;
        hFile = CreateFile(SlotName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            (HANDLE)NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            printf("�� ������� CreateFile. ������ %d.\n", GetLastError());
            return FALSE;
        }

        WriteSlot(hFile, TEXT("������ ���������"));
        WriteSlot(hFile, TEXT("��� ���� ���������"));
        WriteSlot(hFile, TEXT("��������� ���������"));

        CloseHandle(hFile);
    }
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "Russian");
    int Data_Of_Thread_1 = 1;
    int Data_Of_Thread_2 = 2;

    HANDLE Handle_of_Thread_1 = 0;
    HANDLE Handle_of_Thread_2 = 0;

    HANDLE Threads_Handles_Array[2];


    Handle_of_Thread_1 = CreateThread(NULL, 0, Thread_1, &Data_Of_Thread_1, 0, NULL);
    if (Handle_of_Thread_1 == NULL) {
        ExitProcess(Data_Of_Thread_1);
    }

    Handle_of_Thread_2 = CreateThread(NULL, 0, Thread_2, &Data_Of_Thread_2, 0, NULL);
    if (Handle_of_Thread_2 == NULL) {
        ExitProcess(Data_Of_Thread_2);
    }

    Threads_Handles_Array[0] = Handle_of_Thread_1;
    Threads_Handles_Array[1] = Handle_of_Thread_2;

    WaitForMultipleObjects(2, Threads_Handles_Array, TRUE, 20000);

    CloseHandle(Handle_of_Thread_1);
    CloseHandle(Handle_of_Thread_2);
}
