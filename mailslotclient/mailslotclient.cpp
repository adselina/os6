#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <locale.h>
#include <iostream>

HANDLE ghWriteEvent;
HANDLE hSlot;
LPCTSTR SlotName = TEXT("\\\\.\\mailslot\\sample_mailslot");

//запись
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
        printf("WriteFile не удалось - %d.\n", GetLastError());
        return FALSE;
    }

    printf("Клиент: Сообщение отправлено\n");

    return TRUE;
}

//чтение
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
        printf("Создание GetMailslotInfo() провалилось %d.\n", GetLastError());
        return FALSE;
    }
    else
        printf("Сервер: ");

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        printf("Ожидаю сообщение\n");
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
            printf("ReadFile() ошибка %d.\n", GetLastError());
            GlobalFree((HGLOBAL)lpszBuffer);
            return FALSE;
        }
        else
            printf("Получено новое сообщение!\n");

        // Concatenate the message and the message-number string
        StringCbCat(lpszBuffer, lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage, (LPTSTR)achID);

        // Display the message
        _tprintf(TEXT("Содержимое мэйлслот: %s\n"), lpszBuffer);

        GlobalFree((HGLOBAL)lpszBuffer);

        fResult = GetMailslotInfo(hSlot,  // mailslot handle
            (LPDWORD)NULL,                // no maximum message size
            &cbMessage,                   // size of next message
            &cMessage,                    // number of messages
            (LPDWORD)NULL);               // no read time-out

        if (!fResult)
        {
            printf("GetMailslotInfo() ошибка %d\n", GetLastError());
            return FALSE;
        }
        else
            printf("...\n");
    }

    if (CloseHandle(hEvent) != 0)
        printf("...\n");
    else
        printf("Не удалось прочитать %d\n", GetLastError());
    return TRUE;

}

//создание
BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{

    hSlot = CreateMailslot(lpszSlotName,
        0,                             // no maximum message size 
        MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
        (LPSECURITY_ATTRIBUTES)NULL);  // default security

    if (hSlot == INVALID_HANDLE_VALUE)
    {
        printf("Создание CreateMailslot провалилось: %d\n", GetLastError());
        return FALSE;
    }
    else printf("Mailslot успешно создан\n\n");
    return TRUE;
}



//ПЕРВЫЙ процесс должен напечатать на экране полученные данные.
//вторая нить (чтение)
//сервер
DWORD WINAPI Thread_1(LPVOID lpParam) {
    
    MakeSlot(SlotName);
    
    //Первый создал событие для началы работы второго
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


//ВТОРОЙ процесс должен послать первому произвольную строку, используя способ взаимодействия процессов согласно варианту.
//первая нить (запись)
//клиент
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
            printf("Не удалось CreateFile. Ошибка %d.\n", GetLastError());
            return FALSE;
        }

        WriteSlot(hFile, TEXT("Первое сообщение"));
        WriteSlot(hFile, TEXT("Еще одно сообщение"));
        WriteSlot(hFile, TEXT("Последние сообщение"));

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
