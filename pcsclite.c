#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <winscard.h>
#include <wintypes.h>

#define CATCH(err)                                       \
	if (err)                                             \
	{                                                    \
		printf("Error %s\n", pcsc_stringify_error(err)); \
		return 1;                                        \
	}

#define READER_NOTIFICATION "\\\\?PnP?\\Notification"

SCARDCONTEXT context = 0;

LONG pcscInit()
{
	return SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
}

LONG pcscDestroy()
{
	return SCardReleaseContext(context);
}

LONG pcscGetReaders(LPSTR *buffer, DWORD *bufferSize)
{
	LONG error;
	LPSTR buf;
	DWORD bufSize = 0;
	error = SCardListReaders(context, NULL, NULL, &bufSize);
	if (error)
	{
		return error;
	}
	buf = (LPSTR)malloc(sizeof(char) * bufSize);
	error = SCardListReaders(context, NULL, buf, &bufSize);
	*buffer = buf;
	*bufferSize = bufSize;
	return error;
}

LONG pcscConnect(LPCSTR reader, SCARDHANDLE *handle)
{
	DWORD activeProtocol;
	return SCardConnect(context, reader, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, handle, &activeProtocol);
}

LONG pcscDisconnect(SCARDHANDLE handle)
{
	return SCardDisconnect(handle, SCARD_UNPOWER_CARD);
}

LONG pcscGetStatus(SCARDHANDLE handle, DWORD *state)
{
	return SCardStatus(handle, NULL, NULL, state, NULL, NULL, NULL);
}

LONG pcscTransmit(SCARDHANDLE handle, LPCBYTE sendData, DWORD sendSize, LPBYTE *recvData, DWORD *recvSize)
{
	*recvSize = MAX_BUFFER_SIZE;
	*recvData = malloc(sizeof(char) * (*recvSize));
	return SCardTransmit(handle, SCARD_PCI_T0, sendData, sendSize, NULL, *recvData, recvSize);
}

LONG pcscWaitUntilReaderChange(DWORD curState, LPCSTR readerName, DWORD *newState)
{
	LONG error;
	SCARD_READERSTATE state;
	state.szReader = readerName;
	state.dwCurrentState = curState;

	error = SCardGetStatusChange(context, INFINITE, &state, 1);
	*newState = state.dwEventState;
	return error;
}

LONG pcscWaitUntilGlobalChange(DWORD *newState)
{
	LONG error;
	SCARD_READERSTATE state;
	state.szReader = READER_NOTIFICATION;
	state.dwCurrentState = SCARD_STATE_UNAWARE;

	error = SCardGetStatusChange(context, INFINITE, &state, 1);
	*newState = state.dwEventState;
	return error;
}

LONG pcscWaitUntilReaderConnected(LPSTR *buffer, DWORD *bufSize)
{

	LONG error = pcscGetReaders(buffer, bufSize);
	if (error == SCARD_E_NO_READERS_AVAILABLE)
	{
		DWORD globalState;
		do
		{
			error = pcscWaitUntilGlobalChange(&globalState);
		} while (!error && globalState & SCARD_STATE_UNAVAILABLE);
	}
	if (!error)
	{
		error = pcscGetReaders(buffer, bufSize);
	}
	return error;
}
LONG pcscWaitUntilReaderState(LPSTR buffer, DWORD desiredState)
{
	LONG error;
	DWORD readerState = SCARD_STATE_UNAWARE;
	do
	{
		error = pcscWaitUntilReaderChange(readerState, buffer, &readerState);
	} while (!error && !(readerState & desiredState));
	return error;
}

int main()
{
	CATCH(pcscInit());

	LPSTR names;
	DWORD namesLen;
	CATCH(pcscWaitUntilReaderConnected(&names, &namesLen));
	printf("Reader connected\n");
	printf("Bufsize: %lu\n", namesLen);
	printf("Reader: %s\n", names);

	while (1)
	{
		CATCH(pcscWaitUntilReaderState(names, SCARD_STATE_PRESENT));
		printf("Card inserted\n");

		SCARDHANDLE handle;
		CATCH(pcscConnect(names, &handle));

		const BYTE send[] = {0xFF, 0xB0, 0x00, 0x0D, 0x04};
		LPBYTE received;
		DWORD recvSize;
		CATCH(pcscTransmit(handle, send, sizeof(send), &received, &recvSize));

		for (int i = 0; i < recvSize; i++)
		{
			printf("0x%x ", received[i]);
		}
		printf("\n");

		CATCH(pcscDisconnect(handle));
		free(received);

		CATCH(pcscWaitUntilReaderState(names, SCARD_STATE_EMPTY));
		printf("Card removed\n");
	}

	free(names);
	CATCH(pcscDestroy());
	return 0;
}